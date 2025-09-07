#!/usr/bin/env python3
"""
Nintendo DS ROM File Insertion Tool

This tool inserts files into a Nintendo DS ROM with support for:
- Language overlay system (English as base, other languages override)
- NARC file modification
- New file insertion (z_new folder)
- Banner.bin replacement
- Build information injection
- File ID header generation

All files must be organized under language directories. The English version serves as
the base for all languages, then language-specific files can override it.
"""

import argparse
import os
import subprocess
from datetime import datetime
from pathlib import Path
from typing import Dict, Optional, Tuple, Union

import ndspy.fnt
import ndspy.narc
import ndspy.rom


class FileMapLoader:
    """Handles loading and parsing of nitrofs file mapping files."""

    def __init__(self, map_file_path: str):
        self.map_file_path = map_file_path
        self.file_map: Dict[str, str] = {}

    def load(self) -> Dict[str, str]:
        """Load file mappings from the map file."""
        if not Path(self.map_file_path).exists():
            print(f"Warning: Nitrofs map file '{self.map_file_path}' not found")
            return {}

        print(f"Loading nitrofs file mapping from: {self.map_file_path}")

        with open(self.map_file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()

                # Skip empty lines and comments
                if not line or line.startswith('#'):
                    continue

                # Parse line format: relative_path=absolute_source_path
                if '=' not in line:
                    print(f"Warning: Invalid format in {self.map_file_path}:{line_num}: {line}")
                    continue

                rel_path, source_path = line.split('=', 1)
                self.file_map[rel_path.strip()] = source_path.strip()

        print(f"  Loaded {len(self.file_map)} file mappings")
        return self.file_map


class LanguageFileResolver:
    """Resolves files with language overlay logic."""

    SUPPORTED_LANGUAGES = ['en', 'fr', 'ge', 'it', 'jp', 'sp', 'pt', 'ko', 'ch']

    def __init__(self, language: str, nitrofs_dir: str = 'nitrofs'):
        self.language = language
        self.nitrofs_dir = nitrofs_dir

    def resolve_files_from_map(self, file_map: Dict[str, str]) -> Tuple[Dict[str, str], Dict[str, Dict[str, str]], Optional[str]]:
        """
        Resolve files using the nitrofs file map with language overlay.

        Returns:
            (regular_files, narc_files, banner_path)
        """
        regular_files = {}
        narc_files = {}
        banner_path = None

        # Group files by language
        en_files = {}
        lang_files = {}

        for rel_path, source_path in file_map.items():
            path_parts = Path(rel_path).parts

            # Handle banner.bin specially
            if Path(rel_path).name == 'banner.bin':
                if len(path_parts) > 0 and path_parts[0] in self.SUPPORTED_LANGUAGES:
                    lang_prefix = path_parts[0]
                    if lang_prefix == 'en':
                        # English banner is always the base
                        banner_path = source_path
                        print('Found English banner (base)')
                    elif lang_prefix == self.language and self.language != 'en':
                        # Override with language-specific banner
                        banner_path = source_path
                        print(f'Overriding with {self.language} banner')
                continue

            # Check if this file is inside a _narc directory
            narc_info = self._get_narc_info_from_path(Path(rel_path))

            if narc_info:
                narc_path, file_in_narc = narc_info

                # Determine if this is a language-specific file
                if len(path_parts) > 0 and path_parts[0] in self.SUPPORTED_LANGUAGES:
                    lang_prefix = path_parts[0]
                    # Remove language prefix from narc path
                    narc_path_without_lang = '/'.join(Path(narc_path).parts[1:])

                    if lang_prefix == 'en':
                        if narc_path_without_lang not in en_files:
                            en_files[narc_path_without_lang] = ('narc', {})
                        en_files[narc_path_without_lang][1][file_in_narc] = source_path
                    elif lang_prefix == self.language:
                        if narc_path_without_lang not in lang_files:
                            lang_files[narc_path_without_lang] = ('narc', {})
                        lang_files[narc_path_without_lang][1][file_in_narc] = source_path
            else:
                # Regular file
                if len(path_parts) > 0 and path_parts[0] in self.SUPPORTED_LANGUAGES:
                    lang_prefix = path_parts[0]
                    # Remove language prefix
                    file_path_without_lang = '/'.join(path_parts[1:])

                    if lang_prefix == 'en':
                        en_files[file_path_without_lang] = ('regular', source_path)
                    elif lang_prefix == self.language:
                        lang_files[file_path_without_lang] = ('regular', source_path)

        # Apply English base files
        for file_path, file_info in en_files.items():
            file_type, file_data = file_info
            if file_type == 'regular':
                regular_files[file_path] = file_data
            elif file_type == 'narc':
                if file_path not in narc_files:
                    narc_files[file_path] = {}
                narc_files[file_path].update(file_data)

        # Overlay selected language files (if different from English)
        if self.language != 'en':
            for file_path, file_info in lang_files.items():
                file_type, file_data = file_info
                if file_type == 'regular':
                    if file_path in regular_files:
                        print(f"  Overriding {file_path}")
                    regular_files[file_path] = file_data
                elif file_type == 'narc':
                    if file_path not in narc_files:
                        narc_files[file_path] = {}
                    for file_in_narc, source_path in file_data.items():
                        if file_in_narc in narc_files[file_path]:
                            print(f"  Overriding {file_path}/{file_in_narc}")
                        narc_files[file_path][file_in_narc] = source_path

        return regular_files, narc_files, banner_path

    def resolve_files_from_directory(self) -> Tuple[Dict[str, str], Dict[str, Dict[str, str]], Optional[str]]:
        """
        Resolve files by scanning the nitrofs directory with language overlay.
        All files are expected to be under language directories.

        Returns:
            (regular_files, narc_files, banner_path)
        """
        regular_files = {}
        narc_files = {}
        banner_path = None

        # Check if language folders exist
        language_dirs = [Path(self.nitrofs_dir) / lang for lang in self.SUPPORTED_LANGUAGES if (Path(self.nitrofs_dir) / lang).exists()]

        # Group files by language
        en_files = {}
        lang_files = {}

        # Start with English (base language)
        en_dir = Path(self.nitrofs_dir) / 'en'
        if en_dir.exists():
            print(f"Loading base files from 'en' folder")
            reg_files, narc_files_found = self._collect_files_from_directory(en_dir, 2)

            # Convert to the expected format
            for file_path, source_path in reg_files.items():
                en_files[file_path] = ('regular', str(source_path))
            for narc_path, files in narc_files_found.items():
                en_files[narc_path] = ('narc', {file_in_narc: str(source_path) for file_in_narc, source_path in files.items()})

            # Check for English banner
            en_banner = en_dir / 'banner.bin'
            if en_banner.exists():
                banner_path = str(en_banner)

        # Overlay selected language if it's not English
        if self.language != 'en':
            lang_dir = Path(self.nitrofs_dir) / self.language
            if lang_dir.exists():
                print(f"Overlaying files from '{self.language}' folder")
                reg_files, narc_files_found = self._collect_files_from_directory(lang_dir, 2)

                # Convert to the expected format
                for file_path, source_path in reg_files.items():
                    lang_files[file_path] = ('regular', str(source_path))
                for narc_path, files in narc_files_found.items():
                    lang_files[narc_path] = ('narc', {file_in_narc: str(source_path) for file_in_narc, source_path in files.items()})

                # Check for language-specific banner
                lang_banner = lang_dir / 'banner.bin'
                if lang_banner.exists():
                    banner_path = str(lang_banner)
                    print(f'Overriding with {self.language} banner')
            else:
                print(f"Warning: Language folder '{self.language}' not found, using English only")

        # Apply English base files
        for file_path, file_info in en_files.items():
            file_type, file_data = file_info
            if file_type == 'regular':
                regular_files[file_path] = file_data
            elif file_type == 'narc':
                if file_path not in narc_files:
                    narc_files[file_path] = {}
                narc_files[file_path].update(file_data)

        # Overlay selected language files (if different from English)
        if self.language != 'en':
            for file_path, file_info in lang_files.items():
                file_type, file_data = file_info
                if file_type == 'regular':
                    if file_path in regular_files:
                        print(f"  Overriding {file_path}")
                    regular_files[file_path] = file_data
                elif file_type == 'narc':
                    if file_path not in narc_files:
                        narc_files[file_path] = {}
                    for file_in_narc, source_path in file_data.items():
                        if file_in_narc in narc_files[file_path]:
                            print(f"  Overriding {file_path}/{file_in_narc}")
                        narc_files[file_path][file_in_narc] = source_path

        return regular_files, narc_files, banner_path

    def _collect_files_from_directory(self, directory: Path, base_path_parts: int = 1) -> Tuple[Dict[str, str], Dict[str, Dict[str, str]]]:
        """
        Collect files from a directory, separating regular files and narc files.
        Returns tuple of (regular_files, narc_files)
        """
        regular_files = {}
        narc_files = {}

        for path in directory.rglob('*'):
            if path.is_file():
                # Remove the base path parts (e.g., 'nitrofs', 'en')
                path_formatted_ = Path(*path.parts[base_path_parts:])
                path_formatted = path_formatted_.as_posix()

                # Check if this file is inside a _narc directory
                narc_info = self._get_narc_info_from_path(path_formatted_)
                if narc_info:
                    narc_path, file_in_narc = narc_info
                    if narc_path not in narc_files:
                        narc_files[narc_path] = {}
                    narc_files[narc_path][file_in_narc] = str(path)
                else:
                    regular_files[path_formatted] = str(path)

        return regular_files, narc_files

    def _get_narc_info_from_path(self, path_parts: Path) -> Optional[Tuple[str, str]]:
        """Check if a path contains a _narc directory and extract info."""
        parts = list(path_parts.parts)

        # Find the first _narc directory in the path
        for i, part in enumerate(parts):
            if part.endswith('_narc'):
                narc_name = part[:-5]  # Remove '_narc' suffix

                # Build the narc path (everything before _narc + narc extension)
                narc_path_parts = parts[:i] + [narc_name + '.narc']
                narc_path = '/'.join(narc_path_parts)

                # Build the file path inside the narc
                file_in_narc_parts = parts[i + 1:]
                file_in_narc = '/'.join(file_in_narc_parts)

                return narc_path, file_in_narc

        return None


class NarcProcessor:
    """Handles NARC archive file modifications."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def process_narc_files(self, narc_files: Dict[str, Dict[str, str]]) -> None:
        """Process all NARC files that need to be modified."""
        for narc_path, files_to_insert in narc_files.items():
            print(f"Processing NARC: {narc_path}")

            try:
                file_id = self.rom.filenames.idOf(narc_path)
                if file_id is None:
                    print(f"  Warning: NARC file {narc_path} not found in ROM")
                    continue

                narc_data = self.rom.files[file_id]

                # Insert each file into the NARC
                for file_in_narc, file_path in files_to_insert.items():
                    with open(file_path, 'rb') as f:
                        new_file_data = f.read()
                    narc_data = self._insert_file_into_narc(narc_data, file_in_narc, new_file_data)

                # Update the ROM with the modified NARC
                self.rom.files[file_id] = narc_data
                print(f"  Updated NARC {narc_path} [{file_id}]")

            except Exception as e:
                print(f"  Error processing NARC {narc_path}: {e}")

    def _insert_file_into_narc(self, narc_data: bytes, file_path_in_narc: str, new_file_data: bytes) -> bytes:
        """Replace a file in a NARC archive."""
        try:
            narc = ndspy.narc.NARC(narc_data)

            # Try to find existing file
            file_id = narc.filenames.idOf(file_path_in_narc)
            if file_id is not None:
                narc.files[file_id] = new_file_data
                print(f"    Replaced file {file_path_in_narc} in NARC")
                return narc.save()
            else:
                print(f"    Warning: File {file_path_in_narc} not found in NARC, skipping")
                return narc_data

        except Exception as e:
            print(f"    Error modifying NARC: {e}")
            return narc_data


class FileInserter:
    """Handles regular file insertions and replacements."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def insert_regular_files(self, regular_files: Dict[str, str], path_overrides: Dict[str, int]) -> None:
        """Insert regular files into the ROM."""
        print("Processing regular files:")

        # Sort regular files alphabetically for consistent ordering
        for path_formatted, file_path in sorted(regular_files.items()):
            path_formatted_ = Path(path_formatted)

            with open(file_path, 'rb') as f:
                file_data = f.read()

            if path_formatted in path_overrides:
                # Get file ID from overrides
                file_id = path_overrides[path_formatted]

                # Rename file
                sub_folder = self.rom.filenames.subfolder(path_formatted_.parent.as_posix())
                old_filename = sub_folder.files[file_id - sub_folder.firstID]
                sub_folder.files[file_id - sub_folder.firstID] = path_formatted_.name

                # Set file data
                self.rom.files[file_id] = file_data

                print(f'Replaced file {old_filename} as {path_formatted} [{file_id}]')

            else:
                file_id = self.rom.filenames.idOf(path_formatted)

                if file_id is not None:
                    # Set file data
                    self.rom.files[file_id] = file_data
                    print(f'Replaced file {path_formatted} [{file_id}]')
                else:
                    # File doesn't exist in ROM and it's not in z_new - stop the build
                    print(f'ERROR: File {path_formatted} does not exist in ROM and is not in z_new folder')
                    print('Build stopped. Files must either exist in ROM or be placed in z_new folder.')
                    exit(1)


class ZNewFileProcessor:
    """Handles insertion of new files in the z_new folder."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def process_z_new_files(self, z_new_files: Dict[str, str]) -> None:
        """Process z_new files with proper folder management."""
        if not z_new_files:
            return

        print("Processing z_new files:")

        # Insert the reserved file first to establish the z_new folder
        self._ensure_reserved_file()

        # Group files by their containing folder
        folder_files = self._group_files_by_folder(z_new_files)

        # Process each folder and its files
        for folder_path in sorted(folder_files.keys(), key=str.lower):
            self._process_folder(folder_path, folder_files[folder_path])

    def _ensure_reserved_file(self) -> None:
        """Ensure the reserved file exists in z_new folder."""
        reserved_file_id = len(self.rom.files)

        # Ensure z_new folder exists
        z_new_folder = self.rom.filenames.subfolder('z_new')
        if z_new_folder is None:
            # Create the z_new folder
            z_new_folder = ndspy.fnt.Folder(firstID=reserved_file_id)
            self.rom.filenames.folders.append(('z_new', z_new_folder))

        # Update the folder's firstID if it has no files yet
        if len(z_new_folder.files) == 0:
            z_new_folder.firstID = reserved_file_id

        # Insert the reserved file (0 bytes)
        z_new_folder.files.append('reserved')
        self.rom.files.append(b'')  # 0-byte file
        print(f'Inserted reserved file z_new/reserved [{reserved_file_id}]')

    def _group_files_by_folder(self, z_new_files: Dict[str, str]) -> Dict[str, list]:
        """Group files by their containing folder."""
        folder_files = {}
        for path_formatted, file_path in z_new_files.items():
            path_formatted_ = Path(path_formatted)
            folder_path = path_formatted_.parent.as_posix()
            if folder_path not in folder_files:
                folder_files[folder_path] = []
            folder_files[folder_path].append((path_formatted, file_path))
        return folder_files

    def _process_folder(self, folder_path: str, files_in_folder: list) -> None:
        """Process files in a specific folder."""
        # Sort files alphabetically (except reserved file should come first in z_new)
        if folder_path == 'z_new':
            # Separate reserved file from others
            reserved_files = [(p, f) for p, f in files_in_folder if Path(p).name == 'reserved']
            other_files = [(p, f) for p, f in files_in_folder if Path(p).name != 'reserved']
            # Sort other files alphabetically (case-insensitive)
            other_files.sort(key=lambda x: Path(x[0]).name.lower())
            # Reserved files first, then alphabetically sorted files
            files_in_folder = reserved_files + other_files
        else:
            # For other folders, just sort alphabetically (case-insensitive)
            files_in_folder.sort(key=lambda x: Path(x[0]).name.lower())

        # Ensure the folder exists
        sub_folder = self._ensure_folder_exists(folder_path)

        if sub_folder is not None:
            # For z_new folder, don't update firstID since we already set it when adding reserved file
            if folder_path != 'z_new' and len(sub_folder.files) == 0:
                sub_folder.firstID = len(self.rom.files)

            # Add all files in this folder
            for path_formatted, file_path in files_in_folder:
                self._insert_file_in_folder(path_formatted, file_path, sub_folder)

    def _ensure_folder_exists(self, folder_path: str) -> Optional[ndspy.fnt.Folder]:
        """Ensure a folder exists in the ROM, creating it if necessary."""
        sub_folder = self.rom.filenames.subfolder(folder_path)
        if sub_folder is not None:
            return sub_folder

        # Create all parent folders in the path if they don't exist
        path_parts = Path(folder_path).parts

        # Build folders from root down to target (in alphabetical order)
        for i in range(len(path_parts)):
            partial_path = '/'.join(path_parts[:i+1])

            # Check if this folder already exists
            existing_folder = self.rom.filenames.subfolder(partial_path)
            if existing_folder is None:
                # Create the folder - firstID will be set when we add the first file
                new_folder = ndspy.fnt.Folder(firstID=len(self.rom.files))

                if i == 0:
                    # This is a top-level folder, add to root
                    self.rom.filenames.folders.append((path_parts[i], new_folder))
                    # Sort folders to maintain alphabetical order (case-insensitive)
                    self.rom.filenames.folders.sort(key=lambda x: x[0].lower())
                else:
                    # This is a subfolder, add to its parent folder
                    parent_path = '/'.join(path_parts[:i])
                    parent_folder = self.rom.filenames.subfolder(parent_path)
                    if parent_folder is not None:
                        parent_folder.folders.append((path_parts[i], new_folder))
                        # Sort subfolders to maintain alphabetical order (case-insensitive)
                        parent_folder.folders.sort(key=lambda x: x[0].lower())
                    else:
                        print(f"Warning: Could not find parent folder '{parent_path}' for subfolder '{path_parts[i]}'")

        # Get the final subfolder after creation
        return self.rom.filenames.subfolder(folder_path)

    def _insert_file_in_folder(self, path_formatted: str, file_path: str, sub_folder: ndspy.fnt.Folder) -> None:
        """Insert a single file into a folder."""
        path_formatted_ = Path(path_formatted)

        with open(file_path, 'rb') as f:
            file_data = f.read()

        file_id = len(self.rom.files)
        sub_folder.files.append(path_formatted_.name)
        self.rom.files.append(file_data)

        print(f'Inserted new file {path_formatted} [{file_id}]')


class BannerProcessor:
    """Handles banner.bin replacement."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def insert_banner(self, banner_path: Optional[str]) -> None:
        """Insert banner.bin into the ROM."""
        if banner_path and Path(banner_path).exists():
            with open(banner_path, 'rb') as f:
                self.rom.iconBanner = f.read()
            print(f'Replaced banner from {banner_path}')
        else:
            print('Warning: banner.bin not found')


class BuildInfoProcessor:
    """Handles build information injection."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def insert_buildtime(self) -> None:
        """Insert build time information into the ROM."""
        short_hash = self._get_git_revision_short_hash()
        commit_date = self._get_git_commit_date()
        buildtime = f'{short_hash} {commit_date}'
        self.rom.setFileByName('BUILDTIME', bytearray(buildtime, 'utf-8'))
        print(f'Written build time: {buildtime}')

    def _get_git_revision_short_hash(self) -> str:
        """Get the short git hash of the current commit."""
        try:
            return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).strip().decode('utf-8')
        except subprocess.CalledProcessError:
            return 'unknown'

    def _get_git_commit_date(self) -> str:
        """Get the commit date of the current commit."""
        try:
            # Get commit date in UTC (Z timezone)
            iso_date = subprocess.check_output(['git', 'log', '-1', '--format=%cI']).strip().decode('utf-8')
            # Parse and format to ensure Z timezone
            dt = datetime.fromisoformat(iso_date.replace('Z', '+00:00'))
            return dt.strftime('%Y-%m-%dT%H:%M:%SZ')
        except subprocess.CalledProcessError:
            return datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')


class FidGenerator:
    """Generates fid.hpp header file with file IDs."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def generate_fid_hpp(self, output_path: str) -> None:
        """Generate constexpr header file with file IDs using string literal format."""
        print("Generating fid.hpp with all ROM file IDs...")

        # Ensure the output directory exists
        os.makedirs(os.path.dirname(output_path), exist_ok=True)

        # Collect all files
        all_files = {}
        for file_id in range(len(self.rom.files)):
            filename = self.rom.filenames.filenameOf(file_id)
            if filename is not None:
                # For z_new files, remove the z_new/ prefix in the generated header
                display_filename = filename
                if filename.startswith('z_new/'):
                    display_filename = filename[6:]  # Remove 'z_new/' prefix
                all_files[display_filename] = file_id

        if not all_files:
            print("No file IDs found in ROM, skipping fid.hpp generation")
            return

        print(f"Found {len(all_files)} files in ROM")

        # Generate header content
        header_content = self._generate_header_content(all_files)

        # Write the header file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(header_content)

        print(f"Generated fid.hpp: {output_path}")

    def _generate_header_content(self, all_files: Dict[str, int]) -> str:
        """Generate the header file content."""
        header_content = """#pragma once

#include <cstdint>
#include <string_view>

consteval std::uint16_t operator""fid(const char* str, std::size_t len) {
    std::string_view path{str, len};

"""

        # Sort by file ID for consistent output
        sorted_files = sorted(all_files.items(), key=lambda x: x[1])

        # Add all file paths with their adjusted IDs
        for file_path, file_id in sorted_files:
            adjusted_id = file_id - 131  # Subtract 131 as requested
            header_content += f'    if (path == "{file_path}") return {adjusted_id};\n'

        header_content += "}\n"
        return header_content


class DebugPrinter:
    """Handles debug output and file listing."""

    def __init__(self, rom: ndspy.rom.NintendoDSRom):
        self.rom = rom

    def print_z_new_files(self) -> None:
        """Print the files in the z_new directory."""
        z_new_folder = self.rom.filenames.subfolder('z_new')
        if z_new_folder:
            print("\n=== z_new directory contents ===")
            print(f"z_new folder firstID: {z_new_folder.firstID}")
            self._print_folder_contents(z_new_folder)
            print("=== End z_new directory ===\n")
        else:
            print("z_new directory not found in ROM")

    def _print_folder_contents(self, folder: ndspy.fnt.Folder, path: str = "", indent: str = "  ") -> None:
        """Recursively print folder contents."""
        if folder:
            for i, filename in enumerate(folder.files):
                file_id = folder.firstID + i
                full_path = f"{path}/{filename}" if path else filename
                print(f"{indent}[{file_id}] {full_path}")

            # Print subfolders recursively
            for folder_name, subfolder in folder.folders:
                subfolder_path = f"{path}/{folder_name}" if path else folder_name
                print(f"{indent}{subfolder_path}/")
                self._print_folder_contents(subfolder, subfolder_path, indent + "  ")


class RomFileInserter:
    """Main class that orchestrates the file insertion process."""

    def __init__(self, input_rom: str, output_rom: str, language: str = 'en', nitrofs_dir: str = 'nitrofs'):
        self.input_rom = input_rom
        self.output_rom = output_rom
        self.language = language
        self.nitrofs_dir = nitrofs_dir
        self.rom = ndspy.rom.NintendoDSRom.fromFile(input_rom)

        # Initialize processors
        self.language_resolver = LanguageFileResolver(language, nitrofs_dir)
        self.narc_processor = NarcProcessor(self.rom)
        self.file_inserter = FileInserter(self.rom)
        self.z_new_processor = ZNewFileProcessor(self.rom)
        self.banner_processor = BannerProcessor(self.rom)
        self.build_info_processor = BuildInfoProcessor(self.rom)
        self.fid_generator = FidGenerator(self.rom)
        self.debug_printer = DebugPrinter(self.rom)

    def insert_files(self, nitrofs_map: Optional[str] = None,
                     overrides_file: Optional[str] = None,
                     fid_output: Optional[str] = None) -> None:
        """Main method to insert all files into the ROM."""
        print(f"Using language: {self.language}")

        # Load path overrides
        path_overrides = self._load_path_overrides(overrides_file)

        # Get files to insert
        if nitrofs_map and Path(nitrofs_map).exists():
            print("Using nitrofs file mapping")
            file_map_loader = FileMapLoader(nitrofs_map)
            file_map = file_map_loader.load()
            regular_files, narc_files, banner_path = self.language_resolver.resolve_files_from_map(file_map)
        else:
            print("Using directory scanning")
            regular_files, narc_files, banner_path = self.language_resolver.resolve_files_from_directory()

        # Separate z_new files from regular files
        z_new_files, regular_files = self._separate_z_new_files(regular_files)

        # Process files
        if narc_files:
            print("Processing NARC files:")
            self.narc_processor.process_narc_files(narc_files)

        self.file_inserter.insert_regular_files(regular_files, path_overrides)
        self.z_new_processor.process_z_new_files(z_new_files)
        self.banner_processor.insert_banner(banner_path)
        self.build_info_processor.insert_buildtime()

        if fid_output:
            self.fid_generator.generate_fid_hpp(fid_output)

        # Debug output
        self.debug_printer.print_z_new_files()

        # Save ROM
        self.rom.saveToFile(self.output_rom)
        print('Done inserting files')

    def _load_path_overrides(self, overrides_file: Optional[str]) -> Dict[str, int]:
        """Load path overrides from file."""
        path_overrides = {}
        if overrides_file and Path(overrides_file).exists():
            with open(overrides_file, 'r') as f:
                for line in f:
                    line_interpreted = line.split('\n')[0].split(sep=',')
                    if len(line_interpreted) >= 2:
                        path_overrides[line_interpreted[0]] = int(line_interpreted[1])
        return path_overrides

    def _separate_z_new_files(self, regular_files: Dict[str, str]) -> Tuple[Dict[str, str], Dict[str, str]]:
        """Separate z_new files from regular files."""
        z_new_files = {}
        remaining_files = {}

        for path_formatted, file_path in regular_files.items():
            if path_formatted.startswith('z_new/'):
                z_new_files[path_formatted] = file_path
            else:
                remaining_files[path_formatted] = file_path

        return z_new_files, remaining_files


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='Insert files into Nintendo DS ROM')
    parser.add_argument('input_rom', help='Input ROM file path')
    parser.add_argument('output_rom', help='Output ROM file path')
    parser.add_argument('-l', '--language', default='en',
                       choices=['en', 'fr', 'ge', 'it', 'jp', 'sp', 'pt', 'ko', 'ch'],
                       help='Game language (default: en)')
    parser.add_argument('--nitrofs-dir', default='nitrofs',
                       help='Directory containing files to insert (default: nitrofs)')
    parser.add_argument('--overrides-file', default='nitrofs_overrides.txt',
                       help='Path overrides file (default: nitrofs_overrides.txt)')
    parser.add_argument('--fid-output', default=None,
                       help='Output path for constexpr fid.hpp file')
    parser.add_argument('--nitrofs-map', default=None,
                       help='Path to nitrofs file mapping generated by module_gen.py')

    return parser.parse_args()


def main():
    """Main entry point."""
    args = parse_arguments()

    inserter = RomFileInserter(
        input_rom=args.input_rom,
        output_rom=args.output_rom,
        language=args.language,
        nitrofs_dir=args.nitrofs_dir
    )

    inserter.insert_files(
        nitrofs_map=args.nitrofs_map,
        overrides_file=args.overrides_file,
        fid_output=args.fid_output
    )


if __name__ == '__main__':
    main()
