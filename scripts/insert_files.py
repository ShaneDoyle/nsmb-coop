#!/usr/bin/env python3
from sys import argv
from pathlib import Path
import ndspy.rom
import ndspy.fnt
import ndspy.narc
import subprocess
from datetime import datetime

if len(argv) < 2:
    print("Missing first argument: target rom path")
    exit(0)

rom_filename = argv[1]
language = argv[2] if len(argv) >= 3 else 'en'
rom = ndspy.rom.NintendoDSRom.fromFile(rom_filename)
path_overrides_filename = 'nitrofs_overrides.txt'
newfs_dir = 'nitrofs'
supported_languages = ['en', 'fr', 'ge', 'it', 'jp', 'sp']

def collect_files_from_directory(directory, base_path_parts=1):
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
            narc_info = get_narc_info_from_path(path_formatted_)
            if narc_info:
                narc_path, file_in_narc = narc_info
                if narc_path not in narc_files:
                    narc_files[narc_path] = {}
                narc_files[narc_path][file_in_narc] = path
            else:
                regular_files[path_formatted] = path

    return regular_files, narc_files

def get_narc_info_from_path(path_parts):
    """
    Check if a path contains a _narc directory and extract narc path and internal file path.
    Returns tuple of (narc_path, file_in_narc) or None if not a narc file.
    """
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

def get_language_files():
    """
    Build dictionaries of files to insert, with language overlay support.
    Files from 'en' folder serve as base, other languages overlay on top.
    Returns tuple of (regular_files, narc_files)
    """
    regular_files = {}
    narc_files = {}

    # Check if language folders exist
    language_dirs = [Path(newfs_dir) / lang for lang in supported_languages if (Path(newfs_dir) / lang).exists()]

    if not language_dirs:
        # Fall back to original behavior if no language folders exist
        print("No language folders found, using root nitrofs directory")
        reg_files, narc_files_found = collect_files_from_directory(Path(newfs_dir), 1)
        return reg_files, narc_files_found

    # Start with English (base language)
    en_dir = Path(newfs_dir) / 'en'
    if en_dir.exists():
        print(f"Loading base files from 'en' folder")
        reg_files, narc_files_found = collect_files_from_directory(en_dir, 2)
        regular_files.update(reg_files)

        # Merge narc files
        for narc_path, files in narc_files_found.items():
            if narc_path not in narc_files:
                narc_files[narc_path] = {}
            narc_files[narc_path].update(files)

    # Overlay selected language if it's not English
    if language != 'en':
        lang_dir = Path(newfs_dir) / language
        if lang_dir.exists():
            print(f"Overlaying files from '{language}' folder")
            reg_files, narc_files_found = collect_files_from_directory(lang_dir, 2)

            # Override regular files
            for path_formatted, file_path in reg_files.items():
                if path_formatted in regular_files:
                    print(f"  Overriding {path_formatted}")
                regular_files[path_formatted] = file_path

            # Merge and override narc files
            for narc_path, files in narc_files_found.items():
                if narc_path not in narc_files:
                    narc_files[narc_path] = {}
                for file_in_narc, file_path in files.items():
                    if file_in_narc in narc_files[narc_path]:
                        print(f"  Overriding {narc_path}/{file_in_narc}")
                    narc_files[narc_path][file_in_narc] = file_path
        else:
            print(f"Warning: Language folder '{language}' not found, using English only")

    return regular_files, narc_files

def insert_file_into_narc(narc_data, file_path_in_narc, new_file_data):
    """
    Replace a file in a NARC archive.
    Only replaces existing files, does not add new files.
    Returns the modified NARC data.
    """
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

