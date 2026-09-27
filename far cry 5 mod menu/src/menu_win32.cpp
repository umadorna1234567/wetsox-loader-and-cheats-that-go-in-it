#include <windows.h>
#include <commdlg.h>
#include "fc5/menu.hpp"
#include "fc5/targeting.hpp"
#include <cmath>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
fc5::Settings settings;
HFONT font{};
HWND window{},status{};
HINSTANCE menuModule{};
std::filesystem::path configPath;
struct Binding {
    HWND control;
    std::wstring key;
    std::function<std::wstring()> read;
    std::function<bool(const std::wstring&)> write;
};
std::vector<Binding> bindings;
std::vector<HWND> numericControls;
HWND travel{},prediction{},drop{};
constexpr int Save=10,Reset=11,ColorBase=20,Travel=30;
HWND make(const wchar_t* type,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0) {
    HWND control=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),menuModule,nullptr);
    if(!control) throw std::runtime_error("Control creation failed");
    SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    return control;
}
void label(const wchar_t* text,int x,int y,int w=370) {make(L"STATIC",text,0,x,y,w,24);}
HWND toggle(const wchar_t* title,const wchar_t* key,bool& value,int x,int y,int id=0) {
    HWND h=make(L"BUTTON",title,BS_AUTOCHECKBOX|WS_TABSTOP,x,y,350,26,id);
    auto ptr=&value;
    auto write=[h,ptr](const std::wstring& s) {
        if(s!=L"0"&&s!=L"1") return false;
        *ptr=s==L"1";SendMessageW(h,BM_SETCHECK,*ptr?BST_CHECKED:BST_UNCHECKED,0);return true;
    };
    write(value?L"1":L"0");
    bindings.push_back({h,key,[h,ptr] {*ptr=SendMessageW(h,BM_GETCHECK,0,0)==BST_CHECKED;return *ptr?L"1":L"0";},write});
    return h;
}
bool parseNumber(const std::wstring& s,double lo,double hi,double& output) {
    try {std::size_t used{};double v=std::stod(s,&used);
        if(used!=s.size()||!std::isfinite(v)||v<lo||v>hi) return false;
        output=v;return true;
    } catch(...) {return false;}
}
void number(const wchar_t* title,const wchar_t* key,double& value,double lo,double hi,int x,int y) {
    label(title,x,y,255);
    HWND h=make(L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,x+260,y-2,85,26);
    numericControls.push_back(h);
    auto ptr=&value;
    auto write=[h,ptr,lo,hi](const std::wstring& s) {
        double v;if(!parseNumber(s,lo,hi,v)) return false;
        *ptr=v;SetWindowTextW(h,s.c_str());return true;
    };
    write(std::to_wstring(value));
    bindings.push_back({h,key,[h] {wchar_t buf[128]{};GetWindowTextW(h,buf,128);return std::wstring(buf);},write});
}
void dependencies() {
    BOOL enabled=SendMessageW(travel,BM_GETCHECK,0,0)==BST_CHECKED;
    EnableWindow(prediction,enabled);EnableWindow(drop,enabled);
}
void colorsText() {
    for(int i=0;i<3;++i) {
        auto c=settings.colors[i];wchar_t buf[80];
        const wchar_t* names[]{L"Enemies",L"Other humans",L"Animals"};
        swprintf_s(buf,L"%s color: #%02X%02X%02X",names[i],c.r,c.g,c.b);
        SetDlgItemTextW(window,ColorBase+i,buf);
    }
}
void load() {
    for(auto& binding:bindings) {
        wchar_t buf[256]{};
        GetPrivateProfileStringW(L"Menu",binding.key.c_str(),L"",buf,256,configPath.c_str());
        if(buf[0]) binding.write(buf); // Invalid persisted values retain defaults.
    }
    dependencies();colorsText();
}
void save() {
    std::vector<std::wstring> values;
    for(auto& binding:bindings) {
        auto text=binding.read();
        if(!binding.write(text)) {
            std::wstring message=L"Invalid value: "+binding.key+L". Use the displayed range.";
            MessageBoxW(window,message.c_str(),L"Settings",MB_OK|MB_ICONWARNING);
            SetFocus(binding.control);return;
        }
        values.push_back(text);
    }
    bool ok=true;
    for(std::size_t i=0;i<bindings.size();++i)
        ok=WritePrivateProfileStringW(L"Menu",bindings[i].key.c_str(),values[i].c_str(),configPath.c_str())&&ok;
    SetWindowTextW(status,ok?L"Settings saved. Game integration is still unavailable.":L"Could not save settings beside the executable.");
}
void build() {
    label(L"FAR CRY 5  /  MENU CONFIGURATION",24,18,780);
    label(L"Preview only - not connected to the game. These controls do not change gameplay.",24,48,800);
    label(L"AIMBOT",24,92);
    toggle(L"Enable aimbot",L"aim.enabled",settings.aim.enabled,24,122);
    toggle(L"Enemy humans",L"aim.enemies",settings.aim.targets.enemies,24,152);
    toggle(L"Other humans",L"aim.otherHumans",settings.aim.targets.otherHumans,24,182);
    toggle(L"Animals",L"aim.animals",settings.aim.targets.animals,24,212);
    toggle(L"Use with vehicle-mounted weapons",L"aim.vehicleWeapons",settings.aim.vehicleWeapons,24,242);
    label(L"Hit location",24,278,160);
    HWND hit=make(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,200,274,170,150);
    for(auto name:{L"Head",L"Chest",L"Abdomen",L"Pelvis"}) SendMessageW(hit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
    SendMessageW(hit,CB_SETCURSEL,0,0);
    bindings.push_back({hit,L"aim.hitLocation",[hit] {
        return std::to_wstring(SendMessageW(hit,CB_GETCURSEL,0,0));
    },[hit](const std::wstring& s) {
        if(s.size()!=1||s[0]<L'0'||s[0]>L'3') return false;
        settings.aim.hitLocation=static_cast<fc5::HitLocation>(s[0]-L'0');
        SendMessageW(hit,CB_SETCURSEL,s[0]-L'0',0);return true;
    }});
    number(L"Aim FOV diameter (0.1-180 deg)",L"aim.fov",settings.aim.fovDegrees,.1,180,24,316);
    number(L"Smoothing (0-2 seconds)",L"aim.smoothing",settings.aim.smoothingSeconds,0,2,24,352);
    travel=toggle(L"Compensate projectile travel time",L"aim.travelTime",settings.aim.travelTime,24,388,Travel);
    prediction=toggle(L"Predict target motion",L"aim.prediction",settings.aim.prediction,24,418);
    drop=toggle(L"Compensate bullet drop",L"aim.bulletDrop",settings.aim.bulletDrop,24,448);
    label(L"Prediction and drop require travel time.",24,486);
    label(L"Spread stays unchanged. Zero smoothing = instant.",24,514,390);
    label(L"Missing hit locations will not use a substitute.",24,542,390);
    label(L"ESP",430,92);
    toggle(L"Enable ESP",L"esp.enabled",settings.esp,430,122);
    toggle(L"Enemy humans",L"esp.enemies",settings.espTargets.enemies,430,152);
    toggle(L"Other humans (both = everyone)",L"esp.otherHumans",settings.espTargets.otherHumans,430,182);
    toggle(L"Animals",L"esp.animals",settings.espTargets.animals,430,212);
    for(int i=0;i<3;++i) {
        HWND h=make(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,430,246+i*32,345,27,ColorBase+i);
        bindings.push_back({h,L"esp.color"+std::to_wstring(i),[i] {
            auto c=settings.colors[i];return std::to_wstring(RGB(c.r,c.g,c.b));
        },[i](const std::wstring& s) {
            double value;if(!parseNumber(s,0,16777215,value)||std::floor(value)!=value) return false;
            auto c=static_cast<COLORREF>(value);settings.colors[i]={GetRValue(c),GetGValue(c),GetBValue(c)};return true;
        }});
    }
    label(L"WEAPONS & VEHICLES",430,354);
    toggle(L"Unlimited reserve ammunition",L"weapon.unlimitedAmmo",settings.unlimitedAmmo,430,384);
    toggle(L"No reload",L"weapon.noReload",settings.noReload,430,414);
    toggle(L"Override vehicle camera FOV",L"vehicle.fovEnabled",settings.vehicleFovOverride,430,450);
    number(L"Vehicle camera FOV (40-140 deg)",L"vehicle.fov",settings.vehicleFovDegrees,40,140,430,486);
    toggle(L"Override vehicle speed",L"vehicle.speedEnabled",settings.vehicleSpeedOverride,430,518);
    number(L"Vehicle speed multiplier (0.1-5)",L"vehicle.speed",settings.vehicleSpeedMultiplier,.1,5,430,554);
    make(L"BUTTON",L"Save configuration",BS_PUSHBUTTON|WS_TABSTOP,24,610,200,34,Save);
    make(L"BUTTON",L"Reset defaults",BS_PUSHBUTTON|WS_TABSTOP,238,610,150,34,Reset);
    status=make(L"STATIC",L"Integration pending: entity data, ballistics, weapon and vehicle controls.",0,24,663,770,36);
    load();
}
LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_COMMAND:
        if(HIWORD(w)==BN_CLICKED) {
            int id=LOWORD(w);
            if(id==Save) save();
            else if(id==Travel) dependencies();
            else if(id==Reset) {
                // Reset through a fresh process-independent snapshot of defaults.
                settings=fc5::Settings{};
                for(auto& b:bindings) {
                    if(b.key==L"aim.fov") b.write(L"10");
                    else if(b.key==L"vehicle.fov") b.write(L"90");
                    else if(b.key==L"vehicle.speed") b.write(L"1");
                    else if(b.key==L"aim.enemies"||b.key==L"esp.enemies"||b.key==L"aim.vehicleWeapons"||
                            b.key==L"aim.travelTime"||b.key==L"aim.prediction"||b.key==L"aim.bulletDrop") b.write(L"1");
                    else if(b.key.find(L"esp.color")==0) continue;
                    else b.write(L"0");
                }
                dependencies();colorsText();SetWindowTextW(status,L"Defaults restored. Save to keep them.");
            } else if(id>=ColorBase&&id<ColorBase+3) {
                int index=id-ColorBase;auto c=settings.colors[index];
                static COLORREF custom[16]{};
                CHOOSECOLORW chooser{sizeof(chooser)};chooser.hwndOwner=hwnd;
                chooser.rgbResult=RGB(c.r,c.g,c.b);chooser.lpCustColors=custom;
                chooser.Flags=CC_FULLOPEN|CC_RGBINIT;
                if(ChooseColorW(&chooser)) {
                    settings.colors[index]={GetRValue(chooser.rgbResult),GetGValue(chooser.rgbResult),GetBValue(chooser.rgbResult)};
                    colorsText();
                }
            }
        }
        return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,message,w,l);
}
}
int runMenu(HINSTANCE instance,bool smoke,int show) {
    menuModule=instance;
    settings=fc5::Settings{};
    bindings.clear();numericControls.clear();
    // Class callbacks and control closures must be gone before DLL unloading.
    struct Cleanup {
        HINSTANCE instance;
        ~Cleanup() {
            if(window&&IsWindow(window)) DestroyWindow(window);
            bindings.clear();numericControls.clear();
            if(font) DeleteObject(font);
            UnregisterClassW(L"FC5MenuPreview",instance);
            MSG pending{};
            while(PeekMessageW(&pending,nullptr,WM_QUIT,WM_QUIT,PM_REMOVE)) {}
            window=nullptr;status=nullptr;font=nullptr;
        }
    } cleanup{instance};
    wchar_t exe[32768]{};GetModuleFileNameW(instance,exe,32768);
    configPath=std::filesystem::path(exe).parent_path()/L"fc5-menu.ini";
    font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    WNDCLASSW wc{};wc.hInstance=instance;wc.lpszClassName=L"FC5MenuPreview";
    wc.lpfnWndProc=proc;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
    if(!RegisterClassW(&wc)) return 1;
    RECT rect{0,0,820,718};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
    window=CreateWindowW(wc.lpszClassName,L"Far Cry 5 - Menu preview (not connected)",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,
        rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if(!window) return 2;
    try {build();} catch(...) {return 3;}
    if(smoke) {
        bool ok=bindings.size()==24 && IsWindow(status);
        return ok?0:4;
    }
    ShowWindow(window,show);
    MSG msg{};BOOL result;
    while((result=GetMessageW(&msg,nullptr,0,0))>0) {
        if(!IsDialogMessageW(window,&msg)) {TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    return result<0?5:static_cast<int>(msg.wParam);
}
