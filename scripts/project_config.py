#!/usr/bin/env python3
"""
Project configuration utility for NSMB projects.

This module provides the project name and language mappings used by the Python
ROM preparation scripts. NCPatcher owns module configuration.
"""

import yaml
from pathlib import Path
from typing import Dict, List, Optional


class ProjectConfig:
    """Handles project configuration from project.yaml."""

    def __init__(self, project_root: Optional[Path] = None):
        """Initialize project config.

        Args:
            project_root: Root directory of the project. If None, uses current working directory.
        """
        if project_root is None:
            project_root = Path.cwd()

        self.project_root = Path(project_root)
        self.config_file = self.project_root / 'project.yaml'
        self._config_data = None

    def _load_config(self) -> dict:
        """Load project configuration from project.yaml."""
        if self._config_data is not None:
            return self._config_data

        if not self.config_file.exists():
            raise FileNotFoundError(f"Project configuration file {self.config_file} not found")

        try:
            with self.config_file.open('r', encoding='utf-8') as f:
                self._config_data = yaml.safe_load(f) or {}
        except Exception as e:
            raise RuntimeError(f"Error reading {self.config_file}: {e}")

        return self._config_data

    @property
    def project_name(self) -> str:
        """Get project name."""
        config = self._load_config()
        return config.get('name', 'NSMB-Project')

    @property
    def languages(self) -> Dict[str, str]:
        """Get available languages from project.yaml.

        Returns:
            Dictionary mapping language codes to language names.
        """
        config = self._load_config()
        return config.get('languages', {})

    @property
    def language_codes(self) -> List[str]:
        """Get list of available language codes from project.yaml."""
        return list(self.languages.keys())

    def validate_language_code(self, code: str) -> bool:
        """Validate if a language code is supported.

        Args:
            code: Language code to validate

        Returns:
            True if the language code is valid
        """
        return code in self.language_codes

    def get_language_name(self, code: str) -> Optional[str]:
        """Get human-readable language name.

        Args:
            code: Language code

        Returns:
            Language name or None if code is invalid
        """
        return self.languages.get(code)

def get_project_config(project_root: Optional[Path] = None) -> ProjectConfig:
    """Get project configuration instance.

    Args:
        project_root: Root directory of the project. If None, uses current working directory.

    Returns:
        ProjectConfig instance
    """
    return ProjectConfig(project_root)


def find_project_root() -> Optional[Path]:
    """Find project root by looking for project.yaml in current directory or parent directories.

    Returns:
        Path to project root or None if not found
    """
    current = Path.cwd()

    # Check current directory and parents
    for path in [current] + list(current.parents):
        if (path / 'project.yaml').exists():
            return path

    return None


# Convenience function for scripts
def get_script_project_config() -> ProjectConfig:
    """Get project configuration for scripts, trying to find project root automatically.

    Returns:
        ProjectConfig instance
    """
    # Try to find project root
    project_root = find_project_root()

    if project_root is None:
        # Fall back to parent directory of scripts directory
        script_dir = Path(__file__).parent
        project_root = script_dir.parent

    return ProjectConfig(project_root)


if __name__ == '__main__':
    # Print the configuration
    config = get_script_project_config()

    print(f"Project: {config.project_name}")
    print()

    print("Languages:")
    for code, name in config.languages.items():
        print(f"  {code}: {name}")
