#pragma once
#include "Policy.h"
namespace shopradar {
inline bool radarSlotClear(Vec2 p,const Vec2* used,int count,float spacing) {
    for(int i=0;i<count;++i) if(fabsf(p.x-used[i].x)<spacing && fabsf(p.y-used[i].y)<spacing) return false;
    return true;
}
// Accuracy first: never search distant free slots. A small continuous nudge can
// reveal overlapping edges, but cannot move a marker by a whole icon width.
inline Vec2 radarSlot(Vec2 anchor,const Vec2* used,int count,float spacing,Vec2 centre,Vec2 radii) {
    if(spacing<=0 || radii.x<=0 || radii.y<=0) return anchor;
    Vec2 push{};float strength=0;
    for(int i=0;i<count;++i) {
        float dx=anchor.x-used[i].x,dy=anchor.y-used[i].y;
        float overlap=fminf(spacing-fabsf(dx),spacing-fabsf(dy));
        if(overlap<=0) continue;
        float length=sqrtf(dx*dx+dy*dy);
        if(length<.001f) {dx=0;dy=-1;length=1;}
        float weight=overlap/spacing;
        push.x+=dx/length*weight;push.y+=dy/length*weight;
        strength=fmaxf(strength,weight);
    }
    float length=sqrtf(push.x*push.x+push.y*push.y);
    if(length<.001f) return anchor;
    float amount=spacing*.12f*strength;
    Vec2 result{anchor.x+push.x/length*amount,anchor.y+push.y/length*amount};
    float x=(result.x-centre.x)/radii.x,y=(result.y-centre.y)/radii.y;
    return x*x+y*y<=1?result:anchor;
}
}
