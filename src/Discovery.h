#pragma once
#include "Services.h"
namespace shopradar {
inline constexpr int knownServiceCount=sizeof(locations)/sizeof(locations[0]);
inline constexpr int maxDiscoveredServices=64;
struct DiscoveredService { Location location; Availability state; int bombIcon; };
struct DiscoveryResult { DiscoveredService entries[maxDiscoveredServices]{}; int count=0; bool full=false; };
inline bool discoveryPoint(Vec2 p) { return isfinite(p.x) && isfinite(p.y) && fabsf(p.x)<50000 && fabsf(p.y)<50000; }
inline bool sameServiceFamily(Category a,Category b) {
    return a==b || ((a==AmmuNation || a==Phil || a==OtherWeapons) && (b==AmmuNation || b==Phil || b==OtherWeapons));
}
inline bool discoveryDuplicate(const DiscoveryResult& result,Category category,Vec2 p,float radius=35) {
    for(const auto& known:locations) if(sameServiceFamily(category,known.category) && distanceSquared(p,known.position)<=radius*radius) return true;
    for(int i=0;i<result.count;++i) if(sameServiceFamily(category,result.entries[i].location.category) &&
        distanceSquared(p,result.entries[i].location.position)<=radius*radius) return true;
    return false;
}
inline void addDiscovered(DiscoveryResult& result,DiscoveredService service) {
    if(result.count>=maxDiscoveredServices) { result.full=true; return; }
    result.entries[result.count++]=service;
}
inline bool discoveryGarage(const GarageRecord& g) {
    return (g.type>=2 && g.type<=5) && g.deactivated<=1 &&
        discoveryPoint({g.left,g.front}) && discoveryPoint({g.right,g.back}) &&
        isfinite(g.down) && isfinite(g.up) &&
        g.right-g.left>=.5f && g.right-g.left<=80 && g.back-g.front>=.5f && g.back-g.front<=80 &&
        g.up-g.down>=.5f && g.up-g.down<=40;
}
inline bool discoveryWeapon(const PickupRecord& p) {
    // Native for-sale or out-of-stock records, NOT loose/collectable pickup types.
    return (p.type==1 || p.type==6) && p.model>=170 && p.model<=181 && p.model!=179 &&
        discoveryPoint({p.x,p.y}) && isfinite(p.z) && fabsf(p.z)<10000;
}
inline DiscoveryResult discoverServices(const PickupRecord* pickups,int pickupCount,
                                        const GarageRecord* garages,int garageCount,bool enabled) {
    DiscoveryResult result;
    if(!enabled) return result;
    // Garage types encode service and bomb activation method. No coordinate guesses.
    if(garages && garageCount>=0 && garageCount<=32) for(int i=0;i<garageCount;++i) {
        const auto& g=garages[i]; if(!discoveryGarage(g)) continue;
        auto category=g.type==5?Spray:Bomb;
        Vec2 p{(g.left+g.right)*.5f,(g.front+g.back)*.5f};
        bool known=false;
        for(const auto& l:locations) if(l.category==category && l.position.x>=g.left && l.position.x<=g.right &&
            l.position.y>=g.front && l.position.y<=g.back) { known=true;break; }
        if(known || discoveryDuplicate(result,category,p)) continue;
        int bomb=g.type==2?11:g.type==4?12:10;
        addDiscovered(result,{{g.type==5?"Discovered Pay n Spray":"Discovered bomb shop",category,p},
            g.deactivated?Availability::Disabled:Availability::Available,bomb});
    }
    // At least two distinct weapon models must be explicitly marked for sale
    // within fifteen units and on the same storey. Single uncertain sellers stay hidden.
    if(pickups && pickupCount>=0 && pickupCount<=336) for(int i=0;i<pickupCount;++i) {
        const auto& seed=pickups[i]; if(!discoveryWeapon(seed)) continue;
        Vec2 point{seed.x,seed.y}; if(discoveryDuplicate(result,OtherWeapons,point)) continue;
        unsigned models=0; int count=0; Vec2 sum{}; bool active=false,temporary=false;
        for(int j=0;j<pickupCount;++j) {
            const auto& p=pickups[j];
            if(!discoveryWeapon(p) || distanceSquared(point,{p.x,p.y})>225 || fabsf(p.z-seed.z)>3) continue;
            models|=1u<<(p.model-170); ++count; sum.x+=p.x;sum.y+=p.y;
            active|=p.type==1 && !p.removed; temporary|=p.type==1 && p.removed;
        }
        if(!count || !(models&(models-1))) continue;
        Vec2 centre{sum.x/count,sum.y/count};
        if(discoveryDuplicate(result,OtherWeapons,centre)) continue;
        addDiscovered(result,{{"Discovered weapon shop",OtherWeapons,centre},active?Availability::Available:
            temporary?Availability::Unknown:Availability::OutOfStock,0});
    }
    return result;
}
}
