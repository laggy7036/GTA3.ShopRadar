# Install GTA III Shop Radar

This is a beta mod for **classic GTA III on Windows**, not Definitive Edition. It has been tested on the Steam version; retail 1.0/1.1 have not been tested in game. See [test status](docs/VALIDATION.md).

## Requirements

- A working, legally obtained copy of classic GTA III.
- An ASI loader supporting `InitializeASI`, such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). Follow its own instructions for GTA III. Preserve existing graphics wrappers and loaders.
- The compiled mod ZIP. GitHub's **Code → Download ZIP** is source code; it is not a ready-to-install mod. Build from source if no compiled release is available.

## Recommended companion fixes

The classic Steam release has known mouse, menu and widescreen problems. Shop Radar was tested together with SilentPatch III and GTA III Widescreen Fix, and we recommend them. See [the recommended setup and official links](README.md#recommended-setup). Install these separately using their authors' instructions; they are not included in this ZIP.

## Install or update

1. Close GTA III completely.
2. Extract `GTA3.ShopRadar.asi` and `GTA3.ShopRadar.ini` from the compiled ZIP.
3. Copy those two files into the game's `scripts` directory. The loader must be configured to discover ASIs there.
4. On an update, **keep your existing INI** to preserve your settings; missing new options use defaults.
5. Launch the game. Open **Options → Shop Radar**. F7 is an additional shortcut from supported pause/settings pages.

Typical layout with a D3D8 ASI loader:

```text
<GTA III folder>/
  gta3.exe
  d3d8.dll                  existing compatible ASI loader
  scripts/
    GTA3.ShopRadar.asi
    GTA3.ShopRadar.ini
    GTA3.ShopRadar.log      created by the mod
```

No game executable, `main.scm`, texture archive or save needs replacement. The mod writes its own INI/log, not save files.

## Settings

| Option | Values |
| --- | --- |
| Display radius | 50–1000 world units, steps of 50 |
| Icon size | 75%, 100%, 125% |
| Each service category | Independent visibility, icon and colour frame |
| Colours | Original, Green, Purple, Gold, Cyan, Red |
| Mission radius | 25-100% of Display radius, 5-point steps; default 60% |

Alternative colours add an outlined accent frame. The original pictogram keeps its colours and highlights. Original has no extra frame. Before loading a save, previews use read-only classic PAL8 textures from models/hud.txd. Once live radar textures exist, the game sprites take over.

Keyboard: Up/Down selects a row; Tab selects a field; Left/Right changes it; Enter/Space cycles forward; Escape/Backspace/F7 returns. Mouse clicks cycle values. Gamepad controls are not implemented.

## Compatibility

See the [README compatibility table](README.md). The shop catalogue is for the original map and is not adapted for map/script conversions. Other mods replacing radar/menu hooks can conflict. Mod Loader installation has not been tested; use the documented ASI-loader scripts layout. Do not infer compatibility from the mod merely reaching the menu.

## Troubleshooting

- **No menu entry:** inspect `scripts/GTA3.ShopRadar.log`. Unsupported ABI/hook signatures stop initialization. A fallback launcher can appear if only native menu integration fails.
- **No preview before loading:** check the mod log for missing/unsupported HUD textures. The menu supports the classic D3D8 PAL8 hud.txd; replacement HUD formats may require adaptation. No HUD file is replaced.
- **No shop icon:** check category/master visibility, distance and service state. Native mission markers remain game-controlled. Unknown additional service availability is hidden during missions.
- **Crash:** close the game, remove only this mod's ASI and compare. Report version and reproduction steps in an issue; do not attach saves.

## Uninstall

Close GTA III and remove `scripts/GTA3.ShopRadar.asi`. Optionally remove its INI/log. Existing game saves and other mods do not need changes.

The obsolete absolute MissionRadius INI key is ignored. MissionRadiusPercent controls the relative range, including exact distances below 50 world units when the chosen percentage requires it.



## Dynamic discovery

`General/DynamicDiscovery` defaults to 1. Set it to 0 while the game is closed to use only the fixed ten-location catalogue. Discovered sprays/bomb shops use their existing categories. Other weapon shops covers the original Shoreside dealer and discovered weapon-shop clusters, keeping that category's existing INI key and settings.
