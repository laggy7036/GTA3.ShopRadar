#pragma once
#include "RenderStyle.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace shopradar {
struct PreviewImage { unsigned width=0,height=0; RGBA pixels[64*64]{}; };
inline uint32_t previewU32(const unsigned char* p) { uint32_t n; memcpy(&n,p,4); return n; }
inline bool decodeHudPreview(const unsigned char* p,size_t size,const char* name,PreviewImage& out) {
    out.width=out.height=0;
    // Classic PC D3D8, uncompressed PAL8. Reject unsupported replacements safely.
    if(size<88+1024+4 || previewU32(p)!=8 || !memchr(p+8,0,32) || strcmp((const char*)p+8,name)) return false;
    unsigned w=p[80]|unsigned(p[81])<<8,h=p[82]|unsigned(p[83])<<8;
    if(!w || !h || w>64 || h>64 || p[84]!=8 || !p[85] || p[87]!=0 ||
       (previewU32(p+72)&0x6000)!=0x2000) return false;
    size_t count=size_t(w)*h;
    if(previewU32(p+1112)<count || size-1116<count) return false;
    for(size_t i=0;i<count;++i) {
        auto c=p+88+size_t(p[1116+i])*4;
        out.pixels[i]={c[0],c[1],c[2],c[3]};
    }
    out.width=w;out.height=h;return true;
}
inline bool findHudPreview(const unsigned char* bytes,size_t size,const char* name,PreviewImage& out) {
    out.width=out.height=0;
    if(size<12 || previewU32(bytes)!=0x16 || previewU32(bytes+4)>size-12) return false;
    size_t end=12+previewU32(bytes+4);
    for(size_t pos=12;pos+12<=end;) {
        size_t n=previewU32(bytes+pos+4),start=pos+12;
        if(n>end-start) return false;
        if(previewU32(bytes+pos)==0x15 && n>=12 && previewU32(bytes+start)==1) {
            size_t native=previewU32(bytes+start+4);
            if(native>n-12) return false;
            if(decodeHudPreview(bytes+start+12,native,name,out)) return true;
        }
        pos=start+n;
    }
    return false;
}
template<class Draw> inline void drawHudPreview(const PreviewImage& image,const Rect& bounds,unsigned char alpha,Draw draw) {
    if(!image.width || !image.height) return;
    float dx=(bounds.right-bounds.left)/image.width,dy=(bounds.bottom-bounds.top)/image.height;
    for(unsigned y=0;y<image.height;++y) for(unsigned x=0;x<image.width;) {
        auto c=image.pixels[y*image.width+x]; unsigned start=x++;
        while(x<image.width && !memcmp(&c,&image.pixels[y*image.width+x],sizeof(c))) ++x;
        c.a=unsigned(c.a)*alpha/255;
        if(c.a) draw(Rect{bounds.left+start*dx,bounds.top+(y+1)*dy,bounds.left+x*dx,bounds.top+y*dy},c);
    }
}
}
