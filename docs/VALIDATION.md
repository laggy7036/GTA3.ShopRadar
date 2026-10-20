# Test status

Version: 0.1.0 beta, fixed catalogue.

## Automated tests

- 210,500 distance samples and 10,664 Windows INI/UI integration checks pass.
- Radius/fade, category controls, percentage mission radius, sizes 75/100/125, original icon colours, bounded marker nudges, retired icon migration, malformed data and startup hook guards are covered.
- The build produces an x86 PE DLL with an `InitializeASI` export; release files pass the offline publication check.

## In-game testing

Tested on classic English Steam with SilentPatch III, Widescreen Fix and Ultimate ASI Loader. No crashes have been observed so far.

- The Options → Shop Radar entry, settings page, icon previews before loading a save, colour frames and bomb icons were checked in game.
- Saves loaded without problems and icons appeared near shops; settings persisted after restarting the game.
- An earlier font-colour crash was found and fixed before this release.

## Not yet fully tested

- Shop availability, mission radius and overlapping markers during active missions.
- Icon movement in crowded radar areas, every location and size, and exact radius boundaries.
- Death/arrest/reload/cutscene transitions and long play sessions.
- Retail 1.0/1.1 executables (address tables are included but untested). Definitive Edition is unsupported.

Please report problems with reproduction steps in an issue.
