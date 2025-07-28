#!/usr/bin/env python3
from sys import argv
from pathlib import Path
import ndspy.rom
import ndspy.fnt
import subprocess
from datetime import datetime

if len(argv) < 2:
    print("Missing first argument: target rom path")
    exit(0)

rom_filename = argv[1]
rom = ndspy.rom.NintendoDSRom.fromFile(rom_filename)
path_overrides_filename = 'nitrofs_overrides.txt'
newfs_dir = 'nitrofs'

def insert_nitrofs():
    #Generate dictionary from filename overrides file
    path_overrides = {}
    if Path(path_overrides_filename).exists():
        with open(path_overrides_filename, 'r') as path_overrides_file:
            for line in path_overrides_file:
                line_interpreted = line.split('\n')[0].split(sep=',')
                path_overrides[line_interpreted[0]] = int(line_interpreted[1])

    #Insert files into ROM
    for path in Path(newfs_dir).rglob('*.*'):
        path_formatted_ = Path(*path.parts[1:])
        path_formatted = path_formatted_.as_posix()
        with open(path, 'rb') as extracted_file:
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
    insert_nitrofs()
    insert_banner()
    insert_buildtime()
    rom.saveToFile(rom_filename)
    print('Done inserting files')

if __name__ == '__main__':
    main()
