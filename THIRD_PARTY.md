# References and acknowledgements

Plugin-SDK by DK22Pac and GTA community contributors was used to research GTA III function addresses, calling conventions and data layouts:
https://github.com/DK22Pac/plugin-sdk

The bridge in `src/ShopRadar.cpp` is independently implemented and is not the full Plugin-SDK. The SDK checkout in `vendor/` is development-only and excluded from distribution. Plugin-SDK's zlib-style licence is preserved in its checkout.

Location coordinates were cross-checked against community-transcribed GTA III script references:
https://github.com/Lighnat0r-pers/GTA-III-SCM-Converted

Relevant reference files: `init.sc` (garage bounds), `genstuf.sc` (Ammu-Nation interiors), `pickups.sc` (purchasable weapons, including the Shoreside dealer). Reference scripts are not shipped with this mod.

The mod uses textures already loaded by the user's copy of GTA III. No Rockstar assets or executable are redistributed. GTA and related marks belong to their respective owners.

Zig is a development tool only: https://ziglang.org/. Its own licences are included in the downloaded toolchain and it is excluded from distribution.

Recommended companion projects used in testing, distributed separately by their authors: [SilentPatch III](https://silentsblog.com/mods/gta-iii/), [GTA III Widescreen Fix](https://github.com/ThirteenAG/WidescreenFixesPack/releases/tag/gta3), and [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). See README for their roles.
