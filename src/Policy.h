#pragma once
#include <math.h>

namespace shopradar {
enum Category { AmmuNation, Phil, Spray, Bomb, OtherWeapons, CategoryCount };
struct Vec2 { float x, y; };
struct Location { const char* name; Category category; Vec2 position; };
// Storefronts / garage centres, not loose weapon pickups.
inline constexpr Location locations[] = {
    {"Portland Ammu-Nation", AmmuNation, {1068.5f, -400.75f}},
    {"Staunton Ammu-Nation", AmmuNation, {345.5f, -713.5f}},
    {"Phil's Army Surplus", Phil, {143.5f, 170.0f}},
    {"Portland Pay 'n' Spray", Spray, {925.0f, -359.5f}},
    {"Staunton Pay 'n' Spray", Spray, {379.0f, -493.75f}},
    {"Shoreside Pay 'n' Spray", Spray, {-1143.75f, 34.90625f}},
    {"Portland 8-Ball", Bomb, {1282.0f, -104.0f}},
    {"Staunton 8-Ball", Bomb, {381.28125f, -576.53125f}},
    {"Shoreside 8-Ball", Bomb, {-1082.4375f, 53.5f}},
    {"Shoreside weapon dealer", OtherWeapons, {-1208.5625f, -6.5625f}},
};
inline constexpr const char* categoryNames[] = {
    "Ammu-Nation", "Phil's Army Surplus", "Pay 'n' Spray", "8-Ball's Bomb Shops", "Shoreside weapon dealer"
};
inline constexpr int spriteIds[] = {20, 18, 2, 7, 17, 3};
inline constexpr const char* spriteNames[] = {"Weapon", "Spray", "Bomb", "8-Ball", "Safehouse", "Cartel", "Bomb type", "Ignition", "Timer", "Remote", "Logo+key", "Logo+clock", "Logo+radio"};
inline constexpr int iconCount=13; // Stable persisted IDs, including retired assets.
inline bool selectableIcon(int choice) { return choice>=0 && choice<=6; }
inline int activeIcon(int choice) { return choice>=7 && choice<iconCount?6:choice; }
inline int nextIcon(int choice,int direction) {
    do { choice=(choice+direction+iconCount)%iconCount; } while(!selectableIcon(choice));
    return choice;
}
// Describes the service offered, never the vehicle arming state.
inline int locationIcon(int choice,int index) { return choice==6?(index==7?11:index==8?12:10):choice; }
// Accent frames carry these colours; original sprite RGB stays unchanged.
inline constexpr const char* colourNames[] = {"Original", "Green", "Purple", "Gold", "Cyan", "Red"};
struct CategorySettings { bool enabled = true; int icon = 0; int colour = 0; };
struct Settings {
    bool enabled = true;
    bool dynamicDiscovery = true;
    int radius = 250;
    int missionPercent = 60;
    int iconSize = 100;
    CategorySettings categories[CategoryCount] = {{true,0,0},{true,0,0},{true,1,0},{true,6,0},{true,0,0}};
};
enum class Availability { Unknown, Available, OutOfStock, Disabled };
inline int clamp(int value, int low, int high) { return value < low ? low : value > high ? high : value; }
inline float distanceSquared(Vec2 a, Vec2 b) { float x=a.x-b.x, y=a.y-b.y; return x*x+y*y; }
inline unsigned char opacityAtRadius(const Settings& settings, Category category, Vec2 player, Vec2 target,float radius) {
    if (!settings.enabled || !settings.categories[category].enabled) return 0;
    const float squared = distanceSquared(player, target);
    if (squared >= radius*radius || !isfinite(squared)) return 0;
    const float fade = radius * 0.15f;
    const float remaining = radius - sqrtf(squared);
    return remaining >= fade ? 255 : (unsigned char)(255.0f * remaining/fade);
}
inline unsigned char opacity(const Settings& settings,Category category,Vec2 player,Vec2 target) {
    return opacityAtRadius(settings,category,player,target,float(clamp(settings.radius,50,1000)));
}
inline float missionDistance(const Settings& settings) {
    return clamp(settings.radius,50,1000)*clamp(settings.missionPercent,25,100)/100.0f;
}
inline unsigned char serviceOpacity(const Settings& settings, Category category, Vec2 player,
                                    Vec2 target, bool mission, Availability state) {
    if(state==Availability::Disabled || state==Availability::OutOfStock) return 0;
    if(mission && state!=Availability::Available) return 0;
    return opacityAtRadius(settings,category,player,target,mission?missionDistance(settings):float(clamp(settings.radius,50,1000)));
}
inline bool nativeMatches(const Location& location, Vec2 position, int sprite) {
    const int expected = location.category == Spray ? 18 : location.category == Bomb ? 2 : 20;
    return sprite == expected && distanceSquared(location.position, position) <= 35.0f*35.0f;
}
}
