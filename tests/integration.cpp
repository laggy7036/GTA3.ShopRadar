// Runs actual Windows INI and settings code without installing game hooks.
#include "../src/ShopRadar.cpp"
#include <assert.h>
#include <initializer_list>
#include <vector>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main() {
    Vec2 occupied[]={{0,0},{-80,0}};
    auto placed=radarSlot({0,0},occupied,1,20,{0,0},{100,80});
    CHECK(distanceSquared(placed,occupied[0])<=2.4f*2.4f+.001f);
    CHECK(placed.y<=0);
    CHECK(radarSlot({30,30},occupied,1,20,{0,0},{100,80}).x==30);
    occupied[0]={100,0};
    placed=radarSlot({100,0},occupied,1,20,{0,0},{100,80});
    CHECK(distanceSquared(placed,occupied[0])<=2.4f*2.4f+.001f);
    CHECK(placed.x*placed.x/10000+placed.y*placed.y/6400<=1.001f);

    // Dense overlaps and moving anchors must never cause full-icon jumps.
    for(float spacing:{16.0f,24.0f,40.0f,100.0f}) for(int x=-90;x<=90;x+=10) for(int y=-70;y<=70;y+=10) {
        Vec2 anchor{float(x),float(y)}; Vec2 crowded[3]={anchor,{anchor.x+1,anchor.y},{anchor.x,anchor.y+2}};
        auto result=radarSlot(anchor,crowded,3,spacing,{0,0},{100,80});
        CHECK(distanceSquared(result,anchor)<=spacing*spacing*.0144f+.01f);
        auto repeat=radarSlot(anchor,crowded,3,spacing,{0,0},{100,80});
        CHECK(result.x==repeat.x && result.y==repeat.y);
    }
    // Decode real user-owned HUD textures read-only; no game process/hooks needed.
    FILE* hud=fopen("../models/hud.txd","rb");
    if(hud) {
        fseek(hud,0,SEEK_END); long n=ftell(hud); rewind(hud);
        std::vector<unsigned char> bytes(n); CHECK(fread(bytes.data(),1,n,hud)==size_t(n)); fclose(hud);
        for(auto name:{"radar_weapon","radar_spray","radar_bomb","radar_eight","radar_save","radar_cat"}) {
            PreviewImage image; CHECK(findHudPreview(bytes.data(),bytes.size(),name,image));
            CHECK(image.width==16 && image.height==16);
            int runs=0;
            drawHudPreview(image,Rect{0,32,32,0},128,[&](const Rect& r,RGBA c) {
                assert(r.left>=0 && r.right<=32 && r.top>=0 && r.bottom<=32 && c.a<=128); ++runs;
            }); CHECK(runs>0);
            CHECK(!findHudPreview(bytes.data(),11,name,image));
        }
    }
    // Synthetic malformed/unsupported native texture and index/alpha regression.
    std::vector<unsigned char> native(1118); auto put=[&](size_t at,uint32_t n) { memcpy(native.data()+at,&n,4); };
    put(0,8); memcpy(native.data()+8,"test",5); put(72,0x2500);
    native[80]=2; native[82]=1; native[84]=8; native[85]=1;
    put(1112,2); native[1116]=1; native[1117]=2;
    native[92]=255; native[95]=255; native[97]=255; native[99]=128;
    PreviewImage decoded;
    CHECK(decodeHudPreview(native.data(),native.size(),"test",decoded));
    CHECK(decoded.pixels[0].r==255 && decoded.pixels[1].g==255);
    CHECK(!decodeHudPreview(native.data(),1117,"test",decoded));
    native[87]=1; CHECK(!decodeHudPreview(native.data(),native.size(),"test",decoded)); native[87]=0;
    native[80]=65; CHECK(!decodeHudPreview(native.data(),native.size(),"test",decoded)); native[80]=2;
    memset(native.data()+8,'X',32); CHECK(!decodeHudPreview(native.data(),native.size(),"test",decoded));

    CHECK(nextIcon(6,1)==0 && nextIcon(0,-1)==6 && nextIcon(10,-1)==6);
    for(int old:{7,8,9,10,11,12}) CHECK(activeIcon(old)==6 && !selectableIcon(old));

    CHECK(locationIcon(6,6)==10 && locationIcon(6,7)==11 && locationIcon(6,8)==12);
    for(int choice=0;choice<iconCount;++choice) if(choice!=6) CHECK(locationIcon(choice,7)==choice);
    CHECK(nativeIconSprite(6)==2);
    for(int type=0;type<3;++type) {
        CHECK(nativeIconSprite(10+type)==2 && nativeIconSprite(7+type)==-1);
        for(int y=0;y<8;++y) CHECK(strlen(bombBadgePixels[type][y])==8);
        for(int height:{480,720,1080,1440}) for(int size:{75,100,125}) {
            auto style=iconStyle({100,100},height,size,3,123);
            auto badge=bombBadgeBounds(style.texture);
            CHECK(badge.left>style.texture.left && badge.top>style.texture.top);
            CHECK(badge.right==style.texture.right && badge.bottom==style.texture.bottom);
            int count=0;
            drawBombBadge(type,style.texture,RGBA{255,255,255,123},[&](const Rect& r,RGBA c) {
                assert(r.left>=badge.left && r.right<=badge.right);
                assert(r.top>=badge.top && r.bottom<=badge.bottom && c.a==123);
                ++count;
            });
            CHECK(count>1);
        }
    }
    for(int type=0;type<3;++type) {
        int lit=0;
        for(int y=0;y<16;++y) { CHECK(strlen(bombPixels[type][y])==16); for(int x=0;x<16;++x) lit+=bombPixels[type][y][x]=='#'; }
        CHECK(lit>40);
        for(int height:{480,720,1080,1440}) for(int size:{75,100,125}) {
            auto style=iconStyle({100,100},height,size,5,123);
            float area=0;
            drawBombPixels(type,style.texture,RGBA{255,255,255,123},[&](const Rect& r,RGBA c) {
                assert(r.left>=style.texture.left && r.right<=style.texture.right);
                assert(r.top>=style.texture.top && r.bottom<=style.texture.bottom);
                assert(c.a==123 && c.r==255 && c.g==255 && c.b==255);
                area+=(r.right-r.left)*(r.bottom-r.top);
            });
            CHECK(fabsf(area-lit*(style.texture.right-style.texture.left)*(style.texture.bottom-style.texture.top)/256)<.1f);
        }
    }

    // Reproduce split protection regions created by other ASI mods. The native
    // Options scanner must find one intact table and reject ambiguous layouts.
    struct TestImage { IMAGE_NT_HEADERS nt; IMAGE_SECTION_HEADER section; } pe{};
    pe.nt.FileHeader.NumberOfSections=1;
    pe.nt.FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER);
    pe.section.VirtualAddress=4096; pe.section.Misc.VirtualSize=8192;
    auto image=(unsigned char*)VirtualAlloc(nullptr,12288,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(image);
    auto header=image+4096+512;
    memcpy(header,"FET_OPT",8);
    MenuEntry sample[8]={{2,"FET_CTL",0,35},{101,"FET_AUD",0,5},
        {2,"FET_DIS",0,6},{2,"FET_LAN",0,7},{97,"FET_PSU",0,54},{2,"FEDS_TB",0,0},{},{}};
    memcpy(header+28,sample,sizeof(sample));
    DWORD old;
    CHECK(VirtualProtect(image+4096,4096,PAGE_READONLY,&old));
    optionsEntries=nullptr;
    CHECK(findOptionsEntries(image,&pe.nt) && optionsEntries==(MenuEntry*)(header+28));
    CHECK(!wcscmp(menuText(nullptr,nullptr,"SHPRADR"),L"SHOP RADAR"));
    auto duplicate=image+8192+512;
    memcpy(duplicate,header,28+sizeof(sample));
    optionsEntries=nullptr; CHECK(!findOptionsEntries(image,&pe.nt) && !optionsEntries);
    duplicate[0]=0;
    CHECK(VirtualProtect(image+4096,4096,PAGE_NOACCESS,&old));
    CHECK(callTarget((uintptr_t)image+4096)==nullptr);
    CHECK(callTarget(UINTPTR_MAX-2)==nullptr);
    CHECK(VirtualProtect(image+8192,4096,PAGE_EXECUTE_READWRITE,&old));
    auto call=image+8192;
    call[0]=0x90; CHECK(callTarget((uintptr_t)call)==nullptr);
    call[0]=0xE8; int32_t displacement=16; memcpy(call+1,&displacement,sizeof(displacement));
    CHECK(callTarget((uintptr_t)call)==call+21);
    // A CALL straddling an inaccessible page is rejected before reading displacement.
    CHECK(callTarget((uintptr_t)image+8190)==nullptr);

    optionsEntries=nullptr; CHECK(!findOptionsEntries(image,&pe.nt));
    CHECK(VirtualProtect(image+4096,4096,PAGE_READWRITE,&old));
    ((MenuEntry*)(header+28))[5].action=0;
    optionsEntries=nullptr; CHECK(!findOptionsEntries(image,&pe.nt));
    memcpy(header+28,sample,sizeof(sample));
    memset(((MenuEntry*)(header+28))[0].key,'X',8);
    optionsEntries=nullptr; CHECK(!findOptionsEntries(image,&pe.nt));
    memcpy(header+28,sample,sizeof(sample));
    ((MenuEntry*)(header+28))[6].key[0]='X';
    optionsEntries=nullptr; CHECK(!findOptionsEntries(image,&pe.nt));
    CHECK(!accessible(UINTPTR_MAX-4,8));
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    optionsEntries=nullptr;
    GetFullPathNameA("build/test-settings.ini",MAX_PATH,iniPath,nullptr);
    GetFullPathNameA("build/test-settings.log",MAX_PATH,logPath,nullptr);
    DeleteFileA(iniPath); // Only this test-owned file in build.
    settings=Settings{}; loadSettings();
    CHECK(settings.enabled && settings.dynamicDiscovery && settings.radius==250);
    CHECK(settings.missionPercent==70);
    CHECK(settings.iconSize==75);
    CHECK(settings.categories[Spray].icon==1 && settings.categories[Bomb].icon==6);
    settings.categories[Spray].icon=5;
    CHECK(WritePrivateProfileStringA(categoryNames[Spray],"Icon",nullptr,iniPath));
    loadSettings(); CHECK(settings.categories[Spray].icon==1);
    const char* inputs[]={"", "garbage", "12oops", "-1", "0", "49", "50", "1000", "1001", "2147483647", "99999999999999999999999", "-99999999999999999999999"};
    const int expected[]={250,250,250,50,50,50,50,1000,1000,1000,1000,50};
    for(unsigned i=0;i<sizeof(inputs)/sizeof(*inputs);++i) {
        CHECK(WritePrivateProfileStringA("General","Radius",inputs[i],iniPath));
        loadSettings(); CHECK(settings.radius==expected[i]);
    }
    for(int category=0;category<CategoryCount;++category)
        for(int enabled=0;enabled<2;++enabled)
            for(int iconValue=0;iconValue<iconCount;++iconValue)
                if(selectableIcon(iconValue))
                for(int colourValue=0;colourValue<6;++colourValue) {
                    settings=Settings{};
                    settings.enabled=enabled!=0;
                    settings.dynamicDiscovery=(checks%2)!=0; settings.radius=50+(checks%20)*50;
                    settings.missionPercent=25+(checks%16)*5;
                    settings.iconSize=75+(checks%3)*25;
                    settings.categories[category]={enabled!=0,iconValue,colourValue};
                    const Settings expectedSettings=settings;
                    saveSettings(); CHECK(!saveFailed);
                    settings=Settings{}; loadSettings();
                    CHECK(settings.enabled==expectedSettings.enabled && settings.radius==expectedSettings.radius);
                    CHECK(settings.missionPercent==expectedSettings.missionPercent);
                    CHECK(settings.dynamicDiscovery==expectedSettings.dynamicDiscovery);
                    CHECK(settings.iconSize==expectedSettings.iconSize);
                    for(int c=0;c<CategoryCount;++c) {
                        CHECK(settings.categories[c].enabled==expectedSettings.categories[c].enabled);
                        CHECK(settings.categories[c].icon==expectedSettings.categories[c].icon);
                        CHECK(settings.categories[c].colour==expectedSettings.categories[c].colour);
                    }
                }
    // Exercise actual UI mutation with all icon/colour wrap points and categories.
    settings=Settings{};
    for(row=4;row<9;++row) {
        settings.categories[row-4].icon=0;
        column=1; change(-1); CHECK(settings.categories[row-4].icon==6);
        change(1); CHECK(settings.categories[row-4].icon==0);
        column=2; change(-1); CHECK(settings.categories[row-4].colour==5);
        change(1); CHECK(settings.categories[row-4].colour==0);
        column=0; change(1); CHECK(!settings.categories[row-4].enabled);
        change(-1); CHECK(settings.categories[row-4].enabled);
    }
    row=3; settings.missionPercent=25; change(-1); CHECK(settings.missionPercent==100);
    change(1); CHECK(settings.missionPercent==25);
    for(int percent=25;percent<=100;percent+=5) {
        settings.missionPercent=percent; saveSettings(); settings.missionPercent=0; loadSettings(); CHECK(settings.missionPercent==percent);
    }
    row=1; settings.iconSize=75; change(-1); CHECK(settings.iconSize==125);
    change(1); CHECK(settings.iconSize==75);
    for(int size:{75,100,125}) { settings.iconSize=size; saveSettings(); settings.iconSize=0; loadSettings(); CHECK(settings.iconSize==size); }
    CHECK(WritePrivateProfileStringA("General","IconSize","99999",iniPath));
    loadSettings(); CHECK(settings.iconSize==125);
    CHECK(WritePrivateProfileStringA("General","IconSize","-1",iniPath));
    loadSettings(); CHECK(settings.iconSize==75);
    CHECK(WritePrivateProfileStringA("General","IconSize","garbage",iniPath));
    loadSettings(); CHECK(settings.iconSize==75);
    // A short down/up pair must remain buffered until the next menu frame.
    menuOpen=true; memset(queuedPress,0,sizeof(queuedPress));
    captureMenuInput(WM_KEYDOWN,VK_DOWN,0); captureMenuInput(WM_KEYUP,VK_DOWN,0);
    CHECK(queuedPress[VK_DOWN]);
    queuedPress[VK_DOWN]=false;
    captureMenuInput(WM_KEYDOWN,VK_DOWN,1L<<30); CHECK(!queuedPress[VK_DOWN]);
    captureMenuInput(WM_LBUTTONDOWN,0,MAKELPARAM(125,-3)); CHECK(queuedPress[VK_LBUTTON]);
    CHECK(queuedClick.x==125 && queuedClick.y==-3);
    frameClick=queuedClick; frameClickValid=true;
    CHECK(mouseAt(120,-10,10,10)); CHECK(!mouseAt(200,100,10,10));
    captureMenuInput(WM_KILLFOCUS,0,0); CHECK(!queuedPress[VK_DOWN] && !queuedPress[VK_LBUTTON] && !frameClickValid);
    menuOpen=false;
    captureMenuInput(WM_KEYDOWN,VK_DOWN,0); CHECK(!queuedPress[VK_DOWN]);
    captureMenuInput(WM_KEYDOWN,VK_F7,0); CHECK(queuedPress[VK_F7]);
    captureMenuInput(WM_KILLFOCUS,0,0);
    row=2; settings.radius=50; change(-1); CHECK(settings.radius==50);
    settings.radius=1000; change(1); CHECK(settings.radius==1000);
    // Failure must be reported; current in-memory choices must survive.
    char realPath[MAX_PATH]; strcpy(realPath,iniPath);
    strcpy(iniPath,"build/nonexistent-test-directory/settings.ini");
    settings.radius=350; saveSettings(); CHECK(saveFailed && settings.radius==350);
    strcpy(iniPath,realPath); saveSettings(); CHECK(!saveFailed);
    CHECK(SetFileAttributesA(iniPath,FILE_ATTRIBUTE_READONLY));
    saveSettings(); CHECK(saveFailed);
    CHECK(SetFileAttributesA(iniPath,FILE_ATTRIBUTE_NORMAL));
    saveSettings(); CHECK(!saveFailed);
    printf("PASS: %u Windows INI and UI mutation checks; no game hooks installed\n",checks);
}
