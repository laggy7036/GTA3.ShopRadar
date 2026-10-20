#pragma once
#include "RenderStyle.h"
namespace shopradar {
// UNUSED legacy 16x16 assets retained for development; not offered or drawn.
// Original 16x16 art: bomb at left; key, clock or transmitter at right.
inline constexpr const char* bombPixels[3][16]={
 {"................","..........####..",".........##..##.",".........##..##.","..........####..","...........##...",".....###...##...","....##.....####.","..#####....##...",".#######........","#########.......","#########.......","#########.......",".#######........","..#####.........","................"},
 {"..........####..",".........##..##.","........##.#..##","........#..#...#","........#..###.#","........##....##",".....###.##..##.","....##....####..","..#####.........",".#######........","#########.......","#########.......","#########.......",".#######........","..#####.........","................"},
 {"...........#....","...........#....","...........#....","...........#....","..........######","..........#....#",".....###..#.##.#","....##....#.##.#","..#####...#....#",".#######..#....#","#########.######","#########.......","#########.......",".#######........","..#####.........","................"}
};
template<class Draw> inline void drawBombPixels(int type,const Rect& bounds,RGBA colour,Draw draw) {
    type=clamp(type,0,2);
    const float dx=(bounds.right-bounds.left)/16,dy=(bounds.bottom-bounds.top)/16;
    for(int y=0;y<16;++y) for(int x=0;x<16;) {
        if(bombPixels[type][y][x]!='#') { ++x; continue; }
        int begin=x; while(x<16 && bombPixels[type][y][x]=='#') ++x;
        draw(Rect{bounds.left+begin*dx,bounds.top+(y+1)*dy,bounds.left+x*dx,bounds.top+y*dy},colour);
    }
}
// Small activation badges overlay the lower-right quarter of the native logo.
inline constexpr const char* bombBadgePixels[3][8]={
 {".###....",".#.#....",".###....","..#.....","..#.....","..###...","..#.....","........"},
 {"..####..",".##..##.","##.#..##","#..#...#","#..###.#","##....##",".##..##.","..####.."},
 {"..#.....","..#.....","..#.....",".######.",".#....#.",".#.##.#.",".#.##.#.",".######."}
};
inline Rect bombBadgeBounds(const Rect& logo) {
    return {logo.left+(logo.right-logo.left)*.5f,logo.bottom,
            logo.right,logo.top+(logo.bottom-logo.top)*.5f};
}
inline int nativeIconSprite(int choice) {
    return choice>=10 || choice==6?2:choice<6?spriteIds[choice]:-1;
}
template<class Draw> inline void drawBombBadge(int type,const Rect& logo,RGBA colour,Draw draw) {
    type=clamp(type,0,2);
    const auto b=bombBadgeBounds(logo);
    const float dx=(b.right-b.left)/8,dy=(b.bottom-b.top)/8;
    draw(b,RGBA{5,9,14,colour.a});
    for(int y=0;y<8;++y) for(int x=0;x<8;) {
        if(bombBadgePixels[type][y][x]!='#') { ++x; continue; }
        int begin=x; while(x<8 && bombBadgePixels[type][y][x]=='#') ++x;
        draw(Rect{b.left+begin*dx,b.top+(y+1)*dy,b.left+x*dx,b.top+y*dy},colour);
    }
}
}
