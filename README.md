# NSMB Co-op

Play New Super Mario Bros. (DS) with a friend! This mod adds full cooperative multiplayer to the entire game, letting you team up through all the levels, boss fights, and even worldmap exploration.

## Features
- **Full Co-op Campaign**: Play through every level with 2 players via local wireless
- **Respawn System**: When you die, you can watch your friend and respawn near them
- **Functional Worldmap**: Explore and pick levels together
- **Boss Battle Adaptations**: All bosses have been reworked for 2-player fights
- **Desync Protection**: If things get out of sync, the game automatically fixes itself by rolling back to before you entered the level
- **Widescreen Support**: Expands the game to fill the entire 3DS screen for a better view (toggle with L+R+X, requires nds-bootstrap)
- **Multi-language Support**: Available in 9 languages (English, French, German, Italian, Spanish, Japanese, Korean, Chinese, Portuguese)

## Building

### Prerequisites
- **ARM cross-compilation toolchain**:
  - `arm-none-eabi-gcc` and related tools
  - On Ubuntu/Debian: `sudo apt install gcc-arm-none-eabi`
  - On Windows: Install via [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- **Git** - to fetch the code reference and to stamp the build
- **NCPatcher 2.x** - Nintendo DS code patching tool with native module support
- **nsmbtool** - fetches the pinned code reference and generates the object,
  scene and file-id headers. Both it and NCPatcher must be on your `PATH`.
- **Converted Nitro SDK/Nitro System headers** - set `NSMB_NITRO_ROOT` to their
  root. These are private, cannot be fetched, and are the one build input that
  is a property of your machine rather than of this repository, which is why
  they are the only thing you have to place by hand.
- **xdelta3** (optional) - for generating binary patches

There is no Python and no ndspy. Everything the scripts used to do is either a
key in `ncpatcher.yaml` or a `nsmbtool` subcommand.

### The code reference is pinned
`nsmbref.lock` names the exact commit of
[NSMB Code Reference](https://github.com/MammaMiaTeam/NSMB-Code-Reference) this
project builds against. `nsmbtool reference sync` materialises that commit into a
shared store and writes `.ncpatcher.env`, which NCPatcher reads for
`NSMBREF_ROOT`.

That file overrides whatever `NSMBREF_ROOT` your shell profile exports, on
purpose: two projects pinning different revisions have to build correctly in the
same shell, and a stale global must not silently decide what this one compiles
against. `.ncpatcher.env` is generated, machine-specific and gitignored; the lock
is what gets committed.

To move to a newer reference, `nsmbtool reference use <branch|tag|commit>` and
commit the lock it rewrites.

### Build Steps
1. **Clone the repository**:
   ```bash
   git clone --branch module https://github.com/ShaneDoyle/nsmb-coop.git nsmb-coop-module
   cd nsmb-coop-module
   ```

2. **Prepare the base ROM**:
   - Obtain a clean New Super Mario Bros. (USA) ROM[<sup>1</sup>](#notes)
   - Place it as `rom.nds` in the repository root

3. **Build the modification**:

   **All languages**
   ```bash
   nsmbtool reference sync
   ncpatcher build --all-variants
   ```

   **One language**
   ```bash
   ncpatcher build --variant en
   ```

   `sync` is idempotent and offline once the revision is in the store, so it is
   cheap to leave in a build script; only a first checkout needs the network.

### What the build does
NCPatcher sweeps each module's `nitrofs/` tree into the ROM, choosing the layer
that matches the variant and falling back to `en`, then compiles and patches.
Three hooks hang off it:

| Hook | Phase | Does |
|---|---|---|
| `nsmbtool stamp` | pre-build | writes `BUILDTIME`, the commit and date the crash screen shows |
| `nsmbtool glue` | post-files | object ids, profile tables, the scene registry, `fid.hpp`, the level-data getters, and the editor contracts |
| `xdelta3` | post-build | the distributable patch |

`glue` runs at `post-files` rather than `pre-build` because `fid.hpp` turns this
build's NitroFS file ids into constants: that is the one phase which runs after
insertion has assigned them and before anything including them is compiled.

The `mkdir -p` in the xdelta hook is POSIX. Hooks go through the system shell, so
on Windows either change that line or drop the hook - the patches are for
distribution and nothing in the ROM depends on them.

### Build Output
```
build/
├── nds/
│   ├── rom_en.nds      # English version
│   ├── rom_fr.nds      # French version
│   └── ...
├── xdelta/
│   ├── rom_en.xdelta   # English patch
│   └── ...
└── generated/
    ├── files.json          # the ROM file table, per variant
    ├── modules.json        # the resolved module graph
    ├── level_data.json     # editor contract: level-data keys and hashes
    ├── stageobjects.json   # editor contract: placeable objects
    └── include/            # fid.hpp, object ids, registries
```

A single-variant build writes `build/nds/rom.nds`; `--all-variants` appends the
variant name to each.

### Troubleshooting
- Ensure NCPatcher, `nsmbtool` and the ARM toolchain are on your `PATH`
- Verify the ROM matches the expected checksums listed in [Notes](#notes)
- `nsmbtool reference list` shows which revisions are in the store and which one
  this project names; `nsmbtool reference gc` removes the rest
- If a header seems stale, delete `build/generated` - everything in it is
  regenerated, and `glue` only rewrites files whose contents actually changed

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
