# Test status

Version: 0.1.2 beta, hybrid discovery.

## Automated tests

- 210,500 distance samples and 11,088 Windows INI/UI integration checks pass (including saving and logging in a folder with non-ANSI characters), plus 86 discovery checks.
- Radius/fade, category controls, percentage mission radius, sizes 75/100/125, original icon colours, bounded marker nudges, retired icon migration, malformed data and startup hook guards are covered.
- The build produces an x86 PE DLL with an `InitializeASI` export; release files pass the offline publication check.

## In-game testing

Tested on classic English Steam with SilentPatch III, Widescreen Fix and Ultimate ASI Loader. No crashes have been observed so far.

- The Options → Shop Radar entry, settings page, icon previews before loading a save, colour frames and bomb icons were checked in game.
- A save was loaded and driven around Staunton: spray and bomb icons showed separately, native mission markers drew on top, and icons faded out at the configured radius. Settings persisted after restarting the game.
- With discovery on and off, the original map produced no extra services, as expected (all are already in the catalogue).
- An earlier font-colour crash was found and fixed before this release.

## Not yet fully tested

- Shop availability, mission radius and overlapping markers during active missions.
- Icon movement in crowded radar areas, every location and size, and exact radius boundaries.
- Death/arrest/reload/cutscene transitions and long play sessions.
- Discovery of genuinely new shops added by map mods.
- Retail 1.0/1.1 executables (address tables are included but untested). Definitive Edition is unsupported.

Please report problems with reproduction steps in an issue.
