#include "fc5/overlay.hpp"
#include "fc5/session.hpp"
#include "fc5/controller.hpp"
#include "fc5/targeting.hpp"
#include "fc5/runtime.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <atomic>
#include <mutex>
#include <cmath>
#include <algorithm>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
using Microsoft::WRL::ComPtr;
namespace {
using Present=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using Resize=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT);
Present originalPresent{};
Resize originalResize{};
void *presentAddress{},*resizeAddress{};
std::mutex lifecycle;
std::recursive_mutex render;
std::atomic<bool> active{false};
bool visible=true,ready=false,hooksCreated=false;
std::uint64_t frames{},resizes{};
HWND gameWindow{};
IDXGISwapChain* selected{}; // Identity only; do not keep the game's swap chain alive.
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<ID3D11RenderTargetView> target;
ImGuiContext* gui{};
fc5::Settings settings;
bool allHumans=true;

struct GuiScope {
    ImGuiContext* previous=ImGui::GetCurrentContext();
    GuiScope() {ImGui::SetCurrentContext(gui);}
    ~GuiScope() {ImGui::SetCurrentContext(previous);}
};

HANDLE sessionMapping{},sessionMutex{};
nexus::Session* session{};
unsigned aimVk=VK_RBUTTON,aimModifiers{};
bool menuActive=true;
bool openSession() {
    if(session)return true;
    sessionMutex=CreateMutexW(nullptr,FALSE,nexus::mutexName(GetCurrentProcessId()).c_str());
    sessionMapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(nexus::Session),nexus::sessionName(GetCurrentProcessId()).c_str());
    if(!sessionMutex||!sessionMapping)return false;
    session=static_cast<nexus::Session*>(MapViewOfFile(sessionMapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));
    if(!session)return false;
    auto wait=WaitForSingleObject(sessionMutex,1000);
    if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return false;
    *session=nexus::Session{};ReleaseMutex(sessionMutex);return true;
}
void exchange() {
    bool fresh=false;
    auto wait=WaitForSingleObject(sessionMutex,0);
    if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED) {
        if(session->version==nexus::sessionVersion&&GetTickCount64()-session->heartbeat<3000) {
            settings=session->settings;allHumans=session->allHumans;
            menuActive=session->menuActive;aimVk=session->aimKey;aimModifiers=session->modifiers;fresh=true;
        }
        session->status=fc5::runtime::status();session->frames=frames;
        ReleaseMutex(sessionMutex);
    } else return;
    if(!fresh) {settings=fc5::Settings{};menuActive=true;}
}
bool aimHeld() {
    if(!aimVk)return true; // Activation has been evaluated by the loader.
    return wetsox::inputHeld(aimVk) &&
        (!(aimModifiers&1)||(GetAsyncKeyState(VK_CONTROL)&0x8000)) &&
        (!(aimModifiers&2)||(GetAsyncKeyState(VK_SHIFT)&0x8000)) &&
        (!(aimModifiers&4)||(GetAsyncKeyState(VK_MENU)&0x8000)) &&
        (!(aimModifiers&8)||((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN))&0x8000));
}

bool createTarget(IDXGISwapChain* chain) {
    ComPtr<ID3D11Texture2D> back;
    return SUCCEEDED(chain->GetBuffer(0,IID_PPV_ARGS(&back)))&&
           SUCCEEDED(device->CreateRenderTargetView(back.Get(),nullptr,&target));
}

bool initialize(IDXGISwapChain* chain) {
    DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(chain->GetDesc(&desc))||!desc.OutputWindow||!IsWindowVisible(desc.OutputWindow)) return false;
    DWORD owner{};GetWindowThreadProcessId(desc.OutputWindow,&owner);
    if(owner!=GetCurrentProcessId()) return false;
    if(FAILED(chain->GetDevice(IID_PPV_ARGS(&device)))) return false;
    device->GetImmediateContext(&context);
    if(!createTarget(chain)) {device.Reset();context.Reset();return false;}
    auto prior=ImGui::GetCurrentContext();
    gui=ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    // The game owns the OS cursor. ImGui draws its own only while open;
    // otherwise its Win32 backend would keep restoring an arrow every frame.
    io.ConfigFlags|=ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    bool win=ImGui_ImplWin32_Init(desc.OutputWindow);
    bool dx=win&&ImGui_ImplDX11_Init(device.Get(),context.Get());
    if(!dx) {
        if(win) ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(gui);gui=nullptr;ImGui::SetCurrentContext(prior);
        target.Reset();context.Reset();device.Reset();return false;
    }
    gameWindow=desc.OutputWindow;
    ImGui::SetCurrentContext(prior);
    selected=chain;ready=true;return true;
}

