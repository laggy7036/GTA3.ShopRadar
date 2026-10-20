# Known issues and fixes

## Fixed during development

| Issue | Fix |
| --- | --- |
| Crash when drawing menu text (font colour passed with the wrong calling convention) | Colour is passed by pointer as the game expects |
| Startup scanners could read code bytes without validating their full span | All scans are bounds- and page-checked before reading |
| Instruction displacement accessed through potentially unaligned pointers | Bytes are copied with memcpy |
| Strong colour tints hid icon details | Colour is an accent frame; the original pictogram stays untinted |
| Buffered mouse click used the cursor's later position | The position from the button-down message is used |
| F7 close could reach the native menu handler in the same frame | Handler returns immediately after closing |
| Missing Icon field inherited a previous choice on reload | Explicit category defaults |
| Options scanner trusted unbounded strings and partly empty rows | Fixed-size key comparison and whole-record checks |
| Address-plus-length overflow in memory validation | Overflow and region-length checks |
| Short menu key presses could be missed between polls | Key and mouse messages are buffered for the mod's own menu |
| Release ZIP flattened docs and broke README links | Archive keeps the docs/ and media/ layout |

## Open

- Mission-specific access rules, deactivated/busy garages, rejected vehicles and marker overlap during missions need more gameplay coverage.
- Radius crossings at all ten services and retail 1.0/1.1 have not been fully tested.
- The mod's menu buffering only covers its own settings page; it does not change the game's own menu input.

## Gameplay checklist

Scenarios still worth testing (use a separate save):

- Frontend Options, loading a save, pausing, returning and repeating; alt-tab/focus loss.
- All icons, colour modes and sizes; repeated restarts and settings persistence.
- Normal and mission radius boundaries at each service; sold-out stock, busy/deactivated garages, unsuitable vehicles, overlapping mission markers.
- Death/arrest, mission start/fail/end, cutscenes, radar disabled, returning to the frontend and reloading.
- Other radar/menu-hook mods and retail executables.

Startup guards and signature checks reduce risk but cannot rule out every crash. Please report problems in an issue.

## Dynamic discovery limits

- Relocated or resized game record pools are not supported.
- Single-item shops, custom weapon model/pickup types and unusually large garages may be intentionally skipped.
- Nearby same-family shops/floors can be merged; known coordinates are preserved and not relocated.
- Extra weapon shops inherit the Other weapon shops category settings, including an existing OFF choice.
- Discovery is an INI setting; there is no separate menu row yet.