def process_narc_files(narc_files):
    """
    Process all NARC files that need to be modified.
    """
    for narc_path, files_to_insert in narc_files.items():
        print(f"Processing NARC: {narc_path}")

        # Get the current NARC file from ROM
        try:
            file_id = rom.filenames.idOf(narc_path)
            if file_id is None:
                print(f"  Warning: NARC file {narc_path} not found in ROM")
                continue

            narc_data = rom.files[file_id]

            # Insert each file into the NARC
            for file_in_narc, file_path in files_to_insert.items():
                with open(file_path, 'rb') as f:
                    new_file_data = f.read()
                narc_data = insert_file_into_narc(narc_data, file_in_narc, new_file_data)

            # Update the ROM with the modified NARC
            rom.files[file_id] = narc_data
            print(f"  Updated NARC {narc_path} [{file_id}]")

        except Exception as e:
            print(f"  Error processing NARC {narc_path}: {e}")

def insert_nitrofs():
    # Generate dictionary from filename overrides file
    path_overrides = {}
    if Path(path_overrides_filename).exists():
        with open(path_overrides_filename, 'r') as path_overrides_file:
            for line in path_overrides_file:
                line_interpreted = line.split('\n')[0].split(sep=',')
                path_overrides[line_interpreted[0]] = int(line_interpreted[1])

    # Get files to insert with language overlay
    files_to_insert, narc_files = get_language_files()

    # Process NARC files first
    if narc_files:
        print("Processing NARC files:")
        process_narc_files(narc_files)

    # Insert regular files into ROM
    print("Processing regular files:")
    for path_formatted, file_path in files_to_insert.items():
        path_formatted_ = Path(path_formatted)
        with open(file_path, 'rb') as extracted_file:
            file_data = extracted_file.read()

            if path_formatted in path_overrides:
                # Get file ID
                file_id = path_overrides[path_formatted]

                # Rename file
                sub_folder = rom.filenames.subfolder(path_formatted_.parent.as_posix())
                old_filename = sub_folder.files[file_id - sub_folder.firstID]
                sub_folder.files[file_id - sub_folder.firstID] = path_formatted_.name

                # Set file data
                rom.files[file_id] = file_data

                print(f'Replaced file {old_filename} as {path_formatted} [{file_id}]')

            elif path_formatted != 'banner.bin':
                file_id = rom.filenames.idOf(path_formatted)

                if file_id is not None:
                    # Set file data
                    rom.files[file_id] = file_data

                    print(f'Replaced file {path_formatted} [{file_id}]')
                else:
                    file_id = len(rom.files)

                    sub_folder_name = path_formatted_.parent.as_posix()
                    sub_folder = rom.filenames.subfolder(sub_folder_name)

                    # Create the subfolder if it doesn't exist
                    if sub_folder is None:
                        sub_folder = ndspy.fnt.Folder(firstID=file_id)
                        rom.filenames.folders.append((sub_folder_name, sub_folder))

                    # Add the file
                    sub_folder.files.append(path_formatted_.name)
                    rom.files.append(file_data)

                    print(f'Inserted file {path_formatted} [{file_id}]')

def insert_banner():
    rom.iconBanner = open(newfs_dir + '/banner.bin', 'rb').read() # Install banner
    print('Replaced banner')

def get_git_revision_short_hash():
    try:
        return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).strip().decode('utf-8')
    except subprocess.CalledProcessError:
        return 'unknown'

def get_git_commit_date():
    try:
        # Get commit date in UTC (Z timezone)
        iso_date = subprocess.check_output(['git', 'log', '-1', '--format=%cI']).strip().decode('utf-8')
        # Parse and format to ensure Z timezone
        dt = datetime.fromisoformat(iso_date.replace('Z', '+00:00'))
        return dt.strftime('%Y-%m-%dT%H:%M:%SZ')
    except subprocess.CalledProcessError:
        return datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

def insert_buildtime():
    short_hash = get_git_revision_short_hash()
    commit_date = get_git_commit_date()
    buildtime = f'{short_hash} {commit_date}'
    rom.setFileByName('BUILDTIME', bytearray(buildtime, 'utf-8'))
    print(f'Written build time: {buildtime}')

def main():
    print(f"Using language: {language}")
    if language not in supported_languages:
        print(f"Warning: '{language}' is not a supported language. Supported: {', '.join(supported_languages)}")

    insert_nitrofs()
    insert_banner()
    insert_buildtime()
    rom.saveToFile(rom_filename)
    print('Done inserting files')

if __name__ == '__main__':
    main()
