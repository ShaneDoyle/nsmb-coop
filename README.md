# NSMB Co-op

Play New Super Mario Bros. (DS) with a friend! This mod adds full cooperative multiplayer to the entire game, letting you team up through all the levels, boss fights, and even worldmap exploration.

## Features
- **Full Co-op Campaign**: Play through every level with 2 players via local wireless
- **Respawn System**: When you die, you can watch your friend and respawn near them
- **Functional Worldmap**: Explore and pick levels together
- **Boss Battle Adaptations**: All bosses have been reworked for 2-player fights
- **Desync Protection**: If things get out of sync, the game automatically fixes itself by rolling back to before you entered the level
- **Widescreen Support**: Expands the game to fill the entire 3DS screen for a better view (toggle with L+R+X, requires nds-bootstrap)
- **Multi-language Support**: Available in 7 languages (English, French, German, Italian, Japanese, Spanish, Portuguese)

## Building

### Prerequisites
- **Python 3.x** with the following packages:
  - `ndspy` - Nintendo DS ROM manipulation library
  - Install with: `pip install ndspy`
- **ARM cross-compilation toolchain**:
  - `arm-none-eabi-gcc` and related tools
  - On Ubuntu/Debian: `sudo apt install gcc-arm-none-eabi`
  - On Windows: Install via [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- **Git** - For version control and build metadata
- **NCPatcher** - Nintendo DS code patching tool
- **xdelta3** (optional) - For generating binary patches

### Build Steps
1. **Clone the repository**:
   ```bash
   git clone https://github.com/ShaneDoyle/nsmb-coop.git
   cd nsmb-coop
   ```

2. **Prepare the base ROM**:
   - Obtain a clean New Super Mario Bros. (USA) ROM[<sup>1</sup>](#notes)
   - Place it as `rom.nds` in the repository root

3. **Build the modification**:

   **Option A: Build all languages (recommended)**
   ```bash
   python scripts/build_roms.py rom.nds
   ```

   **Option B: Build specific languages**
   ```bash
   python scripts/build_roms.py rom.nds -l en fr ge
   ```

   **Option C: Manual build (single language)**
   ```bash
   python scripts/insert_code.py rom.nds __tmp/rom_code.nds --language en
   python scripts/insert_files.py __tmp/rom_code.nds nsmb-coop.nds --language en
   ```

### Build Options
The `build_roms.py` script supports several options:
- `-l, --languages`: Specify which languages to build (en, fr, ge, it, jp, sp, pt)
- `-o, --output-dir`: Set custom output directory (default: `build`)
- `-p, --prefix`: Custom name prefix for output files
- `--no-patches`: Skip xdelta patch generation
- `--temp-dir`: Custom temporary directory (default: `__tmp`)
- `--clean-temp`: Clean temporary files after build
- `-v, --verbose`: Enable verbose output

### Build Output
The build process generates:
- **ROMs**: Built ROMs for each language in `build/nds/` directory
- **Patches**: xdelta binary patches in `build/xdelta/` directory (optional)
- **Multi-language support**: Separate ROM files for different game languages

Example output structure:
```
build/
├── nds/
│   ├── rom_en.nds      # English version
│   ├── rom_fr.nds      # French version
│   ├── rom_ge.nds      # German version
│   └── ...
└── xdelta/
    ├── rom_en.xdelta   # English patch
    ├── rom_fr.xdelta   # French patch
    └── ...
```

The build process will:
- Insert custom files and assets for the specified language(s)
- Update the ROM with build metadata (commit hash and timestamp)
- Generate object tables and compile C++ source code
- Apply code patches using NCPatcher with language-specific definitions
- Generate xdelta patches for distribution (optional)

### Troubleshooting
- Ensure all prerequisites are installed and in your system PATH
- Verify the ROM matches the expected checksums listed in [Notes](#notes)
- Check that the ARM toolchain is properly configured for cross-compilation
- If xdelta3 is not found, patch generation will be automatically skipped
- Use `--verbose` flag with build scripts for detailed debugging output
- For build issues, try cleaning the temporary directory with `--clean-temp`

## Credits

### Core Development Team
- **[TheGameratorT](https://github.com/TheGameratorT)** - Lead developer, research, gameplay systems, engine modifications, testing and debugging
- **[Shadey21](https://github.com/ShaneDoyle)** - Early development, gameplay systems, testing and debugging

### Special Contributors
- **[Isaac0-dev](https://github.com/Isaac0-dev)** - Flagpole mechanics prototyping
- **[gamemasterplc](https://github.com/gamemasterplc)** - Original widescreen code

### Research & Reference
- **[Mamma Mia Team](https://github.com/MammaMiaTeam)** - Foundational research and [NSMB Code Reference](https://github.com/MammaMiaTeam/NSMB-Code-Reference)
- **[Arisotura](https://github.com/Arisotura)** - melonDS emulator development and DS system expertise

### Tools & Assets
- **[Gota7](https://github.com/Gota7)** - [Nitro Studio 2](https://github.com/Gota7/NitroStudio2) for 3D model editing
- **[Garhoogin](https://github.com/Garhoogin)** - [NitroPaint](https://github.com/Garhoogin/NitroPaint) for texture editing
- **[TheGameratorT](https://github.com/TheGameratorT)** - [NDS Banner Editor](https://github.com/TheGameratorT/NDS_Banner_Editor)
- **[Mamma Mia Team](https://github.com/MammaMiaTeam)** - [NSMB Editor](https://github.com/MammaMiaTeam/NSMB-Editor)
- **dotPDN LLC** - [Paint.NET](https://www.getpaint.net/) for image editing

This project builds upon the extensive research and reverse engineering efforts of the Nintendo DS homebrew community.

## Notes
1\.  
> Common filename: New Super Mario Bros (USA).nds  
> Size: 33554432  
> CRC32: 0197576a  
> MD5: a2ddba012e5c3c2096d0be57cc273be5  
> SHA1: a22713711b5cd58dfbafc9688dadea66c59888ce  
> SHA256: 9f67fef1b4c73e966767f6153431ada3751dc1b0da2c70f386c14a5e3017f354
