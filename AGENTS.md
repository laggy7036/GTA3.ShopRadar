# Contributor guidelines

- Scope: classic GTA III Shop Radar ASI source in this repository.
- Preserve the ten verified service locations when discovering additional services. Never identify loose weapon pickups as shops.
- Do not write save data, replace main.scm or game assets, or allocate persistent radar blips. Test with an independent save copy and do not save during tests.
- Validate executable version, memory layout and hook sites before patching; fail safely and retain partial-install rollback. Never replace an ASI while loaded.
- Preserve the CFont colour-pointer ABI, CSprite2d pointer-array layout and Rect field order. Inspect existing bridges before changing calling conventions.
- Keep native markers above custom shop icons. Use actual service state for mission availability; distance alone is insufficient.
- Keep collision offsets bounded near the real marker anchor; tolerate overlap rather than showing a false location.
- Keep selectable icon IDs separate from internally resolved bomb variants. Preserve migration of retired INI values.
- Load menu previews read-only from the user's local HUD. Do not redistribute extracted game textures.
- Use the wide-character (W) Windows APIs for every file path; never the ANSI (A) variants.
- The version number lives only in `VERSION`; never hardcode it elsewhere.
- All changes reach `master` through a pull request with green CI; `master` is protected.
- To release: bump `VERSION` and add its `CHANGELOG.md` section in a pull request, then push tag `v<VERSION>`. Never build or upload release assets by hand.
- Pin new GitHub Actions to a full commit SHA with the version in a comment. Compiler warnings are errors.
- Run build.ps1 and relevant publication checks. Distinguish constructed checks, live observations and untested scenarios in docs/VALIDATION.md.
- Public media: only reviewed game-window captures, re-encoded without metadata, stored in `docs/media/` and listed in the release package. Raw captures stay in the ignored `docs/media/drafts/`.
- Release ZIPs contain end-user files and installation documentation, excluding local notes, saves, game files, reference dumps, toolchains and credentials.

