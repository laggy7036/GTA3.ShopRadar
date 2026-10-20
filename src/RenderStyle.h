#pragma once
#include "Policy.h"
namespace shopradar {
struct RGBA { unsigned char r,g,b,a; };
struct Rect { float left,bottom,right,top; };
struct IconStyle {
    Rect texture,frame,outline,separator;
    RGBA textureColour,frameColour;
    bool framed;
};
inline IconStyle iconStyle(Vec2 centre,int height,int size,int colour,unsigned char alpha) {
    constexpr RGBA palette[]={{255,255,255,255},{67,218,103,255},{184,119,240,255},
        {255,207,66,255},{74,220,238,255},{255,126,112,255}};
    float scale=(height>0?height:480)/480.0f;
    float half=8.0f*scale*clamp(size,75,125)/100.0f;
    float line=scale<1?1:scale;
    auto bounds=[&](float h) { return Rect{centre.x-h,centre.y+h,centre.x+h,centre.y-h}; };
    auto accent=palette[clamp(colour,0,5)]; accent.a=alpha;
    return {bounds(half),bounds(half+line*(colour==5?2:1)),bounds(half+line*(colour==5?3:2)),bounds(half+line),
        {255,255,255,alpha},accent,colour!=0};
}
}