HRESULT STDMETHODCALLTYPE presentHook(IDXGISwapChain* chain,UINT sync,UINT flags) {
    {
        std::lock_guard lock(render);
        if(active&&!(flags&DXGI_PRESENT_TEST)) {
            if(!ready) initialize(chain);
            if(ready&&selected==chain&&(target||createTarget(chain))) {
                GuiScope scope;
                ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
                ImGui::GetIO().MouseDrawCursor=visible;
                exchange();
                fc5::runtime::refresh();
                fc5::runtime::setNoReload(settings.noReload);
                fc5::runtime::setUnlimitedAmmo(settings.unlimitedAmmo);
                
                fc5::runtime::aim(settings.aim,!menuActive&&GetForegroundWindow()==gameWindow&&aimHeld(),ImGui::GetIO().DeltaTime);
                fc5::runtime::cameraFov(settings.playerFovOverride,settings.playerFovDegrees,settings.vehicleFovOverride,settings.vehicleFovDegrees);
                auto size=ImGui::GetIO().DisplaySize;
                auto draw=ImGui::GetBackgroundDrawList();
                if(settings.aim.showFov) {
                    float rx{},ry{};
                    if(fc5::runtime::aimFovRadii(settings.aim.fovDegrees,size.x,size.y,rx,ry)) {
                        constexpr int segments=128;
                        ImVec2 points[segments];
                        for(int i=0;i<segments;++i) {float a=float(i)*6.283185307f/segments;points[i]={size.x*.5f+std::cos(a)*rx,size.y*.5f+std::sin(a)*ry};}
                        draw->AddPolyline(points,segments,IM_COL32(180,130,255,220),ImDrawFlags_Closed,1.5f);
                    }
                }
                if(settings.esp&&(settings.boxEsp||settings.boneEsp)) {
                    // Body chains use resolved names, never shared numeric bone indices.
                    constexpr unsigned links[][2]{{0,4},{4,1},{1,5},{5,2},{2,3},
                        {1,6},{6,7},{7,8},{8,9},{1,10},{10,11},{11,12},{12,13},
                        {3,14},{14,15},{15,16},{3,17},{17,18},{18,19}};
                    const fc5::Filters visibleKinds{allHumans||settings.espTargets.enemies,allHumans,settings.espTargets.animals};
                    for(const auto& pawn:fc5::runtime::visualPawns(visibleKinds)) {
                        if(pawn.health<=0)continue;
                        const bool cult=!pawn.animal&&(pawn.faction==0||pawn.faction==1);
                        if(!(pawn.animal?settings.espTargets.animals:allHumans||(cult&&settings.espTargets.enemies)))continue;
                        auto color=settings.colors[pawn.animal?2:cult?0:1];
                        auto packed=IM_COL32(color.r,color.g,color.b,255);
                        ImVec2 points[20]{};bool valid[20]{};unsigned count=0;
                        float left=size.x,top=size.y,right=0,bottom=0;
                        for(unsigned i=0;i<20;++i)if(pawn.boneMask&(1u<<i)) {
                            valid[i]=fc5::runtime::project(pawn.bones[i],size.x,size.y,points[i].x,points[i].y);
                            if(valid[i]) {++count;left=std::min(left,points[i].x);right=std::max(right,points[i].x);top=std::min(top,points[i].y);bottom=std::max(bottom,points[i].y);}
                        }
                        for(const auto& bound:pawn.boundsPoints) {
                            float x{},y{};
                            if(fc5::runtime::project(bound,size.x,size.y,x,y)) {
                                ++count;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
                            }
                        }
                        if(settings.boxEsp&&count>=2) {
                            float padding=std::max(3.f,(bottom-top)*.07f);
                            float half=std::max((right-left)*.5f+padding,(bottom-top)*.22f);
                            float center=(left+right)*.5f;
                            if(settings.espOutline)draw->AddRect({center-half,top-padding},{center+half,bottom+padding},IM_COL32(0,0,0,190),0,0,3.5f);
                            draw->AddRect({center-half,top-padding},{center+half,bottom+padding},packed,0,0,1.5f);
                        }
                        if(settings.boneEsp&&!pawn.skeleton.empty())for(const auto& link:pawn.skeleton) {
                            ImVec2 a{},b{};
                            if(!fc5::runtime::project(link[0],size.x,size.y,a.x,a.y)||!fc5::runtime::project(link[1],size.x,size.y,b.x,b.y))continue;
                            if(settings.espOutline)draw->AddLine(a,b,IM_COL32(0,0,0,190),3.5f);
                            draw->AddLine(a,b,packed,1.5f);
                        }
                        if(settings.boneEsp&&pawn.skeleton.empty())for(auto& link:links)if(valid[link[0]]&&valid[link[1]]) {
                            if(settings.espOutline)draw->AddLine(points[link[0]],points[link[1]],IM_COL32(0,0,0,190),3.5f);
                            draw->AddLine(points[link[0]],points[link[1]],packed,1.5f);
                        }
                    }
                }
                ImGui::Render();
                // Backend preserves pipeline state, but OM bindings changed by
                // this caller must also be preserved, including multiple RTVs.
                ID3D11RenderTargetView* saved[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
                ID3D11DepthStencilView* depth{};
                context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,saved,&depth);
                auto view=target.Get();context->OMSetRenderTargets(1,&view,nullptr);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,saved,depth);
                for(auto* old:saved) if(old) old->Release();
                if(depth) depth->Release();
                ++frames;
            }
        }
    }
    return originalPresent(chain,sync,flags);
}
HRESULT STDMETHODCALLTYPE resizeHook(IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags) {
    std::lock_guard lock(render);
    if(chain==selected) {target.Reset();++resizes;}
    return originalResize(chain,count,width,height,format,flags);
}

