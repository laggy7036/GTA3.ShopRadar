#include "../src/Policy.h"
#include "../src/Services.h"
#include "../src/RenderStyle.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
using namespace shopradar;
int main() {
    // Accent choices must preserve the pictogram's RGB and keep the frame
    // outside it at every supported size; this catches the dark-red regression.
    for(int height:{480,1080,1440,2160}) for(int size:{75,100,125})
        for(int colour=0;colour<6;++colour) for(int alpha:{0,64,128,255}) {
            auto style=iconStyle({400,300},height,size,colour,(unsigned char)alpha);
            assert(style.textureColour.r==255 && style.textureColour.g==255 && style.textureColour.b==255);
            assert(style.textureColour.a==alpha && style.frameColour.a==alpha);
            assert(style.framed==(colour!=0));
            assert(style.outline.left<style.frame.left && style.frame.left<style.texture.left);
            assert(style.outline.right>style.frame.right && style.frame.right>style.texture.right);
            assert(style.outline.top<style.texture.top && style.outline.bottom>style.texture.bottom);
            assert(fabsf((style.texture.right-style.texture.left)-16.0f*height/480*size/100)<.001f);
        }
    puts("PASS: original pictogram colours, accent geometry, alpha and all icon sizes/resolutions");
    Settings s;
    assert(opacity(s,AmmuNation,{0,0},{0,0}) == 255);
    assert(opacity(s,AmmuNation,{0,0},{250,0}) == 0);
    assert(opacity(s,AmmuNation,{0,0},{251,0}) == 0);
    auto fade = opacity(s,AmmuNation,{0,0},{240,0});
    assert(fade > 0 && fade < 255);
    assert(opacity(s,AmmuNation,{0,0},{249,0}) < fade);
    assert(opacity(s,AmmuNation,{0,0},{150,200}) == 0);
    s.categories[AmmuNation].enabled = false;
    assert(opacity(s,AmmuNation,{0,0},{0,0}) == 0);
    assert(opacity(s,Phil,{0,0},{0,0}) == 255);
    s.enabled = false;
    assert(opacity(s,Phil,{0,0},{0,0}) == 0);
    s = Settings{};
    s.radius = 50;
    assert(opacity(s,Phil,{0,0},{50,0}) == 0);
    s.radius = 1000;
    assert(opacity(s,Phil,{0,0},{999,0}) > 0);
    assert(opacity(s,Phil,{0,0},{1000,0}) == 0);
    assert(opacity(s,Phil,{0,0},{NAN,0}) == 0);
    assert(nativeMatches(locations[0],{1068,-400},20));
    assert(!nativeMatches(locations[0],{1068,-400},18));
    assert(!nativeMatches(locations[0],{0,0},20));
    for (auto& l : locations) assert(l.category < CategoryCount);
    // Sweep every supported radius and category through/beyond its fade band.
    unsigned sweeps=0;
    for(int radius=50;radius<=1000;radius+=50) {
        s=Settings{}; s.radius=radius;
        for(int c=0;c<CategoryCount;++c) {
            unsigned char previous=255;
            for(int step=0;step<=radius*4+4;++step) {
                float d=step*.25f;
                auto alpha=opacity(s,Category(c),{0,0},{d,0});
                assert(alpha<=previous); previous=alpha;
                assert(alpha==opacity(s,Category(c),{0,0},{0,-d}));
                if(d>=radius) assert(alpha==0);
                ++sweeps;
            }
        }
    }
    s=Settings{};
    assert(opacity(s,Phil,{INFINITY,0},{0,0})==0);
    assert(opacity(s,Phil,{0,0},{-INFINITY,0})==0);
    s.radius=-999; assert(opacity(s,Phil,{0,0},{50,0})==0);
    s.radius=999999; assert(opacity(s,Phil,{0,0},{1000,0})==0);
    for(auto& l:locations) {
        s=Settings{};
        assert(opacity(s,l.category,l.position,l.position)==255);
        int expectedSprite=l.category==Spray?18:l.category==Bomb?2:20;
        assert(nativeMatches(l,{l.position.x+35,l.position.y},expectedSprite));
        assert(!nativeMatches(l,{l.position.x+35.25f,l.position.y},expectedSprite));
        assert(!nativeMatches(l,{NAN,0},expectedSprite));
    }
    printf("PASS: %u distance sweep samples across every radius/category\n",sweeps);
    s=Settings{};
    for(int radius:{50,100,250,500,1000}) for(int percent=25;percent<=100;percent+=5) {
        s.radius=radius;s.missionPercent=percent;
        assert(fabsf(missionDistance(s)-radius*percent/100.0f)<.001f);
        assert(serviceOpacity(s,Spray,{0,0},{missionDistance(s),0},true,Availability::Available)==0);
        assert(serviceOpacity(s,Spray,{0,0},{missionDistance(s)*.8f,0},true,Availability::Available)==255);
    }
    s=Settings{};
    for(int state=0;state<4;++state) {
        auto availability=Availability(state);
        assert((serviceOpacity(s,AmmuNation,{0,0},{0,0},true,availability)>0)==(availability==Availability::Available));
        assert(serviceOpacity(s,AmmuNation,{0,0},{missionDistance(s),0},true,availability)==0);
    }
    assert(serviceOpacity(s,AmmuNation,{0,0},{74,0},true,Availability::Available)>0);
    assert(serviceOpacity(s,AmmuNation,{0,0},{100,0},false,Availability::Unknown)>0);
    s.radius=50; assert(serviceOpacity(s,AmmuNation,{0,0},{50,0},true,Availability::Available)==0);
    PickupRecord stock[3]{};
    stock[0].x=locations[0].position.x; stock[0].y=locations[0].position.y;
    stock[0].type=14; assert(weaponStock(stock,3,locations[0])==Availability::Disabled);
    stock[0].type=6; assert(weaponStock(stock,3,locations[0])==Availability::OutOfStock);
    stock[0].type=1; stock[0].removed=1; assert(weaponStock(stock,3,locations[0])==Availability::Unknown);
    stock[0].removed=0; assert(weaponStock(stock,3,locations[0])==Availability::Available);
    stock[0].object=0; assert(weaponStock(stock,3,locations[0])==Availability::Available);
    stock[0].x=NAN; assert(weaponStock(stock,3,locations[0])==Availability::Disabled);
    // Mixed stock must not let one sold-out item hide the whole open shop,
    // nor let a nearby loose pickup imply that a locked shop is open.
    for(int i=0;i<3;++i) {
        stock[i]={}; stock[i].x=locations[0].position.x; stock[i].y=locations[0].position.y;
    }
    stock[0].type=6; stock[1].type=1;
    assert(weaponStock(stock,3,locations[0])==Availability::Available);
    stock[1].removed=1;
    assert(weaponStock(stock,3,locations[0])==Availability::Unknown);
    stock[1].type=2; stock[2].type=14;
    assert(weaponStock(stock,3,locations[0])==Availability::OutOfStock);
    stock[0].type=0;
    assert(weaponStock(stock,3,locations[0])==Availability::Disabled);
    stock[0].type=1; stock[0].x+=15.25f;
    assert(weaponStock(stock,3,locations[0])==Availability::Disabled);
    stock[0].x=locations[0].position.x+15;
    assert(weaponStock(stock,3,locations[0])==Availability::Available);
    assert(weaponStock(stock,3,locations[1])==Availability::Disabled);
    // All categories honour the mission availability gate and radius cap,
    // including malformed INI radius values and master/category toggles.
    for(int c=0;c<CategoryCount;++c) for(int normal:{-1,50,250,1000,99999})
        for(int nearby:{-1,50,75,500,99999}) {
            Settings view; view.radius=normal; view.missionPercent=nearby;
            for(auto state:{Availability::Unknown,Availability::Disabled,Availability::OutOfStock})
                assert(serviceOpacity(view,Category(c),{0,0},{0,0},true,state)==0);
            assert(serviceOpacity(view,Category(c),{0,0},{0,0},true,Availability::Available)==255);
            assert(serviceOpacity(view,Category(c),{0,0},{float(clamp(normal,50,1000)),0},true,Availability::Available)==0);
            assert(serviceOpacity(view,Category(c),{0,0},{missionDistance(view),0},true,Availability::Available)==0);
            view.categories[c].enabled=false;
            assert(serviceOpacity(view,Category(c),{0,0},{0,0},true,Availability::Available)==0);
            view.categories[c].enabled=true; view.enabled=false;
            assert(serviceOpacity(view,Category(c),{0,0},{0,0},true,Availability::Available)==0);
        }
    GarageRecord garages[2]{};
    garages[0].type=5; garages[0].left=920; garages[0].right=929; garages[0].front=-368; garages[0].back=-355;
    garages[0].state=1; assert(garageService(garages,2,locations[3])==Availability::Available);
    garages[0].deactivated=1; assert(garageService(garages,2,locations[3])==Availability::Disabled);
    assert(garageService(garages,2,locations[6])==Availability::Unknown);
    puts("PASS: mission proximity, unknown/disabled/out-of-stock, shop-only stock, garage activation");
    puts("PASS: radius, fade, diagonal distance, toggles, bounds and native duplicate matching");
}
