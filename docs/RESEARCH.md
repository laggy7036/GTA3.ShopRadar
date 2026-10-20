# Availability and existing mods

Research date: 2026-10-20. The read-only classifiers below are now implemented, with limitations and runtime coverage recorded in VALIDATION.md. They are not a universal availability guarantee.

## Existing work

- [1826: Ammu-Nation, Pay'n'Spray and Bomb short range blips](https://libertycity.net/files/gta-3/182381-ammu-nation-paynspray-and-bomb-short-range-blips.html), updated 21 October 2022. A close match for the basic feature: service blips disappear when far away. Its author publishes [ShortRangeBlips-III in gta-sources](https://github.com/the-1826/gta-sources). The author's README describes CLEO 3+ and no external opcodes/memory edits. The listing also mentions phone markers and a Shoreside bomb marker cleanup fix.
- [Radar Icons v1](https://libertycity.net/files/gta-3/153135-radar-icons-v1.html), 2021: permanently marked radar images. Different approach from runtime icons.
- [WILDVICE: The Ability To See Icons On Radar for Mobile](https://www.mobilegta.net/en/gta3/mods/186973-the-ability-to-see-icons-on-radar-for-mobile/), 2023: Android/CLEO service markers.

The underlying idea is not new. No exact match with all our pause settings, category toggles, icon/colour choices and availability-aware mission behaviour was identified in these searches. This is not proof none exists. None of those mods was installed or copied into this project.

## Recommended availability architecture

Use read-only game state, separated from drawing. Do not patch missions or maintain a list keyed by mission titles. Cache one state per catalogue location and refresh around 4 times per second; render from that cache. Invalidate on new/load game, state transitions and invalid ABI data. Check only nearby catalogue locations when appropriate.

Represent at least Unknown, Available, OutOfStock and Disabled, with a reason. Separate “shop service exists” from “this player can buy/use it now”: insufficient money, already owning armour or an unsuitable vehicle must not be mistaken for a permanently closed shop.

### Weapon services

[Plugin-SDK CPickup](https://raw.githubusercontent.com/DK22Pac/plugin-sdk/master/plugin_III/game_III/CPickup.h) exposes shop type 1 and out-of-stock type 6, position and removal/timer state. [CPickups](https://github.com/DK22Pac/plugin-sdk/blob/master/plugin_III/game_III/CPickups.cpp) gives a versioned pool of 336 records, each 0x1C bytes. Classify only these shop types inside tight store-specific bounds; never make radar icons for loose pickups.

The [SCM reference](https://github.com/Lighnat0r-pers/GTA-III-SCM-Converted) corroborates this distinction: pickups.sc creates Portland out-of-stock items; help.sc and mission 27_joey4.sc replace items with shop stock. Phil's mission 55_ray2.sc creates a free mission pickup before creating purchasable stock. A nearby arbitrary weapon or an unlock-completion guess would therefore give false positives.

Do not require a rendered object pointer: distant/unstreamed stock need not have an object. Treat temporary removal/respawn carefully, and verify Update semantics before calling absence “closed.” Stock presence alone does not establish every script restriction or physical access condition.

### Garages

[Plugin-SDK CGarage](https://raw.githubusercontent.com/DK22Pac/plugin-sdk/master/plugin_III/game_III/CGarage.h) exposes service type, bounds, deactivation and door state. Match the six catalogue garages to their actual bounds/type, then inspect deactivation and the relevant service's player/vehicle conditions. A closed door alone does not mean unavailable: it can open on approach or be busy processing a vehicle. Do not call OpenThisGarage/ActivateGarage or mutate any fields.

The current CGarages.cpp global addresses in this SDK are unversioned. Resolve/validate Steam addresses rather than assuming the 1.0 layout address. Check vehicle-specific respray restrictions and any global service controls against executable behaviour. Availability code must return Unknown if unsupported instead of reading guessed pointers.

### Missions and rendering

Keep native mission markers intact. For an additional catalogue icon during a mission, require confirmed availability and a shorter configurable distance (a proposed starting point is 75 world units, capped by the normal radius). Unknown is not confirmed available. Resolve screen-space overlaps in favour of mission markers, including vehicle/ped/coordinate marker types. Draw order or position matching to the same shop sprite alone does not guarantee priority.

The implementation now draws custom icons before the original radar blips, so native coordinate/entity/sprite markers draw on top. Mission shop duplicates remain suppressed. Stock/garage state is cached for 250 ms. Vehicle restrictions are enabled only on the validated Steam ABI; unknown mission service state fails closed. Actual mission overlap, locked stock and vehicle scenarios still need gameplay verification.

## Acceptance matrix before enabling availability

Test Portland before/after first stock unlock, Portland temporary stock removal, Staunton stock, Phil before/during/after Arms Shortage, Shoreside dealer, each garage active/deactivated/busy, supported and rejected vehicles, player on foot, new/load game, streaming in/out, and a usable/disabled service during missions. Use separate test saves for these states.