DWORD discover() {
    const auto instance=GetModuleHandleW(nullptr);
    WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"FC5OverlayProbe";
    if(!RegisterClassW(&wc)) return GetLastError();
    HWND probe=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPEDWINDOW,0,0,64,64,nullptr,nullptr,instance,nullptr);
    if(!probe) {auto error=GetLastError();UnregisterClassW(wc.lpszClassName,instance);return error;}
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=1;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=probe;desc.SampleDesc.Count=1;desc.Windowed=TRUE;
    ComPtr<IDXGISwapChain> chain;ComPtr<ID3D11Device> probeDevice;ComPtr<ID3D11DeviceContext> probeContext;
    HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&chain,&probeDevice,nullptr,&probeContext);
    if(FAILED(hr)) hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&chain,&probeDevice,nullptr,&probeContext);
    if(SUCCEEDED(hr)) {
        auto table=*reinterpret_cast<void***>(chain.Get());presentAddress=table[8];resizeAddress=table[13];
    }
    chain.Reset();probeContext.Reset();probeDevice.Reset();DestroyWindow(probe);UnregisterClassW(wc.lpszClassName,instance);
    return SUCCEEDED(hr)?ERROR_SUCCESS:static_cast<DWORD>(hr);
}
}

extern "C" DWORD WINAPI FC5OverlayStart(void*) {
    std::lock_guard lock(lifecycle);
    if(active) return ERROR_SUCCESS;
    {std::lock_guard state(render);if(ready) return ERROR_BUSY;}
    if(!hooksCreated) {
        DWORD result=discover();if(result) return result;
        if(MH_Initialize()!=MH_OK) return ERROR_DLL_INIT_FAILED;
        if(MH_CreateHook(presentAddress,reinterpret_cast<void*>(presentHook),reinterpret_cast<void**>(&originalPresent))!=MH_OK) {
            MH_Uninitialize();return ERROR_INVALID_FUNCTION;
        }
        if(MH_CreateHook(resizeAddress,reinterpret_cast<void*>(resizeHook),reinterpret_cast<void**>(&originalResize))!=MH_OK) {
            MH_RemoveHook(presentAddress);MH_Uninitialize();return ERROR_INVALID_FUNCTION;
        }
        hooksCreated=true;
    }
    // A disabled detour may still have callers on its stack. Keep code and its
    // trampolines mapped until process exit; never guess a sleep-to-unload delay.
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&FC5OverlayStart),&pinned)) return GetLastError();
    if(!openSession()) return ERROR_NOT_ENOUGH_MEMORY;
    {std::lock_guard state(render);visible=false;frames=0;resizes=0;active=true;}
    wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);
    bool testHost=std::wstring(exe).ends_with(L"NexusFC5TestHost.exe");
    if(!fc5::runtime::start()&&!testHost) {active=false;return ERROR_REVISION_MISMATCH;}
    MH_QueueEnableHook(presentAddress);MH_QueueEnableHook(resizeAddress);
    if(MH_ApplyQueued()!=MH_OK) {
        active=false;fc5::runtime::stop();MH_DisableHook(presentAddress);MH_DisableHook(resizeAddress);return ERROR_INVALID_FUNCTION;
    }
    return ERROR_SUCCESS;
}
extern "C" DWORD WINAPI FC5OverlayStop(void*) {
    std::lock_guard lock(lifecycle);
    if(!hooksCreated) return ERROR_SUCCESS;
    active=false;
    fc5::runtime::stop();
    MH_QueueDisableHook(presentAddress);MH_QueueDisableHook(resizeAddress);
    if(MH_ApplyQueued()!=MH_OK) return ERROR_INVALID_FUNCTION;
    std::lock_guard state(render);
    if(ready) {
        auto prior=ImGui::GetCurrentContext();ImGui::SetCurrentContext(gui);
        ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext(gui);
        ImGui::SetCurrentContext(prior==gui?nullptr:prior);gui=nullptr;
    }
    target.Reset();context.Reset();device.Reset();ready=false;selected=nullptr;visible=false;
    return ERROR_SUCCESS;
}
extern "C" DWORD WINAPI FC5OverlayStatus(OverlayStatus* output) {
    if(!output||output->size!=sizeof(OverlayStatus)) return ERROR_INVALID_PARAMETER;
    std::lock_guard lock(render);
    *output={sizeof(OverlayStatus),active?1u:0u,ready?1u:0u,visible?1u:0u,frames,resizes};
    return ERROR_SUCCESS;
}
