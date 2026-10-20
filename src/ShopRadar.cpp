// GTA III ABI addresses and layouts researched using Plugin-SDK.
// See THIRD_PARTY.md; this bridge is independently implemented.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Policy.h"
#include "Services.h"
#include "RenderStyle.h"
#include "BombIcons.h"
#include "HudPreview.h"
#include "RadarLayout.h"
#include "Discovery.h"
// build.ps1 generates version.h from the VERSION file.
#if __has_include("version.h")
#include "version.h"
#else
#define SHOPRADAR_VERSION "dev"
#endif

using namespace shopradar;
static Settings settings;
// Wide paths keep the INI/log usable when the install path has non-ANSI characters.
static wchar_t iniPath[MAX_PATH], logPath[MAX_PATH];
static HMODULE module;
static int version = -1;
static bool menuOpen, keys[256], saveFailed;
static int row, column, releaseFrames;
static bool down[256], pressed[256];
static bool queuedPress[256], recoveredInputLogged;
static POINT queuedClick,frameClick;
static bool frameClickValid;
static WNDPROC previousWindowProc;
static HWND inputWindow;
static int windowWidth=640, windowHeight=448;
static uintptr_t fontDetails;
static uintptr_t garagePool,radarSuppressed,radarEnabled;
static bool vehicleAbi;
static Availability services[knownServiceCount];
static DiscoveryResult discovered;
static bool discoveryLimitLogged;
static DWORD stockChecked;
static int v(int a,int b,int c) { return version==0?a:version==1?b:c; }
template<class T> static T fn(uintptr_t address) { return reinterpret_cast<T>(address); }
static void log(const char* text) {
    FILE* f=_wfopen(logPath,L"a"); if(f) { fprintf(f,"%s\n",text); fclose(f); }
}
static bool accessible(uintptr_t address, size_t length, bool executable=false) {
    MEMORY_BASIC_INFORMATION m{};
    if(length>UINTPTR_MAX-address) return false;
    if(!VirtualQuery((void*)address,&m,sizeof(m)) || m.State!=MEM_COMMIT ||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) || address<(uintptr_t)m.BaseAddress ||
       length>m.RegionSize-(address-(uintptr_t)m.BaseAddress)) return false;
    return !executable || (m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
static bool widen(const char* text,wchar_t* out,int size) {
    return MultiByteToWideChar(CP_UTF8,0,text,-1,out,size)>0;
}
static bool writeProfile(const char* section,const char* key,const char* value) {
    wchar_t s[64],k[64],v[64];
    if(!widen(section,s,64) || !widen(key,k,64) || (value && !widen(value,v,64))) return false;
    return WritePrivateProfileStringW(s,k,value?v:nullptr,iniPath)!=FALSE;
}
static int readInt(const char* section,const char* key,int fallback,int lo,int hi) {
    wchar_t s[64],k[64],text[64],*end;
    if(!widen(section,s,64) || !widen(key,k,64)) return fallback;
    GetPrivateProfileStringW(s,k,L"",text,64,iniPath);
    if(!text[0]) return fallback;
    long value=wcstol(text,&end,10);
    if(end==text || *end) return fallback;
    return value<lo?lo:value>hi?hi:int(value);
}
static void loadSettings() {
    const Settings defaults;
    settings.enabled=readInt("General","Enabled",1,0,1)!=0;
    settings.dynamicDiscovery=readInt("General","DynamicDiscovery",1,0,1)!=0;
    settings.radius=readInt("General","Radius",250,50,1000);
    settings.missionPercent=((readInt("General","MissionRadiusPercent",70,25,100)+2)/5)*5;
    settings.iconSize=((readInt("General","IconSize",75,75,125)+12)/25)*25;
    for(int i=0;i<CategoryCount;++i) {
        auto& s=settings.categories[i];
        s.enabled=readInt(categoryNames[i],"Enabled",1,0,1)!=0;
        s.icon=activeIcon(readInt(categoryNames[i],"Icon",defaults.categories[i].icon,0,iconCount-1));
        s.colour=readInt(categoryNames[i],"Colour",0,0,5);
    }
}
static bool writeInt(const char* section,const char* key,int value) {
    char text[32]; snprintf(text,sizeof(text),"%d",value);
    return writeProfile(section,key,text);
}
static void saveSettings() {
    bool ok=writeInt("General","Enabled",settings.enabled);
    ok=writeInt("General","DynamicDiscovery",settings.dynamicDiscovery)&&ok;
    ok=writeInt("General","Radius",settings.radius)&&ok;
    ok=writeInt("General","MissionRadiusPercent",settings.missionPercent)&&ok;
    ok=writeInt("General","IconSize",settings.iconSize)&&ok;
    for(int i=0;i<CategoryCount;++i) {
        auto& s=settings.categories[i];
        ok=writeInt(categoryNames[i],"Enabled",s.enabled)&&ok;
        ok=writeInt(categoryNames[i],"Icon",s.icon)&&ok;
        ok=writeInt(categoryNames[i],"Colour",s.colour)&&ok;
    }
    saveFailed=!ok;
    if(!ok) log("Could not save settings INI; current choices still apply this session.");
}
struct Trace {
    unsigned colour,type; int handle; Vec2 pos2; float x,y,z;
    unsigned short index; bool bright,inUse; float sphere;
    unsigned short size,display,sprite,pad;
};
static_assert(sizeof(Trace)==0x30);
static_assert(sizeof(void*)==4,"Build as x86");
static void updateWindowSize() {
    HWND window=GetForegroundWindow(); DWORD pid=0;
    GetWindowThreadProcessId(window,&pid);
    RECT rect{};
    if(pid==GetCurrentProcessId() && GetClientRect(window,&rect) && rect.right>0 && rect.bottom>0) {
        windowWidth=rect.right; windowHeight=rect.bottom;
    }
}
static void rect(float x,float y,float w,float h,RGBA colour) {
    float sx=windowWidth/640.0f, sy=windowHeight/448.0f;
    Rect r{x*sx,(y+h)*sy,(x+w)*sx,y*sy};
    fn<void(__cdecl*)(const Rect&,const RGBA&)>(v(0x51F970,0x51FBA0,0x51FB30))(r,colour);
}
static uintptr_t fontAddress(uintptr_t a) { return a+(version==0?0:version==1?0xE0:0x70); }
static void text(float x,float y,const char* string,RGBA colour={240,240,240,255},float scale=.36f,float height=.73f) {
    wchar_t wide[256];
    if(!MultiByteToWideChar(CP_UTF8,0,string,-1,wide,256)) return; // Too long or invalid: draw nothing.
    fn<void(__cdecl*)(float,float)>(fontAddress(0x501B80))(scale*windowWidth/640.0f,height*windowHeight/448.0f);
    fn<void(__cdecl*)(float)>(fontAddress(0x501BC0))(0.0f);
    fn<void(__cdecl*)(float)>(fontAddress(0x501DD0))(255.0f);
    // SetColor applies the current alpha fade, so establish it first.
    // The game's ABI takes a colour pointer, despite the SDK's public value API.
    fn<void(__cdecl*)(const RGBA*)>(fontAddress(0x501BD0))(&colour);
    fn<void(__cdecl*)(short)>(fontAddress(0x501E70))(0);
    fn<void(__cdecl*)()>(fontAddress(0x501C80))(); // justify off
    fn<void(__cdecl*)()>(fontAddress(0x501CB0))(); // centre off
    fn<void(__cdecl*)()>(fontAddress(0x501D70))(); // right justify off
    fn<void(__cdecl*)()>(fontAddress(0x501CF0))(); // background off
    fn<void(__cdecl*)()>(fontAddress(0x501DA0))(); // proportional
    fn<void(__cdecl*)(float)>(fontAddress(0x501CC0))(float(windowWidth));
    fn<void(__cdecl*)(short)>(fontAddress(0x501DB0))(0);
    fn<void(__cdecl*)(float,float,const wchar_t*)>(fontAddress(0x500F50))(x*windowWidth/640.0f,y*windowHeight/448.0f,wide);
}
static PreviewImage menuPreviewImages[6];
static bool menuPreviewsTried;
static void loadMenuPreviews() {
    if(menuPreviewsTried) return;
    menuPreviewsTried=true;
    wchar_t path[MAX_PATH];
    DWORD length=GetModuleFileNameW(nullptr,path,MAX_PATH);
    if(!length || length>=MAX_PATH) return;
    auto slash=wcsrchr(path,L'\\'); if(!slash || size_t(slash-path)+16>=MAX_PATH) return;
    wcscpy(slash+1,L"models\\hud.txd");
    FILE* file=_wfopen(path,L"rb"); if(!file) { log("Menu HUD preview file unavailable."); return; }
    if(fseek(file,0,SEEK_END)) { fclose(file); return; }
    long lengthBytes=ftell(file);
    if(lengthBytes<12 || lengthBytes>8*1024*1024 || fseek(file,0,SEEK_SET)) { fclose(file); return; }
    auto bytes=(unsigned char*)malloc(size_t(lengthBytes));
    if(!bytes) { fclose(file); return; }
    size_t read=fread(bytes,1,size_t(lengthBytes),file); fclose(file);
    const char* names[]={"radar_weapon","radar_spray","radar_bomb","radar_eight","radar_save","radar_cat"};
    unsigned loaded=0;
    if(read==size_t(lengthBytes)) for(int i=0;i<6;++i) loaded+=findHudPreview(bytes,read,names[i],menuPreviewImages[i]);
    free(bytes);
    log(loaded==6?"All six menu HUD previews loaded from local hud.txd.":"Some menu HUD previews unavailable: unsupported or missing textures.");
}
static void icon(int choice,int colour,Vec2 screen,unsigned char alpha,bool preview=false) {
    choice=clamp(choice,0,iconCount-1);
    if(choice>=7 && choice<=9) choice=6; // Retired art; IDs10-12 are internal auto variants.
    if(choice==6) choice=10; // Automatic menu preview.
    auto sprites=(void**)v(0x5F6CA4,0x5F6A8C,0x603A84);
    int sprite=nativeIconSprite(choice);
    bool live=sprite>=0 && accessible((uintptr_t)(sprites+sprite),sizeof(void*)) && sprites[sprite] &&
        accessible((uintptr_t)sprites[sprite],sizeof(void*)) && *(void**)sprites[sprite];
    const PreviewImage* cached=nullptr;
    if(!live) {
        if(!preview) return;
        loadMenuPreviews();
        for(int i=0;i<6;++i) if(spriteIds[i]==sprite && menuPreviewImages[i].width) cached=&menuPreviewImages[i];
        if(!cached) return;
    }
    auto style=iconStyle(screen,windowHeight,settings.iconSize,colour,alpha);
    if(style.framed) {
        RGBA dark{5,9,14,alpha};
        auto drawRect=fn<void(__cdecl*)(const Rect&,const RGBA&)>(v(0x51F970,0x51FBA0,0x51FB30));
        drawRect(style.outline,dark); drawRect(style.frame,style.frameColour);
        if(colour==5) drawRect(style.separator,dark); // Separate red artwork from its red frame.
    }
    // Keep the original pictogram highlights: colour belongs to the frame.
    if(live) fn<void(__thiscall*)(void*,const Rect&,const RGBA&)>(v(0x51ED50,0x51EF80,0x51EF10))(sprites[sprite],style.texture,style.textureColour);
    else drawHudPreview(*cached,style.texture,alpha,
        fn<void(__cdecl*)(const Rect&,const RGBA&)>(v(0x51F970,0x51FBA0,0x51FB30)));
    if(choice>=10) {
        auto drawRect=fn<void(__cdecl*)(const Rect&,const RGBA&)>(v(0x51F970,0x51FBA0,0x51FB30));
        drawBombBadge(choice-10,style.texture,style.textureColour,drawRect);
    }
}
using RadarFn=void(__cdecl*)();
static RadarFn originalRadar;
using SpriteFn=void(__cdecl*)(unsigned short,float,float,unsigned char);
static SpriteFn originalSprites[4];
static bool filterNativeShops;
static void nativeSprite(int index,unsigned short sprite,float x,float y,unsigned char alpha) {
    if(filterNativeShops && (sprite==2 || sprite==18 || sprite==20)) return;
    originalSprites[index](sprite,x,y,alpha);
}
static void __cdecl nativeSprite0(unsigned short s,float x,float y,unsigned char a) { nativeSprite(0,s,x,y,a); }
static void __cdecl nativeSprite1(unsigned short s,float x,float y,unsigned char a) { nativeSprite(1,s,x,y,a); }
static void __cdecl nativeSprite2(unsigned short s,float x,float y,unsigned char a) { nativeSprite(2,s,x,y,a); }
static void __cdecl nativeSprite3(unsigned short s,float x,float y,unsigned char a) { nativeSprite(3,s,x,y,a); }
static void refreshServices() {
    DWORD now=GetTickCount();
    if(stockChecked && DWORD(now-stockChecked)<250) return;
    stockChecked=now;
    auto pool=(PickupRecord*)v(0x878C98,0x878C48,0x888D88);
    bool valid=accessible((uintptr_t)pool,336*sizeof(PickupRecord));
    if(valid) for(int i=0;i<336;++i) if(pool[i].type>14) { valid=false; break; }
    bool garagesValid=garagePool && accessible(garagePool,32*sizeof(GarageRecord));
    for(int i=0;i<knownServiceCount;++i) {
        auto& l=locations[i];
        if(l.category==Spray || l.category==Bomb) {
            services[i]=garagesValid?
                garageService((GarageRecord*)garagePool,32,l):Availability::Unknown;
        } else services[i]=valid?weaponStock(pool,336,l):Availability::Unknown;
    }
    discovered=discoverServices(valid?pool:nullptr,valid?336:0,garagesValid?(GarageRecord*)garagePool:nullptr,
                                garagesValid?32:0,settings.dynamicDiscovery);
    if(discovered.full && !discoveryLimitLogged) { log("Dynamic service capacity reached; extra candidates skipped."); discoveryLimitLogged=true; }
}
// Vehicle fields read for mission service checks (Plugin-SDK CVehicle/CAutomobile layout;
// only used when vehicleAbi is true, i.e. on the validated Steam executable).
constexpr size_t vehicleHealth=0x200;       // CVehicle::m_fHealth (float); 0 or less = wrecked.
constexpr size_t vehicleClass=0x284;        // CVehicle::m_nVehicleClass; 0 = automobile (not boat, train, heli, plane).
constexpr size_t automobileFlags=0x4D9;     // CAutomobile::m_nAutomobileFlags bitfield.
constexpr unsigned char bombTypeMask=7;     // Bits 0-2: nBombType; non-zero = a bomb is already fitted.
constexpr unsigned char fixedColourBit=32;  // Bit 5: bFixedColour; Pay 'n' Spray cannot respray it.
static Availability currentService(Availability state,Category category,bool mission) {
    if(!mission || state!=Availability::Available || (category!=Spray && category!=Bomb)) return state;
    // On-foot markers describe an active service. In a vehicle, exclude rejections.
    if(!vehicleAbi) return Availability::Unknown;
    auto car=(unsigned char*)fn<void*(__cdecl*)()>(0x4A1140)();
    if(!car) return state;
    if(!accessible((uintptr_t)car,0x4DC)) return Availability::Unknown;
    if(*(uint32_t*)(car+vehicleClass)!=0 || *(float*)(car+vehicleHealth)<=0) return Availability::Disabled;
    if(category==Bomb && (car[automobileFlags]&bombTypeMask)) return Availability::Disabled;
    // 0x426700 = CGarages::IsCarSprayable (Steam address).
    if(category==Spray && ((car[automobileFlags]&fixedColourBit) || !fn<bool(__cdecl*)(void*)>(0x426700)(car))) return Availability::Disabled;
    return state;
}
static void __cdecl drawRadar() {
    if(!settings.enabled) { originalRadar(); return; }
    if((radarSuppressed && *(uint8_t*)radarSuppressed==1) ||
       (radarEnabled && !*(uint8_t*)radarEnabled)) { originalRadar(); return; }
    auto origin=*(Vec2*)v(0x6299B8,0x6299B8,0x6399B8);
    auto traces=(Trace*)v(0x6ED5E0,0x6ED5E0,0x6FD720);
    const bool mission=fn<bool(__cdecl*)()>(0x439410)();
    // Some original shop icons are drawn directly from garages, not radar traces.
    // Filter their sprite draw calls, never modify the game's radar/save state.
    refreshServices();
    updateWindowSize();
    Vec2 used[knownServiceCount+maxDiscoveredServices]; int usedCount=0;
    auto toScreen=fn<void(__cdecl*)(Vec2&,const Vec2&)>(v(0x4A5040,0x4A5130,0x4A50C0));
    Vec2 centre{},right{},top{};
    toScreen(centre,{0,0}); toScreen(right,{1,0}); toScreen(top,{0,1});
    Vec2 radii{fabsf(right.x-centre.x),fabsf(top.y-centre.y)};
    // Largest colour frame sets separation, so changing colour cannot re-stack icons.
    auto footprint=iconStyle({0,0},windowHeight,settings.iconSize,5,255);
    float spacing=footprint.outline.right-footprint.outline.left+2*windowHeight/480.0f;
    for(int locationIndex=0;locationIndex<knownServiceCount+(settings.dynamicDiscovery?discovered.count:0);++locationIndex) {
        bool extra=locationIndex>=knownServiceCount;
        auto& dynamic=discovered.entries[extra?locationIndex-knownServiceCount:0];
        const auto& location=extra?dynamic.location:locations[locationIndex];
        auto state=extra?dynamic.state:services[locationIndex];
        auto alpha=serviceOpacity(settings,location.category,origin,location.position,mission,currentService(state,location.category,mission));
        if(!alpha) continue;
        // Existing mission markers always retain priority, regardless of settings.
        bool existing=false;
        for(int i=0;i<32;++i) {
            auto& t=traces[i];
            if(t.inUse && (t.type==4 || t.type==5) && (t.display==2 || t.display==3) &&
               nativeMatches(location,{t.x,t.y},t.sprite)) { existing=true; break; }
        }
        if(mission && existing) continue;
        Vec2 radar{},screen{};
        fn<void(__cdecl*)(Vec2&,const Vec2&)>(v(0x4A50D0,0x4A51C0,0x4A5150))(radar,location.position);
        fn<float(__cdecl*)(Vec2&)>(v(0x4A4F30,0x4A5020,0x4A4FB0))(radar);
        fn<void(__cdecl*)(Vec2&,const Vec2&)>(v(0x4A5040,0x4A5130,0x4A50C0))(screen,radar);
        screen=radarSlot(screen,used,usedCount,spacing,centre,radii);
        used[usedCount++]=screen;
        auto& config=settings.categories[location.category];
        int choice=extra && location.category==Bomb && config.icon==6?dynamic.bombIcon:locationIcon(config.icon,locationIndex);
        icon(choice,config.colour,screen,alpha);
    }
    // Original sprites AND coordinate/entity blips are drawn last, above our icons.
    filterNativeShops=!mission;
    originalRadar();
    filterNativeShops=false;
}
static bool focused() {
    DWORD pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    return pid==GetCurrentProcessId();
}
static void captureMenuInput(UINT message,WPARAM key,LPARAM details) {
    if(message==WM_KILLFOCUS) { memset(queuedPress,0,sizeof(queuedPress)); frameClickValid=false; return; }
    if(message==WM_LBUTTONDOWN) {
        if(menuOpen) {
            queuedPress[VK_LBUTTON]=true;
            queuedClick={LONG(short(LOWORD(details))),LONG(short(HIWORD(details)))};
        }
    } else if(message==WM_KEYDOWN && key<256 && !(details&(1L<<30))) {
        if(menuOpen || key==VK_F7) queuedPress[key]=true;
    }
}
static LRESULT CALLBACK menuInputWindowProc(HWND window,UINT message,WPARAM key,LPARAM details) {
    captureMenuInput(message,key,details);
    return CallWindowProcW(previousWindowProc,window,message,key,details);
}
static void ensureMenuInputHook() {
    if(inputWindow && IsWindow(inputWindow)) return;
    inputWindow=nullptr;
    HWND window=GetActiveWindow(); DWORD pid=0;
    if(!window || !GetWindowThreadProcessId(window,&pid) || pid!=GetCurrentProcessId()) return;
    auto old=(WNDPROC)SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)menuInputWindowProc);
    if(old) { previousWindowProc=old; inputWindow=window; log("Menu window input buffering active."); }
}
static void pollKeys() {
    bool focus=focused();
    frameClickValid=false;
    for(int i=0;i<256;++i) {
        // Do not consume key transitions used by the original menu unless our
        // settings page owns input. In normal menus we only need the shortcut.
        if(!menuOpen && releaseFrames==0 && i!=VK_F7 && i!=VK_LBUTTON) { down[i]=pressed[i]=false; keys[i]=queuedPress[i]=false; continue; }
        down[i]=focus && (GetAsyncKeyState(i)&0x8000);
        // Preserve a down/up message pair that arrives between render frames.
        pressed[i]=focus && !keys[i] && (queuedPress[i] || down[i]);
        if(i==VK_LBUTTON && pressed[i] && queuedPress[i]) { frameClick=queuedClick; frameClickValid=true; }
        if(pressed[i] && queuedPress[i] && !down[i] && !recoveredInputLogged) {
            recoveredInputLogged=true; log("Menu press recovered from messages after async key-state poll missed it.");
        }
        queuedPress[i]=false; keys[i]=down[i];
    }
}
static bool menuAllowed(void* self) {
    auto bytes=(unsigned char*)self;
    int page=*(int*)(bytes+0x548);
    return bytes[0x111] && ((page==52 && !bytes[0x116]) || page==41 || page==6);
}
static void closeMenu() { menuOpen=false; releaseFrames=3; }
static void change(int direction) {
    if(row==0) settings.enabled=!settings.enabled;
    else if(row==2) settings.radius=clamp(settings.radius+direction*50,50,1000);
    else if(row==1) settings.iconSize=75+((settings.iconSize-75)/25+direction+3)%3*25;
    else if(row==3) settings.missionPercent=25+((settings.missionPercent-25)/5+direction+16)%16*5;
    else {
        auto& s=settings.categories[row-4];
        if(column==0) s.enabled=!s.enabled;
        if(column==1) s.icon=nextIcon(s.icon,direction);
        if(column==2) s.colour=(s.colour+direction+6)%6;
    }
    saveSettings();
}
static bool mouseAt(float x,float y,float w,float h) {
    POINT p;
    if(frameClickValid) p=frameClick;
    else { GetCursorPos(&p); ScreenToClient(GetForegroundWindow(),&p); }
    float mx=p.x*640.0f/windowWidth,my=p.y*448.0f/windowHeight;
    return mx>=x&&mx<x+w&&my>=y&&my<y+h;
}
using MenuFn=void(__thiscall*)(void*);
static MenuFn originalProcess;
static MenuFn originalDraw[2];
static bool nativeOptions;
using TextFn=wchar_t*(__thiscall*)(void*,const char*);
static TextFn originalText;
static wchar_t* __fastcall menuText(void* self,void*,const char* key) {
    static wchar_t name[]=L"SHOP RADAR";
    if(key && !strcmp(key,"SHPRADR")) return name;
    return originalText(self,key);
}
static void __fastcall processMenu(void* self,void*) {
    ensureMenuInputHook();
    if(!menuAllowed(self)) { menuOpen=false; originalProcess(self); return; }
    pollKeys(); updateWindowSize();
    // Wait for the actual close key to be released before resuming the game menu.
    if(!menuOpen && releaseFrames>0) {
        bool held=down[VK_ESCAPE]||down[VK_F7]||down[VK_RETURN]||down[VK_LBUTTON]||down[VK_SPACE]||down[VK_BACK];
        if(!held) --releaseFrames;
        return;
    }
    if(pressed[VK_F7]) {
        if(menuOpen) { closeMenu(); return; }
        menuOpen=true; row=0; column=0;
    }
    if(menuOpen) {
        if(pressed[VK_ESCAPE] || pressed[VK_BACK]) { closeMenu(); return; }
        if(pressed[VK_UP]) row=(row+8)%9;
        if(pressed[VK_DOWN]) row=(row+1)%9;
        if(pressed[VK_TAB]) column=(column+1)%3;
        if(pressed[VK_LEFT]) change(-1);
        if(pressed[VK_RIGHT] || pressed[VK_RETURN] || pressed[VK_SPACE]) change(1);
        if(pressed[VK_LBUTTON]) {
            if(mouseAt(535,34,70,24)) { closeMenu(); return; }
            for(int r=0;r<9;++r) if(mouseAt(32,100+r*27,576,24)) {
                row=r;
                column=mouseAt(458,100+r*27,150,24)?2:mouseAt(358,100+r*27,100,24)?1:0;
                change(1); break;
            }
        }
        return; // Keep normal menu actions from firing behind the settings page.
    }
    if(!nativeOptions && pressed[VK_LBUTTON] && mouseAt(421,12,206,30)) { menuOpen=true; return; }
    originalProcess(self);
    if(nativeOptions && *(int*)((char*)self+0x548)==18) {
        *(int*)((char*)self+0x548)=41;
        *(int*)((char*)self+0x54C)=5;
        menuOpen=true; row=0; column=0;
    }
}
static void renderMenu(void* self,int index) {
    if(!menuOpen) originalDraw[index](self);
    if(!menuAllowed(self)) return;
    updateWindowSize();
    // Fonts are queued by the game; flush before snapshotting/restoring state.
    fn<void(__cdecl*)()>(fontAddress(0x501B50))();
    unsigned char saved[0x44]; if(fontDetails) memcpy(saved,(void*)fontDetails,sizeof(saved));
    if(!menuOpen) {
        if(!nativeOptions) {
            rect(421,12,206,30,{12,20,30,230});
            text(432,17,"SHOP RADAR  [F7]",{255,190,65,255});
        }
    } else {
        rect(0,0,640,448,{7,12,18,255});
        rect(18,22,604,414,{12,20,30,255});
        rect(18,22,604,4,{255,190,65,255});
        text(34,38,"SHOP RADAR",{255,190,65,255},.55f);
        text(545,39,"BACK",{240,240,240,255});
        text(34,77,"NEARBY SERVICES",{155,170,185,255},.28f);
        text(278,77,"SHOW",{155,170,185,255},.28f);
        text(370,77,"ICON",{155,170,185,255},.28f);
        text(485,77,"COLOUR",{155,170,185,255},.28f);
        for(int r=0;r<9;++r) {
            float y=100+r*27;
            if(r==row) rect(30,y,580,24,{35,51,65,255});
            text(38,y+3,r==0?"Mod enabled":r==1?"Icon size":r==2?"Display radius":r==3?"Mission radius":r==8?"Other weapon shops":categoryNames[r-4],{240,240,240,255},.31f);
            char value[64];
            if(r==0) snprintf(value,sizeof(value),"%s",settings.enabled?"ON":"OFF");
            else if(r==2) snprintf(value,sizeof(value),"%d m",settings.radius);
            else if(r==1) snprintf(value,sizeof(value),"%d%%",settings.iconSize);
            else if(r==3) snprintf(value,sizeof(value),"%d%%",settings.missionPercent);
            else snprintf(value,sizeof(value),"%s",settings.categories[r-4].enabled?"ON":"OFF");
            text(279,y+3,value,r==row&&column==0?RGBA{255,190,65,255}:RGBA{230,230,230,255},.30f);
            if(r>=4) {
                auto& s=settings.categories[r-4];
                text(370,y+5,spriteNames[s.icon],r==row&&column==1?RGBA{255,190,65,255}:RGBA{230,230,230,255},.27f);
                text(485,y+5,colourNames[s.colour],r==row&&column==2?RGBA{255,190,65,255}:RGBA{230,230,230,255},.27f);
                // Preview uses the actual game sprite and selected accent frame.
                icon(s.icon,s.colour,{351*windowWidth/640.0f,(y+15)*windowHeight/448.0f},255,true);
            }
        }
        text(432,426,"UP/DOWN: row   TAB: field   LEFT/RIGHT: change",{140,165,185,255},.18f,.42f);
        text(34,370,saveFailed?"Save failed - check INI file permissions":"Changes saved automatically. ESC: back",saveFailed?RGBA{255,140,120,255}:RGBA{170,190,205,255},.27f);
        const bool showBombTypes=row==7 && settings.categories[Bomb].icon==6;
        if(!showBombTypes) {
            const char* help=row==0?"Show nearby service icons. OFF restores the original radar.":
                row==1?"Icon size: 75%, 100% or 125%. Mission markers keep their size.":
                row==2?"Display radius: maximum shop distance during free roam.":
                row==3?"Mission radius: percentage of Display radius used in missions.":
                column==0?"Show or hide this service category. Mission markers stay visible.":
                column==1?"Choose this category's symbol. Bomb type follows each shop.":
                "Colour changes the frame; the original symbol keeps its colours.";
            text(34,386,help,{175,195,210,255},.23f);
            char detail[140];
            if(row==2) snprintf(detail,sizeof(detail),"Current range: %d units. Icons fade over the outer 15%%.",settings.radius);
            else if(row==3) snprintf(detail,sizeof(detail),"%d%% of %d = %.0f units. Available shops only; mission markers take priority.",settings.missionPercent,settings.radius,missionDistance(settings));
            else snprintf(detail,sizeof(detail),"Changes save automatically. Shop icons stay close to their true position.");
            text(34,404,detail,{140,165,185,255},.22f);
        }
        if(showBombTypes) {
            const float x[]={45,225,415};
            const char* names[]={"Portland: ignition","Staunton: timer","Shoreside: remote"};
            for(int i=0;i<3;++i) {
                icon(locationIcon(6,6+i),settings.categories[Bomb].colour,
                     {x[i]*windowWidth/640.0f,410*windowHeight/448.0f},255,true);
                text(x[i]+16,404,names[i],{220,230,240,255},.22f);
            }
        }
    }
    fn<void(__cdecl*)()>(fontAddress(0x501B50))();
    if(fontDetails) memcpy((void*)fontDetails,saved,sizeof(saved));
}
static void __fastcall drawMenu0(void* self,void*) { renderMenu(self,0); }
static void __fastcall drawMenu1(void* self,void*) { renderMenu(self,1); }
struct Hook { uintptr_t site; void* replacement; void* original; };
static Hook hooks[96]; static int hookCount;
struct MenuEntry { int action; char key[8]; int saveSlot,target; };
static_assert(sizeof(MenuEntry)==20);
static MenuEntry* optionsEntries;
static bool findOptionsEntries(unsigned char* base,IMAGE_NT_HEADERS* nt) {
    auto section=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        if(section[i].Characteristics&IMAGE_SCN_MEM_EXECUTE) continue;
        auto start=base+section[i].VirtualAddress; size_t size=section[i].Misc.VirtualSize;
        for(size_t j=0;j+28+8*sizeof(MenuEntry)<=size;++j) {
            // Existing mods can split a section into several protection regions.
            // Validate each page instead of rejecting the entire data section.
            if((j&4095)==0 && !accessible((uintptr_t)start+j,(size-j<4096)?size-j:4096)) {
                j+=4095; continue;
            }
            if(start[j]!='F' || !accessible((uintptr_t)start+j,28+8*sizeof(MenuEntry))) continue;
            if(memcmp(start+j,"FET_OPT",8)) continue;
            auto entries=(MenuEntry*)(start+j+28);
            MenuEntry empty{};
            if(memcmp(entries[0].key,"FET_CTL",8) || memcmp(entries[1].key,"FET_AUD",8) ||
               memcmp(entries[2].key,"FET_DIS",8) || memcmp(entries[3].key,"FET_LAN",8) ||
               memcmp(entries[4].key,"FET_PSU",8) || memcmp(entries[5].key,"FEDS_TB",8) ||
               entries[5].action!=2 || memcmp(entries+6,&empty,sizeof(empty)) ||
               memcmp(entries+7,&empty,sizeof(empty))) continue;
            if(optionsEntries) { optionsEntries=nullptr; return false; }
            optionsEntries=entries;
        }
    }
    return optionsEntries!=nullptr;
}
static void* callTarget(uintptr_t site) {
    if(!accessible(site,5,true)||*(unsigned char*)site!=0xE8) return nullptr;
    int32_t displacement; memcpy(&displacement,(void*)(site+1),sizeof(displacement));
    return (void*)(site+5+displacement);
}
static bool addHook(uintptr_t site,void* replacement) {
    void* original=callTarget(site);
    if(!original || !accessible((uintptr_t)original,1,true) || hookCount>=96) {
        char message[128]; snprintf(message,sizeof(message),"Hook validation failed at %08lx",(unsigned long)site); log(message); return false;
    }
    hooks[hookCount++]={site,replacement,original}; return true;
}
static bool writeCall(const Hook& hook,void* destination) {
    DWORD old;
    if(!VirtualProtect((void*)hook.site,5,PAGE_EXECUTE_READWRITE,&old)) return false;
    int32_t displacement=int32_t((uintptr_t)destination-hook.site-5);
    memcpy((void*)(hook.site+1),&displacement,sizeof(displacement));
    DWORD unused; VirtualProtect((void*)hook.site,5,old,&unused);
    FlushInstructionCache(GetCurrentProcess(),(void*)hook.site,5);
    return true;
}
static bool initialise() {
    if((uintptr_t)GetModuleHandleW(nullptr)!=0x400000) { log("Unsupported image base; no hooks installed."); return false; }
    int checks[]={0x5C1E70,0x5C2130,0x5C6FD0};
    for(int i=0;i<3;++i) if(accessible(checks[i],4,true)&&*(uint32_t*)checks[i]==0x53E58955) { version=i; break; }
    if(version<0) { log("Unsupported GTA III executable; no hooks installed."); return false; }
    char message[100]; snprintf(message,sizeof(message),"GTA III version: %s",version==0?"1.0":version==1?"1.1":"Steam"); log(message);
    // Resolve the garage array from DeActivateGarage's validated indexed byte store.
    auto deactivate=(unsigned char*)0x426C40;
    if(accessible((uintptr_t)deactivate,32,true) && !memcmp(deactivate,"\x0f\xbf\x4c\x24\x04",5) &&
       !memcmp(deactivate+18,"\xc6\x04\x85",3)) {
        uintptr_t pool=*(uint32_t*)(deactivate+21)-4;
        if(accessible(pool,32*sizeof(GarageRecord))) garagePool=pool;
    }
    if(!garagePool) log("Garage ABI unavailable; mission service availability will fail closed.");
    if(version==2 && accessible(0x4A1140,40,true) && accessible(0x426700,36,true) &&
       !memcmp((void*)0x4A1140,"\x0f\xb6\x05",3) &&
       !memcmp((void*)0x426700,"\x8b\x4c\x24\x04\x0f\xbf\x41\x5c",8)) vehicleAbi=true;
    auto blips=(unsigned char*)v(0x4A42F0,0x4A43E0,0x4A4370);
    if(accessible((uintptr_t)blips,32,true) && !memcmp(blips+7,"\x80\x3d",2) &&
       !memcmp(blips+16,"\x80\x3d",2)) {
        auto suppressed=*(uint32_t*)(blips+9), enabled=*(uint32_t*)(blips+18);
        if(accessible(suppressed,1) && accessible(enabled,1)) { radarSuppressed=suppressed; radarEnabled=enabled; }
    }
    if(!radarSuppressed || !radarEnabled) { log("HUD visibility ABI mismatch; no hooks installed."); return false; }
    // IsPlayerOnAMission has the same entry point in these classic versions.
    // Check its structure and globals before calling it to preserve mission blips.
    auto missionCode=(unsigned char*)0x439410;
    if(!accessible(0x439410,34,true) || missionCode[0]!=0x83 || missionCode[1]!=0x3D ||
       missionCode[6]!=0 || missionCode[7]!=0x74 || missionCode[9]!=0xB8 ||
       missionCode[14]!=0x03 || missionCode[15]!=0x05 ||
       *(uint32_t*)(missionCode+2)!=*(uint32_t*)(missionCode+16) ||
       !accessible(*(uint32_t*)(missionCode+2),4) || !accessible(*(uint32_t*)(missionCode+10),163840)) {
        log("Mission status ABI mismatch; no hooks installed."); return false;
    }
    // Only the regular HUD radar call: the cutscene/replay call is deliberately left alone.
    if(!addHook(v(0x508499,0x508579,0x508509),(void*)drawRadar)) return false;
    originalRadar=(RadarFn)hooks[0].original;
    const uintptr_t spriteSites[]={
        uintptr_t(v(0x4A4460,0x4A4550,0x4A44E0)), uintptr_t(v(0x4A47E4,0x4A48D4,0x4A4864)),
        uintptr_t(v(0x4A4A2D,0x4A4B1D,0x4A4AAD)), uintptr_t(v(0x4A4BEF,0x4A4CDF,0x4A4C6F))
    };
    void* spriteHooks[]={(void*)nativeSprite0,(void*)nativeSprite1,(void*)nativeSprite2,(void*)nativeSprite3};
    for(int i=0;i<4;++i) {
        if(!addHook(spriteSites[i],spriteHooks[i])) return false;
        originalSprites[i]=(SpriteFn)hooks[hookCount-1].original;
    }
    // Locate direct calls to the version-specific Process function within executable sections.
    uintptr_t process=v(0x485100,0x4851D0,0x4851D0);
    auto base=(unsigned char*)GetModuleHandleW(nullptr);
    auto dos=(IMAGE_DOS_HEADER*)base;
    auto nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);
    auto section=IMAGE_FIRST_SECTION(nt);
    int processCalls=0,drawCalls=0;
    uintptr_t draw=v(0x47A540,0x47A530,0x47A530);
    bool optionsFound=findOptionsEntries(base,nt);
    log(optionsFound?"Options table validated.":"Options table unavailable; using F7 launcher.");
    uintptr_t textGet=v(0x52C5A0,0x52C7E0,0x52C770);
    originalText=(TextFn)textGet;
    int textCalls=0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) if(section[i].Characteristics&IMAGE_SCN_MEM_EXECUTE) {
        uintptr_t start=(uintptr_t)base+section[i].VirtualAddress;
        size_t size=section[i].Misc.VirtualSize;
        for(size_t j=0;j+5<=size;++j) {
            // Validate each chunk before even reading its opcode; callTarget
            // separately guards CALLs spanning a protection boundary.
            if((j&4095)==0 && !accessible(start+j,(size-j<4096)?size-j:4096,true)) {
                j+=4095; continue;
            }
            if(*(unsigned char*)(start+j)!=0xE8) continue;
            auto target=callTarget(start+j);
            if(!target) continue;
            if(optionsFound && target==(void*)textGet && start+j>=0x47A000 && start+j<0x47E000) {
                if(!addHook(start+j,(void*)menuText)) return false;
                ++textCalls;
            }
            if(target==(void*)process) {
                if(!addHook(start+j,(void*)processMenu)) return false;
                ++processCalls;
            }
            if(target==(void*)draw) {
                if(drawCalls>=2 || !addHook(start+j,drawCalls==0?(void*)drawMenu0:(void*)drawMenu1)) return false;
                originalDraw[drawCalls++]=(MenuFn)target;
            }
        }
    }
    if(processCalls<1 || processCalls>4) { log("Unexpected menu Process callers; no hooks installed."); return false; }
    if(drawCalls<1) { log("Could not find frontend Draw callers; no hooks installed."); return false; }
    originalProcess=(MenuFn)process;
    // Find the font state through SetScale's absolute stores (not a guessed global offset).
    auto scale=(unsigned char*)fontAddress(0x501B80);
    if(!accessible((uintptr_t)scale,37,true)) { log("Font code unavailable; no hooks installed."); return false; }
    for(int i=0;i<32;++i) if(scale[i]==0xD9 && scale[i+1]==0x1D) {
        uintptr_t candidate=*(uint32_t*)(scale+i+2)-4;
        if(accessible(candidate,0x44)) { fontDetails=candidate; break; }
    }
    if(!fontDetails) { log("Font ABI mismatch; no hooks installed."); return false; }
    loadSettings();
    for(int i=0;i<hookCount;++i) if(!writeCall(hooks[i],hooks[i].replacement)) {
        for(int j=i-1;j>=0;--j) writeCall(hooks[j],hooks[j].original);
        log("Could not install hooks; rolled back."); return false;
    }
    log("ShopRadar active. Pause game, then choose SHOP RADAR or press F7. Radius default: 250m.");
    if(optionsFound && textCalls>0) {
        DWORD old;
        if(VirtualProtect(optionsEntries+5,3*sizeof(MenuEntry),PAGE_READWRITE,&old)) {
            optionsEntries[6]=optionsEntries[5];
            optionsEntries[5]={2,"SHPRADR",0,18};
            optionsEntries[7]={};
            DWORD unused; VirtualProtect(optionsEntries+5,3*sizeof(MenuEntry),old,&unused);
            nativeOptions=true; log("Native Options row installed.");
        }
    }
    return true;
}
// Ultimate ASI Loader calls this export after DLL loading, outside the loader lock.
extern "C" __declspec(dllexport) void InitializeASI() {
    static bool once=false; if(once) return; once=true;
    DWORD length=GetModuleFileNameW(module,iniPath,MAX_PATH);
    if(!length || length>=MAX_PATH) return; // No usable path: stay inert.
    wchar_t* dot=wcsrchr(iniPath,L'.'); if(!dot) return;
    wcscpy(dot,L".ini");
    wcscpy(logPath,iniPath); wcscpy(wcsrchr(logPath,L'.'),L".log");
    log("GTA3.ShopRadar " SHOPRADAR_VERSION " beta starting.");
    if(!initialise()) log("Initialisation stopped; see diagnostics above. Game files were not modified.");
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) { module=instance; DisableThreadLibraryCalls(instance); }
    return TRUE;
}
