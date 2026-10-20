#pragma once
#include "Policy.h"
#include <stdint.h>
namespace shopradar {
struct PickupRecord {
    uint8_t type,removed; uint16_t quantity; uint32_t object,timer;
    uint16_t model,reference; float x,y,z;
};
static_assert(sizeof(PickupRecord)==28);
inline Availability weaponStock(const PickupRecord* pool,int count,const Location& shop) {
    bool empty=false,temporary=false;
    for(int i=0;i<count;++i) {
        auto& p=pool[i];
        if((p.type!=1 && p.type!=6) || distanceSquared(shop.position,{p.x,p.y})>225.0f ||
           !isfinite(p.x) || !isfinite(p.y)) continue;
        if(p.type==1 && !p.removed) return Availability::Available;
        if(p.type==6) empty=true; else temporary=true;
    }
    if(temporary) return Availability::Unknown;
    return empty?Availability::OutOfStock:Availability::Disabled;
}
struct GarageRecord {
    uint8_t type,state,field2,lockup,deactivated,resprayed;
    uint8_t reserved[22];
    float left,right,front,back,down,up;
    uint8_t tail[88];
};
static_assert(sizeof(GarageRecord)==140);
inline Availability garageService(const GarageRecord* garages,int count,const Location& shop) {
    for(int i=0;i<count;++i) {
        auto& g=garages[i];
        bool type=shop.category==Spray?g.type==5:g.type>=2&&g.type<=4;
        if(type && shop.position.x>=g.left && shop.position.x<=g.right &&
                   shop.position.y>=g.front && shop.position.y<=g.back)
            return g.deactivated?Availability::Disabled:Availability::Available;
    }
    return Availability::Unknown;
}
}
