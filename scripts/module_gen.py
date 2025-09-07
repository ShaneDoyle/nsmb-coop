#!/usr/bin/env python3
"""
Module generator for NSMB projects.

This script processes module configuration files and generates build configurations
for ARM7 and ARM9 processors, along with VSCode IntelliSense configuration.
"""

import argparse
import glob
import json
import os
import re
import yaml
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set, Union


@dataclass
class ComponentOverride:
    """Override settings for a module component."""
    enabled: Optional[bool] = None
    target: Optional[str] = None
    defines: Optional[Dict[str, Union[str, int, bool]]] = None


@dataclass
class ProcessedModule:
    """Container for processed module data."""
    name: str
    path: Path
    config: dict
    enabled_components: Set[str] = field(default_factory=set)
    disabled_components: Set[str] = field(default_factory=set)
    overrides: Dict[str, ComponentOverride] = field(default_factory=dict)


class ModuleProcessor:
    """Processes module configurations and generates build files."""

    def __init__(self, modules_dir: Path, output_dir: Path, compiler_path: Path):
        self.modules_dir = modules_dir
        self.output_dir = output_dir
        self.compiler_path = compiler_path
        self.root_dir = Path.cwd()
        self.nitrofs_map_filename = 'nitrofs_file_map.txt'  # Default filename

        # Collections for build configuration
        self.arm7_defines: Dict[str, Union[str, int, bool, None]] = {}
        self.arm9_defines: Dict[str, Union[str, int, bool, None]] = {}
        self.component_sources: Dict[tuple, bool] = {}  # (file, base, region) -> enabled

        # Collections for nitrofs file mapping
        self.nitrofs_file_map: Dict[str, str] = {}  # final_path -> source_path

    def load_yaml(self, path: Path) -> dict:
        """Load and parse a YAML file."""
        with path.open('r', encoding='utf-8') as f:
            return yaml.safe_load(f) or {}

    def load_json(self, path: Path) -> dict:
        """Load and parse a JSON file."""
        with path.open('r', encoding='utf-8') as f:
            return json.load(f)

    def save_json(self, data: dict, path: Path) -> None:
        """Save data as formatted JSON."""
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('w', encoding='utf-8') as f:
            json.dump(data, f, indent=4, sort_keys=False)

    def ensure_list(self, value) -> List:
        """Convert value to list if it isn't already."""
        if value is None:
            return []
        return value if isinstance(value, list) else [value]

    def resolve_source_paths(self, module_dir: Path, source_pattern: str) -> List[str]:
        """
        Resolve source patterns to actual file paths.
        Supports glob patterns and returns paths relative to working directory.
        """
        if isinstance(source_pattern, list):
            source_pattern = source_pattern[0]

        pattern_str = str(source_pattern)
        has_glob = any(char in pattern_str for char in '*?[')

        if has_glob:
            # Expand glob pattern
            full_pattern = str((module_dir / pattern_str).resolve())
            matches = glob.glob(full_pattern, recursive=True)
            matches.sort()  # Deterministic order

            resolved_paths = []
            for match in matches:
                match_path = Path(match)
                # Skip directories - we only want files
                if match_path.is_file():
                    rel_path = os.path.relpath(match, self.root_dir)
                    resolved_paths.append(Path(rel_path).as_posix())

            if not resolved_paths:
                print(f"Warning: glob pattern '{full_pattern}' matched no files")
                # Fallback to literal path
                fallback_path = module_dir / pattern_str
                rel_path = os.path.relpath(fallback_path, self.root_dir)
                resolved_paths.append(Path(rel_path).as_posix())

            return resolved_paths
        else:
            # Single file path
            file_path = module_dir / pattern_str
            if file_path.exists() and file_path.is_dir():
                print(f"Error: '{file_path}' is a directory. Only files or globs allowed.")
                return []

            rel_path = os.path.relpath(file_path, self.root_dir)
            return [Path(rel_path).as_posix()]

    def parse_target(self, target_str: str) -> tuple[str, Optional[int], bool]:
        """Parse target string like 'arm9' or 'arm9(12)' into base, overlay number, and locked flag.

        Supports locked targets with '!' prefix, e.g. '!arm9' or '!arm9(12)'.
        """
        is_locked = False
        if target_str.startswith('!'):
            is_locked = True
            target_str = target_str[1:]  # Remove the '!' prefix

        if '(' not in target_str:
            return target_str, None, is_locked

        base, overlay_part = target_str.split('(', 1)
        try:
            overlay_num = int(overlay_part.rstrip(')'))
            return base, overlay_num, is_locked
        except ValueError:
            return base, None, is_locked

    def get_region_dest(self, overlay_num: Optional[int]) -> str:
        """Convert overlay number to region destination string."""
        return "main" if overlay_num is None else f"ov{overlay_num}"

    def parse_define(self, define_input: Union[str, dict]) -> tuple[str, Union[str, int, bool, None]]:
        """
        Parse a define input into name and value.

        Args:
            define_input: Can be:
                - String: "DEFINE_NAME" -> (DEFINE_NAME, None)
                - String: "DEFINE_NAME=value" -> (DEFINE_NAME, value)
                - Dict: {"DEFINE_NAME": value} -> (DEFINE_NAME, value)

        Returns:
            Tuple of (define_name, define_value)
        """
        if isinstance(define_input, str):
            if '=' in define_input:
                name, value = define_input.split('=', 1)
                # Try to convert value to appropriate type
                try:
                    # Try integer first
                    if value.startswith('0x') or value.startswith('0X'):
                        return name.strip(), int(value, 16)
                    elif value.isdigit() or (value.startswith('-') and value[1:].isdigit()):
                        return name.strip(), int(value)
                    elif value.lower() in ('true', 'false'):
                        return name.strip(), value.lower() == 'true'
                    else:
                        return name.strip(), value
                except ValueError:
                    return name.strip(), value
            else:
                return define_input.strip(), None
        elif isinstance(define_input, dict):
            if len(define_input) == 1:
                name, value = next(iter(define_input.items()))
                return str(name), value
            else:
                raise ValueError(f"Define dict must have exactly one key-value pair: {define_input}")
        else:
            raise ValueError(f"Invalid define format: {define_input}")

    def add_define(self, target_defines: Dict[str, Union[str, int, bool, None]],
                   define_name: str, define_value: Union[str, int, bool, None],
                   overrides: Optional[Dict[str, Union[str, int, bool]]] = None) -> None:
        """Add a define to the target dictionary, applying overrides if provided."""
        # Apply override if available
        if overrides and define_name in overrides:
            define_value = overrides[define_name]

        target_defines[define_name] = define_value

    def parse_component_overrides(self, components_config: List) -> Dict[str, ComponentOverride]:
        """Parse component override configuration from modules.yaml."""
        overrides = {}

        for component_item in self.ensure_list(components_config):
            if not isinstance(component_item, dict):
                continue

            for comp_name, comp_value in component_item.items():
                override = ComponentOverride()

                if isinstance(comp_value, bool):
                    override.enabled = comp_value
                elif isinstance(comp_value, dict):
                    override.enabled = comp_value.get('enabled')
                    override.target = comp_value.get('target')
                    # Handle define overrides
                    if 'defines' in comp_value:
                        override.defines = comp_value['defines']
                elif isinstance(comp_value, list):
                    # Handle list of override dicts
                    for item in comp_value:
                        if isinstance(item, dict):
                            if 'enabled' in item:
                                override.enabled = bool(item['enabled'])
                            if 'target' in item:
                                override.target = item['target']
                            if 'defines' in item:
                                if override.defines is None:
                                    override.defines = {}
                                override.defines.update(item['defines'])

                overrides[str(comp_name)] = override

        return overrides

    def load_enabled_modules(self) -> List[ProcessedModule]:
        """Load and process all enabled modules from modules.yaml."""
        modules_yaml = self.modules_dir / "modules.yaml"
        if not modules_yaml.exists():
            print(f"Error: {modules_yaml} not found")
            return []

        modules_config = self.load_yaml(modules_yaml)
        processed_modules = []

        for module_item in modules_config.get('modules', []):
            if not isinstance(module_item, dict):
                continue

            for module_name, module_config in module_item.items():
                if not module_config or not module_config.get('enabled', False):
                    continue

                module_path = self.modules_dir / module_name
                module_yaml_path = module_path / "module.yaml"

                if not module_yaml_path.exists():
                    print(f"Warning: module.yaml not found for '{module_name}' at {module_yaml_path}")
                    continue

                module_data = self.load_yaml(module_yaml_path)
                overrides = self.parse_component_overrides(module_config.get('components', []))

                processed_module = ProcessedModule(
                    name=module_name,
                    path=module_path,
                    config=module_data,
                    overrides=overrides
                )

                # Determine enabled/disabled components
                for component_item in module_data.get('components', []):
                    if isinstance(component_item, dict):
                        for comp_name in component_item.keys():
                            override = overrides.get(comp_name, ComponentOverride())
                            if override.enabled is False:
                                processed_module.disabled_components.add(comp_name)
                            else:
                                processed_module.enabled_components.add(comp_name)

                processed_modules.append(processed_module)

        return processed_modules

    def validate_component_requirements(self, modules: List[ProcessedModule]) -> bool:
        """Validate that component requirements are satisfied."""
        has_errors = False

        for module in modules:
            for component_item in module.config.get('components', []):
                if not isinstance(component_item, dict):
                    continue

                for comp_name, comp_config in component_item.items():
                    # Skip disabled components
                    if comp_name in module.disabled_components:
                        continue

                    if not isinstance(comp_config, dict):
                        continue

                    # Check if this component has requirements
                    requires = comp_config.get('requires')
                    if requires:
                        required_components = self.ensure_list(requires)

                        for required_comp in required_components:
                            required_comp = str(required_comp).strip()

                            # Check if the required component is disabled
                            if required_comp in module.disabled_components:
                                print(f"Error: Component '{comp_name}' in module '{module.name}' requires '{required_comp}', but '{required_comp}' is disabled.")
                                print(f"  To fix this, either:")
                                print(f"    1. Enable '{required_comp}' by removing it from the disabled components list")
                                print(f"    2. Disable '{comp_name}' by adding it to the disabled components list")
                                has_errors = True
                            # Check if the required component exists in the module
                            elif not self._component_exists_in_module(module, required_comp):
                                print(f"Error: Component '{comp_name}' in module '{module.name}' requires '{required_comp}', but '{required_comp}' does not exist in the module.")
                                has_errors = True

        return not has_errors

    def _component_exists_in_module(self, module: ProcessedModule, comp_name: str) -> bool:
        """Check if a component exists in the module configuration."""
        for component_item in module.config.get('components', []):
            if isinstance(component_item, dict):
                if comp_name in component_item:
                    return True
        return False

    def collect_component_sources(self, modules: List[ProcessedModule]) -> None:
        """Pre-collect all component source assignments to avoid conflicts."""
        for module in modules:
            for component_item in module.config.get('components', []):
                if not isinstance(component_item, dict):
                    continue

                for comp_name, comp_config in component_item.items():
                    if comp_name in module.disabled_components:
                        continue

                    if not isinstance(comp_config, dict):
                        continue

                    # Get target (with possible override)
                    target = comp_config.get('target')
                    original_target_locked = False

                    # Check if original target is locked
                    if target and target.startswith('!'):
                        original_target_locked = True

                    override = module.overrides.get(comp_name, ComponentOverride())
                    # Only apply override if original target is not locked
                    if override.target and not original_target_locked:
                        target = override.target

                    if not target:
                        continue

                    base, overlay_num, target_locked = self.parse_target(target)
                    if base not in ('arm7', 'arm9'):
                        continue

                    region = self.get_region_dest(overlay_num)

                    # Process component sources
                    sources = comp_config.get('sources', [])
                    for source_pattern in self.ensure_list(sources):
                        resolved_paths = self.resolve_source_paths(module.path, source_pattern)
                        for source_path in resolved_paths:
                            self.component_sources[(source_path, base, region)] = True

    def add_sources_to_region(self, build_config: dict, region_dest: str, new_sources: List[str]) -> None:
        """Add source files to a specific region in the build configuration."""
        regions = build_config.setdefault('regions', [])

        # Find the target region
        target_region = None
        for region in regions:
            if region.get('dest') == region_dest:
                target_region = region
                break

        if not target_region:
            raise RuntimeError(f"Error: region '{region_dest}' not found in build configuration")

        # Add sources, avoiding duplicates
        region_sources = target_region.setdefault('sources', [])
        existing_sources = set(region_sources)

        for source in new_sources:
            if source not in existing_sources:
                region_sources.append(source)
                existing_sources.add(source)

    def add_include_path(self, build_config: dict, module_path: Path, include_path: str) -> None:
        """Add an include path to the build configuration."""
        full_path = module_path / include_path
        rel_path = os.path.relpath(full_path, self.root_dir)
        posix_path = Path(rel_path).as_posix()

        includes = build_config.setdefault('includes', [])
        if posix_path not in includes:
            includes.append(posix_path)

    def process_module_defines(self, module: ProcessedModule) -> None:
        """Process module-level defines and add them to appropriate ARM configurations."""
        module_defines = self.ensure_list(module.config.get('defines', []))
        if not module_defines:
            return

        # Determine which ARM processors this module targets
        targets = module.config.get('targets', {})
        arm_targets = set()

        # Handle both dict and list format for targets
        target_items = targets.items() if isinstance(targets, dict) else []
        if isinstance(targets, list):
            for target_item in targets:
                if isinstance(target_item, dict):
                    target_items.extend(target_item.items())

        for target_name, _ in target_items:
            base, _, _ = self.parse_target(target_name)
            if base in ('arm7', 'arm9'):
                arm_targets.add(base)

        # Add defines to appropriate ARM configurations
        # Default to arm9 if no specific targets
        if not arm_targets:
            arm_targets.add('arm9')

        for define in module_defines:
            define_name, define_value = self.parse_define(define)
            if 'arm7' in arm_targets:
                self.add_define(self.arm7_defines, define_name, define_value)
            if 'arm9' in arm_targets:
                self.add_define(self.arm9_defines, define_name, define_value)

    def process_module_targets(self, module: ProcessedModule, arm7_config: dict, arm9_config: dict) -> None:
        """Process module target configurations."""
        targets = module.config.get('targets', {})

        # Handle both dict and list formats
        target_items = []
        if isinstance(targets, dict):
            target_items = list(targets.items())
        elif isinstance(targets, list):
            for item in targets:
                if isinstance(item, dict):
                    target_items.extend(item.items())

        for target_name, target_config in target_items:
            base, overlay_num, _ = self.parse_target(target_name)
            if base not in ('arm7', 'arm9'):
                continue

            build_config = arm9_config if base == 'arm9' else arm7_config
            region_dest = self.get_region_dest(overlay_num)

            # Handle both dict and list formats for target_config
            config_items = [target_config] if isinstance(target_config, dict) else self.ensure_list(target_config)

            for config_item in config_items:
                if not isinstance(config_item, dict):
                    continue

                # Process includes
                includes = config_item.get('includes', [])
                for include_path in self.ensure_list(includes):
                    self.add_include_path(build_config, module.path, include_path)

                # Process sources
                sources = config_item.get('sources', [])
                if sources:
                    resolved_sources = []
                    for source_pattern in self.ensure_list(sources):
                        resolved_paths = self.resolve_source_paths(module.path, source_pattern)
                        resolved_sources.extend(resolved_paths)

                    # Filter out disabled component sources
                    filtered_sources = self.filter_disabled_sources(module, resolved_sources)

                    # Filter out sources assigned to different regions via components
                    final_sources = self.filter_component_region_conflicts(
                        filtered_sources, base, region_dest
                    )

                    if final_sources:
                        self.add_sources_to_region(build_config, region_dest, final_sources)

    def filter_disabled_sources(self, module: ProcessedModule, sources: List[str]) -> List[str]:
        """Filter out sources belonging to disabled components."""
        if not module.disabled_components:
            return sources

        # Get all sources from disabled components
        disabled_sources = set()
        for component_item in module.config.get('components', []):
            if not isinstance(component_item, dict):
                continue

            for comp_name, comp_config in component_item.items():
                if comp_name not in module.disabled_components:
                    continue

                if isinstance(comp_config, dict):
                    # Add component sources to disabled set
                    comp_sources = comp_config.get('sources', [])
                    for source_pattern in self.ensure_list(comp_sources):
                        resolved_paths = self.resolve_source_paths(module.path, source_pattern)
                        disabled_sources.update(resolved_paths)

        # Filter out disabled sources
        return [src for src in sources if src not in disabled_sources]

    def filter_component_region_conflicts(self, sources: List[str], base: str, region: str) -> List[str]:
        """Filter out sources assigned to components targeting different regions."""
        filtered = []
        for source in sources:
            # Check if this source is assigned to a component for a different region
            assigned_elsewhere = any(
                src == source and src_base == base and src_region != region
                for (src, src_base, src_region) in self.component_sources
            )
            if not assigned_elsewhere:
                filtered.append(source)

        return filtered

    def process_module_components(self, module: ProcessedModule, arm7_config: dict, arm9_config: dict) -> None:
        """Process module component configurations."""
        for component_item in module.config.get('components', []):
            if not isinstance(component_item, dict):
                continue

            for comp_name, comp_config in component_item.items():
                if comp_name in module.disabled_components:
                    continue

                if not isinstance(comp_config, dict):
                    continue

                # Get target (with possible override)
                target = comp_config.get('target')
                original_target_locked = False

                # Check if original target is locked
                if target and target.startswith('!'):
                    original_target_locked = True

                override = module.overrides.get(comp_name, ComponentOverride())
                # Only apply override if original target is not locked
                if override.target and not original_target_locked:
                    target = override.target

                if not target:
                    continue

                base, overlay_num, target_locked = self.parse_target(target)
                if base not in ('arm7', 'arm9'):
                    continue

                build_config = arm9_config if base == 'arm9' else arm7_config
                region_dest = self.get_region_dest(overlay_num)

                # Process component defines
                comp_defines = self.ensure_list(comp_config.get('defines', []))
                for define in comp_defines:
                    define_name, define_value = self.parse_define(define)
                    target_defines = self.arm9_defines if base == 'arm9' else self.arm7_defines
                    self.add_define(target_defines, define_name, define_value, override.defines)

                # Process component includes
                includes = self.ensure_list(comp_config.get('includes', []))
                for include_path in includes:
                    self.add_include_path(build_config, module.path, include_path)

                # Process component sources
                sources = self.ensure_list(comp_config.get('sources', []))
                if sources:
                    resolved_sources = []
                    for source_pattern in sources:
                        resolved_paths = self.resolve_source_paths(module.path, source_pattern)
                        resolved_sources.extend(resolved_paths)

                    if resolved_sources:
                        self.add_sources_to_region(build_config, region_dest, resolved_sources)

    def inject_defines_into_flags(self, build_config: dict, defines: Dict[str, Union[str, int, bool, None]]) -> None:
        """Inject preprocessor defines into ARM flags."""
        if not defines or '$arm_flags' not in build_config:
            return

        define_flags = []
        for define_name, define_value in sorted(defines.items()):
            if define_value is None:
                # Simple define without value
                define_flag = f'-D{define_name}'
            elif isinstance(define_value, bool):
                # Boolean define
                define_flag = f'-D{define_name}={1 if define_value else 0}'
            elif isinstance(define_value, int):
                # Integer define (including hex)
                if define_value >= 0 and define_value <= 15:
                    # Small integers as decimal
                    define_flag = f'-D{define_name}={define_value}'
                else:
                    # Larger integers as hex
                    define_flag = f'-D{define_name}=0x{define_value:X}'
            else:
                # String define
                define_flag = f'-D{define_name}={define_value}'

            define_flags.append(define_flag)

        if define_flags:
            current_flags = build_config['$arm_flags']
            build_config['$arm_flags'] = current_flags + ' ' + ' '.join(define_flags)

    def remove_empty_regions(self, build_config: dict) -> None:
        """Remove regions with no sources from build configuration."""
        regions = build_config.get('regions', [])
        filtered_regions = [
            region for region in regions
            if region.get('sources') and len(region['sources']) > 0
        ]
        build_config['regions'] = filtered_regions

    def generate_vscode_config(self, arm7_config: dict, arm9_config: dict) -> None:
        """Generate VSCode C++ IntelliSense configuration."""
        # Skip generating VSCode config if compiler path is not specified or doesn't exist
        if not self.compiler_path or not self.compiler_path.exists():
            if not self.compiler_path:
                print("Skipping VSCode configuration generation: no compiler path specified")
            else:
                print(f"Skipping VSCode configuration generation: compiler path does not exist: {self.compiler_path}")
            return

        vscode_dir = Path('.vscode')
        vscode_dir.mkdir(exist_ok=True)

        # Collect include paths
        includes = set()
        includes.update(arm7_config.get('includes', []))
        includes.update(arm9_config.get('includes', []))

        # Extract defines from ARM flags and our define dictionaries
        defines = set()

        # Get defines from ARM flags (for backward compatibility)
        for config in [arm7_config, arm9_config]:
            flag_string = config.get('$arm_flags', '')
            found_defines = re.findall(r'-D([A-Za-z0-9_]+(?:=[^\\s]*)?)', flag_string)
            defines.update(found_defines)

        # Add defines from our dictionaries (arm9 takes precedence)
        for define_name, define_value in self.arm7_defines.items():
            if define_value is None:
                defines.add(define_name)
            else:
                if isinstance(define_value, bool):
                    defines.add(f'{define_name}={1 if define_value else 0}')
                elif isinstance(define_value, int):
                    if define_value >= 0 and define_value <= 15:
                        defines.add(f'{define_name}={define_value}')
                    else:
                        defines.add(f'{define_name}=0x{define_value:X}')
                else:
                    defines.add(f'{define_name}={define_value}')

        for define_name, define_value in self.arm9_defines.items():
            # Remove any existing define with the same name
            defines = {d for d in defines if not d.startswith(f'{define_name}=')}
            defines.discard(define_name)  # Remove simple define without value

            if define_value is None:
                defines.add(define_name)
            else:
                if isinstance(define_value, bool):
                    defines.add(f'{define_name}={1 if define_value else 0}')
                elif isinstance(define_value, int):
                    if define_value >= 0 and define_value <= 15:
                        defines.add(f'{define_name}={define_value}')
                    else:
                        defines.add(f'{define_name}=0x{define_value:X}')
                else:
                    defines.add(f'{define_name}={define_value}')

        # Add common defines
        common_defines = ['SDK_GCC', 'SDK_ARM9', 'SDK_FINALROM', 'IDE', 'NTR_DEBUG']
        defines.update(common_defines)

        cpp_config = {
            'configurations': [{
                'name': 'DS-ARM9',
                'includePath': sorted(list(includes)),
                'forcedInclude': ['${env:NCPATCHER_ROOT}/ncp_ide.h'],
                'defines': sorted(list(defines)),
                'compilerPath': str(self.compiler_path),
                'cStandard': 'c11',
                'cppStandard': 'c++23',
                'intelliSenseMode': 'gcc-arm'
            }],
            'version': 4
        }

        config_path = vscode_dir / 'c_cpp_properties.json'
        with config_path.open('w', encoding='utf-8') as f:
            json.dump(cpp_config, f, indent=2)

        print(f"VSCode C++ configuration written to {config_path}")

    def collect_module_nitrofs_files(self, module: ProcessedModule) -> Dict[str, str]:
        """Collect all nitrofs files from a module, respecting component excludes."""
        module_files = {}
        nitrofs_dir = module.path / "nitrofs"

        if not nitrofs_dir.exists():
            return module_files

        # Collect files that should be excluded based on disabled components
        excluded_files = set()
        for component_item in module.config.get('components', []):
            if not isinstance(component_item, dict):
                continue

            for comp_name, comp_config in component_item.items():
                if comp_name in module.disabled_components:
                    # This component is disabled, exclude its files
                    component_files = comp_config.get('files', []) if isinstance(comp_config, dict) else []
                    for file_pattern in self.ensure_list(component_files):
                        excluded_files.add(file_pattern)

        # Define supported language prefixes
        supported_languages = ['en', 'fr', 'ge', 'it', 'jp', 'sp', 'pt', 'ko', 'ch']

        # Collect all files from nitrofs directory
        for file_path in nitrofs_dir.rglob('*'):
            if file_path.is_file():
                # Get relative path from nitrofs root
                rel_path = file_path.relative_to(nitrofs_dir)
                rel_path_str = rel_path.as_posix()

                # Check if this file should be excluded
                should_exclude = False
                for excluded_pattern in excluded_files:
                    # Check for exact match
                    if rel_path_str == excluded_pattern:
                        should_exclude = True
                        break

                    # Check if file matches pattern when accounting for language prefixes
                    # If the file is under a language directory (e.g., en/enemy/w3_sign.nsbmd)
                    # and the excluded pattern is enemy/w3_sign.nsbmd, it should be excluded
                    path_parts = rel_path.parts
                    if len(path_parts) > 1 and path_parts[0] in supported_languages:
                        # Remove language prefix and check if it matches the excluded pattern
                        file_without_lang = '/'.join(path_parts[1:])
                        if file_without_lang == excluded_pattern:
                            should_exclude = True
                            break

                    # Also check if the excluded pattern ends the path (legacy behavior)
                    if rel_path_str.endswith('/' + excluded_pattern):
                        should_exclude = True
                        break

                if not should_exclude:
                    module_files[rel_path_str] = str(file_path)

        return module_files

    def generate_nitrofs_file_map(self, modules: List[ProcessedModule]) -> None:
        """Generate nitrofs file mapping based on module order."""
        print("Generating nitrofs file mapping...")

        # Process modules in order (earlier modules take precedence over later ones)
        for module in modules:
            print(f"  Collecting files from module: {module.name}")
            module_files = self.collect_module_nitrofs_files(module)

            # Add files to the final mapping, preserving earlier modules' files
            for rel_path, source_path in module_files.items():
                if rel_path in self.nitrofs_file_map:
                    print(f"    Skipping {rel_path} (already provided by earlier module)")
                else:
                    print(f"    Adding {rel_path}")
                    self.nitrofs_file_map[rel_path] = source_path

        print(f"Total nitrofs files collected: {len(self.nitrofs_file_map)}")

    def save_nitrofs_file_map(self) -> None:
        """Save the nitrofs file mapping to a text file for insert_files.py."""
        if not self.nitrofs_file_map:
            print("No nitrofs files to save")
            return

        map_file_path = self.output_dir / self.nitrofs_map_filename
        self.output_dir.mkdir(parents=True, exist_ok=True)

        with map_file_path.open('w', encoding='utf-8') as f:
            f.write("# Nitrofs file mapping generated by module_gen.py\n")
            f.write("# Format: relative_path=absolute_source_path\n")
            f.write(f"# Generated from {len(self.nitrofs_file_map)} files\n\n")

            # Sort by relative path for consistent output
            for rel_path in sorted(self.nitrofs_file_map.keys()):
                source_path = self.nitrofs_file_map[rel_path]
                f.write(f"{rel_path}={source_path}\n")

        print(f"Nitrofs file mapping written to {map_file_path}")

    def generate_build_configs(self) -> None:
        """Main method to generate build configurations."""
        print("Loading enabled modules...")
        modules = self.load_enabled_modules()

        if not modules:
            print("No enabled modules found.")
            return

        print(f"Processing {len(modules)} enabled modules...")

        # Validate component requirements
        print("Validating component requirements...")
        if not self.validate_component_requirements(modules):
            print("Error: Component requirement validation failed. Please fix the errors above before continuing.")
            return

        # Load base configurations
        arm7_config = self.load_json(self.root_dir / 'arm7.json')
        arm9_config = self.load_json(self.root_dir / 'arm9.json')

        # Pre-collect component sources to handle conflicts
        self.collect_component_sources(modules)

        # Process each module
        for module in modules:
            print(f"  Processing module: {module.name}")

            # Process module-level defines
            self.process_module_defines(module)

            # Process targets
            self.process_module_targets(module, arm7_config, arm9_config)

            # Process components
            self.process_module_components(module, arm7_config, arm9_config)

        # Inject defines into ARM flags
        self.inject_defines_into_flags(arm7_config, self.arm7_defines)
        self.inject_defines_into_flags(arm9_config, self.arm9_defines)

        # Clean up empty regions
        self.remove_empty_regions(arm7_config)
        self.remove_empty_regions(arm9_config)

        # Save output files
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.save_json(arm7_config, self.output_dir / 'arm7.json')
        self.save_json(arm9_config, self.output_dir / 'arm9.json')

        print(f"Build configurations written to {self.output_dir}")

        # Generate nitrofs file mapping
        self.generate_nitrofs_file_map(modules)
        self.save_nitrofs_file_map()

        # Generate VSCode configuration
        self.generate_vscode_config(arm7_config, arm9_config)


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Generate build configurations from module definitions',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This script processes module YAML files and generates ARM7/ARM9 build configurations
for NSMB projects. It also creates VSCode IntelliSense configuration.

