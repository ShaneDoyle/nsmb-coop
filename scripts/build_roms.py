#!/usr/bin/env python3

import os
import argparse
import subprocess
import shutil
from pathlib import Path
import sys

def parse_arguments():
    parser = argparse.ArgumentParser(description='Build many Nintendo DS ROMs')
    parser.add_argument('input_rom', help='Input original ROM file path')
    parser.add_argument('-o', '--output-dir', default='build',
                       help='Output directory for generated ROMs (default: build)')
    parser.add_argument('-l', '--languages', nargs='+',
                       choices=['en', 'fr', 'ge', 'it', 'jp', 'sp'],
                       default=['en', 'fr', 'ge', 'it', 'jp', 'sp'],
                       help='Languages to build (default: all)')
    parser.add_argument('-p', '--prefix', default=None,
                       help='Name prefix for output files (default: use input ROM stem)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Enable verbose output')
    parser.add_argument('--no-patches', action='store_true',
                       help='Skip xdelta patch generation')
    parser.add_argument('--temp-dir', default='__tmp',
                       help='Temporary directory for intermediate files')
    parser.add_argument('--clean-temp', action='store_true',
                       help='Clean temporary directory after build')

    return parser.parse_args()

def run_command(cmd, cwd=None, check=True):
    """Run a command and optionally print it"""
    if args.verbose:
        print(f"Running: {' '.join(cmd)}")

    # Always let stdout/stderr flow to terminal to prevent hanging
    # Tools like ncpatcher may expect live stdout
    result = subprocess.run(cmd, cwd=cwd, text=True)

    if check and result.returncode != 0:
        print(f"Command failed with return code {result.returncode}")
        sys.exit(1)

    return result

def check_dependencies():
    """Check if required tools are available"""
    print("Checking dependencies...")

    # Check for xdelta3 if patch generation is enabled
    if not args.no_patches:
        try:
            run_command(['xdelta3', '-h'], check=False)
        except FileNotFoundError:
            print("Warning: xdelta3 not found. Patch generation will be skipped.")
            args.no_patches = True

    # Check for ncpatcher
    try:
        run_command(['ncpatcher', '--help'], check=False)
    except FileNotFoundError:
        print("Error: ncpatcher not found. Please ensure it's installed and in PATH.")
        sys.exit(1)

def ensure_directory(path):
    """Create directory if it doesn't exist"""
    Path(path).mkdir(parents=True, exist_ok=True)

def build_rom_for_language(language, base_name):
    """Build ROM for a specific language"""
    print(f"\n=== Building ROM for language: {language} ===")

    # Define subdirectories
    nds_dir = os.path.join(args.output_dir, 'nds')
    xdelta_dir = os.path.join(args.output_dir, 'xdelta')

    # Ensure subdirectories exist
    ensure_directory(nds_dir)
    if not args.no_patches:
        ensure_directory(xdelta_dir)

    # Define file paths
    code_rom = os.path.join(args.temp_dir, f"{base_name}_code_{language}.nds")
    final_rom = os.path.join(nds_dir, f"{base_name}_{language}.nds")
    patch_file = os.path.join(xdelta_dir, f"{base_name}_{language}.xdelta")

    # Step 1: Insert code
    print(f"Step 1: Inserting code for {language}...")
    insert_code_cmd = [
        sys.executable, 'scripts/insert_code.py',
        args.input_rom, code_rom,
        '--language', language
    ]
    if args.verbose:
        insert_code_cmd.append('--verbose')
    if args.temp_dir != '__tmp':
        insert_code_cmd.extend(['--temp-dir', args.temp_dir])

    run_command(insert_code_cmd)

    # Step 2: Insert files
    print(f"Step 2: Inserting files for {language}...")
    insert_files_cmd = [
        sys.executable, 'scripts/insert_files.py',
        code_rom, final_rom,
        '--language', language
    ]

    run_command(insert_files_cmd)

    # Step 3: Generate xdelta patch
    if not args.no_patches:
        print(f"Step 3: Generating xdelta patch for {language}...")
        try:
            run_command([
                'xdelta3', '-e', '-f', '-s', args.input_rom, final_rom, patch_file
            ])
            print(f"Generated patch: {patch_file}")
        except Exception as e:
            print(f"Warning: Failed to generate patch for {language}: {e}")

    # Clean up intermediate ROM
    if os.path.exists(code_rom):
        os.remove(code_rom)

    print(f"Built ROM: {final_rom}")
    return final_rom, patch_file if not args.no_patches else None

def clean_temp_directory():
    """Clean the temporary directory"""
    if args.clean_temp and os.path.exists(args.temp_dir):
        print(f"Cleaning temporary directory: {args.temp_dir}")
        shutil.rmtree(args.temp_dir)

def main():
    global args
    args = parse_arguments()

    # Validate input ROM
    if not os.path.isfile(args.input_rom):
        print(f"Error: Input ROM '{args.input_rom}' not found")
        sys.exit(1)

    # Extract base name from input ROM or use prefix
    base_name = args.prefix if args.prefix else Path(args.input_rom).stem

    print(f"Building ROMs from: {args.input_rom}")
    print(f"Output directory: {args.output_dir}")
    print(f"Output name: {base_name}")
    print(f"Languages: {', '.join(args.languages)}")
    print(f"Temporary directory: {args.temp_dir}")

    # Check dependencies
    check_dependencies()

    # Ensure output directory exists
    ensure_directory(args.output_dir)

    # Build ROMs for each language
    built_roms = []
    built_patches = []

    try:
        for language in args.languages:
            rom_path, patch_path = build_rom_for_language(language, base_name)
            built_roms.append((language, rom_path))
            if patch_path:
                built_patches.append((language, patch_path))

        # Summary
        print(f"\n=== Build Complete ===")
        print(f"Built {len(built_roms)} ROM(s):")
        for language, rom_path in built_roms:
            file_size = os.path.getsize(rom_path) / (1024 * 1024)  # MB
            print(f"  {language}: {rom_path} ({file_size:.1f} MB)")

        if built_patches:
            print(f"\nGenerated {len(built_patches)} patch(es):")
            for language, patch_path in built_patches:
                file_size = os.path.getsize(patch_path) / 1024  # KB
                print(f"  {language}: {patch_path} ({file_size:.1f} KB)")

    except KeyboardInterrupt:
        print("\nBuild interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\nBuild failed: {e}")
        sys.exit(1)
    finally:
        # Clean up temporary directory if requested
        clean_temp_directory()

if __name__ == '__main__':
    main()
