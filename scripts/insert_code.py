#!/usr/bin/env python3

import os
import argparse
import subprocess
import struct
import ndspy.rom

def parse_arguments():
    parser = argparse.ArgumentParser(description='Insert code into Nintendo DS ROM')
    parser.add_argument('input_rom', help='Input ROM file path')
    parser.add_argument('output_rom', help='Output ROM file path')
    parser.add_argument('-l', '--language', default='en',
                       choices=['en', 'fr', 'ge', 'it', 'jp', 'sp', 'pt'],
                       help='Game language (default: en)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Enable verbose output')
    parser.add_argument('--temp-dir', default='__tmp',
                       help='Temporary directory for extracted files (default: __tmp)')

    return parser.parse_args()

args = parse_arguments()
input_rom_filename = args.input_rom
output_rom_filename = args.output_rom
language = args.language
verbose = args.verbose
outputdir = args.temp_dir
ov9dir = outputdir + "/overlay9"
ov7dir = outputdir + "/overlay7"
supported_languages = ['en', 'fr', 'ge', 'it', 'jp', 'sp', 'pt']

def run_ncp():
    print("Running NCPatcher")

    args = ["ncpatcher"]
    if verbose:
        args.append("--verbose")

    # Add the GAME_LANGUAGE macro definition
    language_upper = language.upper()
    args.extend(["--define", f"GAME_LANGUAGE_{language_upper}"])

    subprocess.run(args)

def get_header(rom):
    return rom.save(updateDeviceCapacity=True)[0:0x4000]

def get_overlays(rom, tableData):
    ovs = []
    for i in range(0, len(tableData), 32):
        (ovID, ramAddr, ramSize, bssSize, staticInitStart, staticInitEnd,
            fileID, compressedSize_Flags) = struct.unpack_from('<8I', tableData, i)
        ovs.append(fileID)
    return ovs

def unpack():
    print("Extracting the code binaries")

    if not os.path.exists(outputdir):
        os.mkdir(outputdir)
    if not os.path.exists(ov9dir):
        os.mkdir(ov9dir)
    if not os.path.exists(ov7dir):
        os.mkdir(ov7dir)

    rom = ndspy.rom.NintendoDSRom.fromFile(input_rom_filename)

    with open(outputdir + "/header.bin", 'wb') as header_file:
        header_file.write(get_header(rom))
    with open(outputdir + "/arm9.bin", 'wb') as arm9_file:
        arm9_file.write(rom.arm9)
    with open(outputdir + "/arm7.bin", 'wb') as arm7_file:
        arm7_file.write(rom.arm7)
    with open(outputdir + "/arm9ovt.bin", 'wb') as arm9ovt_file:
        arm9ovt_file.write(rom.arm9OverlayTable)
    with open(outputdir + "/arm7ovt.bin", 'wb') as arm7ovt_file:
        arm7ovt_file.write(rom.arm7OverlayTable)

    ovs9 = get_overlays(rom, rom.arm9OverlayTable)
    for i in range(0, len(ovs9)):
        fileID = ovs9[i]
        with open(ov9dir + "/overlay9_" + str(i) + ".bin", 'wb') as ov_file:
            ov_file.write(rom.files[fileID])

    ovs7 = get_overlays(rom, rom.arm7OverlayTable)
    for i in range(0, len(ovs7)):
        fileID = ovs7[i]
        with open(ov7dir + "/overlay7_" + str(i) + ".bin", 'wb') as ov_file:
            ov_file.write(rom.files[fileID])

def pack():
    print("Merging the code binaries")

    rom = ndspy.rom.NintendoDSRom.fromFile(input_rom_filename)

    with open(outputdir + "/arm9.bin", 'rb') as arm9_file:
        rom.arm9 = arm9_file.read()
    with open(outputdir + "/arm7.bin", 'rb') as arm7_file:
        rom.arm7 = arm7_file.read()
    with open(outputdir + "/arm9ovt.bin", 'rb') as arm9ovt_file:
        rom.arm9OverlayTable = arm9ovt_file.read()
    with open(outputdir + "/arm7ovt.bin", 'rb') as arm7ovt_file:
        rom.arm7OverlayTable = arm7ovt_file.read()

    ovs9 = get_overlays(rom, rom.arm9OverlayTable)
    for i in range(0, len(ovs9)):
        fileID = ovs9[i]
        with open(ov9dir + "/overlay9_" + str(i) + ".bin", 'rb') as ov_file:
            rom.files[fileID] = ov_file.read()

    ovs7 = get_overlays(rom, rom.arm7OverlayTable)
    for i in range(0, len(ovs7)):
        fileID = ovs7[i]
        with open(ov7dir + "/overlay7_" + str(i) + ".bin", 'rb') as ov_file:
            rom.files[fileID] = ov_file.read()

    rom.saveToFile(output_rom_filename, updateDeviceCapacity=True)

def main():
    print(f"Using language: {language}")
    if language not in supported_languages:
        print(f"Warning: '{language}' is not a supported language. Supported: {', '.join(supported_languages)}")

    unpack()
    run_ncp()
    pack()
    print("Done")

if __name__ == '__main__':
    main()
