# NSMB Co-op

Play New Super Mario Bros. (DS) with a friend! This mod adds full cooperative multiplayer to the entire game, letting you team up through all the levels, boss fights, and even worldmap exploration.

## Features
- **Full Co-op Campaign**: Play through every level with 2 players via local wireless
- **Respawn System**: When you die, you can watch your friend and respawn near them
- **Functional Worldmap**: Explore and pick levels together
- **Boss Battle Adaptations**: All bosses have been reworked for 2-player fights
- **Desync Protection**: If things get out of sync, the game automatically fixes itself by rolling back to before you entered the level
- **Widescreen Support**: Expands the game to fill the entire 3DS screen for a better view (toggle with L+R+X, requires nds-bootstrap)

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

### Build Steps
1. **Clone the repository**:
   ```bash
   git clone https://github.com/ShaneDoyle/nsmb-coop.git
   cd nsmb-coop
   ```

2. **Prepare the base ROM**:
   - Obtain a clean New Super Mario Bros. (USA) ROM[<sup>1</sup>](#notes)
   - Place it as `nsmb-coop.nds` in the repository root

3. **Build the modification**:
   ```bash
   python scripts/insert_files.py nsmb-coop.nds
   python scripts/insert_code.py nsmb-coop.nds
   ```

The build process will:
- Insert custom files and assets
- Update the ROM with build metadata (commit hash and timestamp)
- Generate object tables and compile C++ source code
- Apply code patches using NCPatcher

### Troubleshooting
- Ensure all prerequisites are installed and in your system PATH
- Verify the ROM matches the expected checksums listed in [Notes](#notes)
- Check that the ARM toolchain is properly configured for cross-compilation

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