Example usage:
  python module_gen.py
  python module_gen.py --modules-dir modules --out-dir build/generated
        """
    )

    parser.add_argument(
        '--modules-dir',
        type=Path,
        default=Path('modules'),
        help='Directory containing modules.yaml and module subdirectories'
    )

    parser.add_argument(
        '--out-dir',
        type=Path,
        default=Path('build/generated'),
        help='Output directory for generated build configurations'
    )

    parser.add_argument(
        '--compiler-path',
        type=Path,
        help='Path to ARM GCC compiler for VSCode configuration'
    )

    parser.add_argument(
        '--nitrofs-map-file',
        type=str,
        default='nitrofs_file_map.txt',
        help='Name of the nitrofs file mapping output file (default: nitrofs_file_map.txt)'
    )

    args = parser.parse_args()

    # Validate inputs
    if not args.modules_dir.exists():
        print(f"Error: modules directory '{args.modules_dir}' does not exist")
        return 1

    if not (args.modules_dir / 'modules.yaml').exists():
        print(f"Error: modules.yaml not found in '{args.modules_dir}'")
        return 1

    # Run the processor
    processor = ModuleProcessor(args.modules_dir, args.out_dir, args.compiler_path)
    processor.nitrofs_map_filename = args.nitrofs_map_file
    processor.generate_build_configs()

    return 0


if __name__ == '__main__':
    exit(main())
