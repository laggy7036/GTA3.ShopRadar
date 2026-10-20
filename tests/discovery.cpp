#include "../src/Discovery.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
using namespace shopradar;
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static GarageRecord garage(float x,float y,unsigned char type=5) {
    GarageRecord g{};g.type=type;g.left=x-5;g.right=x+5;g.front=y-5;g.back=y+5;g.down=0;g.up=5;return g;
}
static PickupRecord weapon(float x,float y,unsigned short model,unsigned char type=1,float z=0) {
    PickupRecord p{};p.x=x;p.y=y;p.z=z;p.model=model;p.type=type;return p;
}
int main() {
    const Vec2 known=locations[3].position;
    GarageRecord g[4]={garage(5000,5000),garage(5000,5000),garage(known.x,known.y),garage(5000,5000,3)};
    auto r=discoverServices(nullptr,0,g,4,true);
    CHECK(r.count==2);CHECK(r.entries[0].location.category==Spray);CHECK(r.entries[1].location.category==Bomb);
    CHECK(r.entries[1].bombIcon==10);CHECK(r.entries[0].state==Availability::Available);
    CHECK(discoverServices(nullptr,0,g,4,false).count==0);
    for(int type=2;type<=4;++type) {g[0]=garage(6000,6000,type);r=discoverServices(nullptr,0,g,1,true);CHECK(r.count==1);CHECK(r.entries[0].bombIcon==(type==2?11:type==3?10:12));}
    g[0]=garage(5000,5000);g[0].deactivated=1;
    r=discoverServices(nullptr,0,g,1,true);CHECK(r.count==1 && r.entries[0].state==Availability::Disabled);
    for(int type:{0,1,7,13,14,16,21,255}) {g[0]=garage(5000,5000,type);CHECK(discoverServices(nullptr,0,g,1,true).count==0);}
    g[0]=garage(5000,5000);g[0].left=NAN;CHECK(discoverServices(nullptr,0,g,1,true).count==0);
    g[0]=garage(5000,5000);g[0].right=INFINITY;CHECK(discoverServices(nullptr,0,g,1,true).count==0);
    g[0]=garage(5000,5000);g[0].up=-1;CHECK(discoverServices(nullptr,0,g,1,true).count==0);
    g[0]=garage(5000,5000);g[0].right=g[0].left+100;CHECK(discoverServices(nullptr,0,g,1,true).count==0);
    CHECK(discoverServices(nullptr,0,g,33,true).count==0);
    PickupRecord p[4]={weapon(5000,5000,173),weapon(5002,5000,178),weapon(5010,5000,176),{}};
    r=discoverServices(p,4,nullptr,0,true);CHECK(r.count==1);CHECK(r.entries[0].state==Availability::Available);
    CHECK(r.entries[0].location.category==OtherWeapons);
    CHECK(discoverServices(p,1,nullptr,0,true).count==0);
    p[1].model=173;p[2].type=0;CHECK(discoverServices(p,4,nullptr,0,true).count==0);
    for(int type:{0,2,3,4,5,7,8,9,10,11,12,13,14,255}) {
        p[0]=weapon(5000,5000,173,type);p[1]=weapon(5002,5000,178,type);
        CHECK(discoverServices(p,2,nullptr,0,true).count==0);
    }
    p[0]=weapon(5000,5000,173);p[1]=weapon(5002,5000,178,1,10);
    CHECK(discoverServices(p,2,nullptr,0,true).count==0); // Separate storeys.
    p[1]=weapon(5050,5000,178);CHECK(discoverServices(p,2,nullptr,0,true).count==0);
    p[1]=weapon(5002,5000,179);CHECK(discoverServices(p,2,nullptr,0,true).count==0); // Projectile.
    p[1]=weapon(NAN,5000,178);CHECK(discoverServices(p,2,nullptr,0,true).count==0);
    p[1]=weapon(5002,5000,178);p[0].removed=p[1].removed=1;
    r=discoverServices(p,2,nullptr,0,true);CHECK(r.count==1 && r.entries[0].state==Availability::Unknown);
    p[0].type=p[1].type=6;
    r=discoverServices(p,2,nullptr,0,true);CHECK(r.count==1 && r.entries[0].state==Availability::OutOfStock);
    Settings settings;
    CHECK(serviceOpacity(settings,OtherWeapons,{5000,5000},r.entries[0].location.position,true,r.entries[0].state)==0);
    for(const auto& l:locations) if(l.category==AmmuNation || l.category==Phil || l.category==OtherWeapons) {
        p[0]=weapon(l.position.x,l.position.y,173);p[1]=weapon(l.position.x+2,l.position.y,178);
        CHECK(discoverServices(p,2,nullptr,0,true).count==0);
    }
    PickupRecord many[336]{};GarageRecord garages[32]{};
    for(int i=0;i<168;++i) {many[i*2]=weapon(5000+i*100,5000,173);many[i*2+1]=weapon(5002+i*100,5000,178);}
    for(int i=0;i<32;++i) garages[i]=garage(5000+i*100,8000);
    r=discoverServices(many,336,garages,32,true);
    CHECK(r.count==maxDiscoveredServices && r.full);
    CHECK(discoverServices(many,337,nullptr,0,true).count==0);
    CHECK(knownServiceCount==10 && locations[3].position.x==known.x && locations[3].position.y==known.y);
    for(int radius:{50,250,1000}) for(int percent:{25,60,100}) {
        settings.radius=radius;settings.missionPercent=percent;
        CHECK(serviceOpacity(settings,Spray,{0,0},{0,0},true,Availability::Available)==255);
        CHECK(serviceOpacity(settings,Spray,{0,0},{missionDistance(settings),0},true,Availability::Available)==0);
        settings.categories[Spray].enabled=false;
        CHECK(serviceOpacity(settings,Spray,{0,0},{0,0},true,Availability::Available)==0);settings.categories[Spray].enabled=true;
    }
    printf("PASS: %u hybrid discovery checks (shop-only evidence, duplicates, activation, malformed records, capacity)\n",checks);
}
