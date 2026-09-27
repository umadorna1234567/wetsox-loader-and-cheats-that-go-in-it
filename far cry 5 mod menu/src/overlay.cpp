#include "fc5/overlay.hpp"
#include "fc5/targeting.hpp"
#include "fc5/runtime.hpp"
#include "fc5/input.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <atomic>
#include <mutex>

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
WNDPROC previousProc{};
IDXGISwapChain* selected{}; // Identity only; do not keep the game's swap chain alive.
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<ID3D11RenderTargetView> target;
ImGuiContext* gui{};
fc5::Settings settings;
bool diagnosticMarkers=false;
bool allHumans=true;
bool boneMarkers=false;
int aimKey=0;

struct GuiScope {
    ImGuiContext* previous=ImGui::GetCurrentContext();
    GuiScope() {ImGui::SetCurrentContext(gui);}
    ~GuiScope() {ImGui::SetCurrentContext(previous);}
};

LRESULT CALLBACK windowProc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    WNDPROC next;
    {
        std::lock_guard lock(render);
        next=previousProc;
        if(active&&ready) {
            GuiScope scope;
            if(message==WM_KEYUP&&w==VK_INSERT) {visible=!visible;fc5::input::menu(visible,gameWindow);return 0;}
            if(message==WM_KEYDOWN&&w==VK_INSERT) return 0;
            ImGui_ImplWin32_WndProcHandler(hwnd,message,w,l);
            if(visible) {
                // DirectInput is neutralized separately; consume UI messages
                // here so no click is delivered to the game window procedure.
                bool keyboard=message>=WM_KEYFIRST&&message<=WM_KEYLAST;
                bool mouse=message>=WM_MOUSEFIRST&&message<=WM_MOUSELAST;
                if(keyboard||mouse) return 0;
                if(message==WM_INPUT) return DefWindowProcW(hwnd,message,w,l);
                if(message==WM_SETCURSOR){SetCursor(nullptr);return TRUE;}
            }
        }
    }
    return next?CallWindowProcW(next,hwnd,message,w,l):DefWindowProcW(hwnd,message,w,l);
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
    SetLastError(0);
    previousProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(windowProc)));
    if(!previousProc) {
        ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext(gui);
        gui=nullptr;ImGui::SetCurrentContext(prior);target.Reset();context.Reset();device.Reset();gameWindow=nullptr;
        return false;
    }
    ImGui::SetCurrentContext(prior);
    selected=chain;ready=true;fc5::input::menu(visible,gameWindow);return true;
}

void slider(const char* label,double& value,float lo,float hi,const char* format="%.1f") {
    float v=static_cast<float>(value);
    if(ImGui::SliderFloat(label,&v,lo,hi,format)) value=v;
}
void filters(fc5::Filters& f) {
    ImGui::Checkbox("Cult / Blessed humans",&f.enemies);
    ImGui::Checkbox("Other humans",&f.otherHumans);
    ImGui::Checkbox("Animals",&f.animals);
}
void menu() {
    if(!visible) return;
    ImGui::SetNextWindowSize(ImVec2(630,550),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(40,40),ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Far Cry 5 | Offline menu",&visible)) {
        ImGui::TextColored(ImVec4(.95f,.72f,.3f,1),"ESP, on-foot aiming and ammunition controls connected.");
        ImGui::TextWrapped("Insert: show/hide. Menu captures mouse and keyboard while open.");
        ImGui::Separator();
        if(ImGui::BeginTabBar("tabs")) {
            if(ImGui::BeginTabItem("Aiming")) {
                ImGui::Checkbox("Aimbot",&settings.aim.enabled);filters(settings.aim.targets);
                ImGui::Combo("Hold to aim",&aimKey,"Right mouse\0Left Alt\0Mouse side button\0");
                ImGui::Checkbox("Sticky aim",&settings.aim.stickyAim);
                ImGui::TextWrapped("Sticky aim keeps the same eligible target while the aim key is held; release the key to reset.");
                ImGui::Combo("Vehicle occupant cover",&settings.aim.vehicleOccupantCover,"Normal cover checks\0Ignore occupant's vehicle\0");
                ImGui::TextWrapped("On-foot aiming. Projectile compensation is experimental for recognized bullet weapons; mounted aiming is pending. Motion prediction estimates constant velocity from position samples. Clear line of sight to the selected body part is required. Exposed/covered target test passed.");
                ImGui::Checkbox("Mounted weapons",&settings.aim.vehicleWeapons);
                int hit=static_cast<int>(settings.aim.hitLocation);
                if(ImGui::Combo("Hit location",&hit,"Head\0Chest\0Abdomen\0Pelvis\0")) settings.aim.hitLocation=static_cast<fc5::HitLocation>(hit);
                slider("Aim FOV diameter",settings.aim.fovDegrees,.1f,180);
                slider("Smoothing (seconds)",settings.aim.smoothingSeconds,0,2,"%.3f");
                ImGui::Checkbox("Projectile travel time",&settings.aim.travelTime);
                ImGui::BeginDisabled(!settings.aim.travelTime);
                ImGui::Checkbox("Motion prediction",&settings.aim.prediction);
                ImGui::Checkbox("Bullet drop",&settings.aim.bulletDrop);ImGui::EndDisabled();
                ImGui::TextUnformatted("Weapon spread stays unchanged.");ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("ESP")) {
                ImGui::Checkbox("Diagnostic pawn positions (unclassified)",&diagnosticMarkers);
                ImGui::TextWrapped("Markers use entity origins, not bones. When ESP is enabled, its filters replace diagnostic markers.");
                ImGui::Checkbox("ESP",&settings.esp);
                ImGui::Checkbox("All humans",&allHumans);
                ImGui::BeginDisabled(allHumans);
                ImGui::Checkbox("Cult / Blessed humans only",&settings.espTargets.enemies);
                ImGui::EndDisabled();
                ImGui::Checkbox("Animals",&settings.espTargets.animals);
                ImGui::Checkbox("Show named body-part markers",&boneMarkers);
                ImGui::TextWrapped("Human classification uses faction, not current per-character hostility. Unknown factions use the other-human color.");
                const char* names[]{"Cult / Blessed color","Other human color","Animal color"};
                for(int i=0;i<3;++i) {
                    auto& c=settings.colors[i];float rgb[]{c.r/255.f,c.g/255.f,c.b/255.f};
                    if(ImGui::ColorEdit3(names[i],rgb)) c={static_cast<std::uint8_t>(rgb[0]*255),static_cast<std::uint8_t>(rgb[1]*255),static_cast<std::uint8_t>(rgb[2]*255)};
                }
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Weapons / vehicles")) {
                ImGui::Checkbox("Unlimited reserve ammunition",&settings.unlimitedAmmo);
                ImGui::Checkbox("No reload",&settings.noReload);
                ImGui::TextWrapped("No reload preserves single-round magazine consumption on local-player weapons. Reserve ammo and no reload have passed live checks.");
                ImGui::Checkbox("Override vehicle camera FOV",&settings.vehicleFovOverride);
                slider("Vehicle camera FOV",settings.vehicleFovDegrees,40,140);
                ImGui::TextWrapped("Vehicle FOV restores on disable or exit. Live vehicle test pending.");
                ImGui::BeginDisabled();
                ImGui::Checkbox("Override vehicle speed",&settings.vehicleSpeedOverride);
                slider("Speed multiplier",settings.vehicleSpeedMultiplier,.1f,5,"%.2fx");
                ImGui::EndDisabled();ImGui::TextUnformatted("Vehicle speed binding: pending");
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Status")) {
                auto data=fc5::runtime::status();
                ImGui::Text("Supported engine: %s | Entities: %u | Animals: %u",data.supported?"yes":"no",data.pawnCount,data.animalCount);
                ImGui::Text("Player: %.2f, %.2f, %.2f",data.localPosition[0],data.localPosition[1],data.localPosition[2]);
                ImGui::Text("Snapshots: %llu | Matched projections: %llu | Mismatches: %llu",data.snapshots,data.projections,data.mismatches);
                ImGui::Text("Overlay frames: %llu",static_cast<unsigned long long>(frames));
                ImGui::Text("Menu input capture: %s | Blocked polls: %llu",fc5::input::capturing()?"active":"off",fc5::input::blockedPolls());
                ImGui::Text("Magazine hook: %u | Calls: %llu | Preserved rounds: %llu",data.magazineHook,data.magazineCalls,data.preservedRounds);
                ImGui::Text("Reserve hook: %u | Overrides: %llu | Entities with bones: %u",data.unlimitedHook,data.unlimitedQueries,data.boneEntities);
                const char* aimStates[]{"Inactive","Unsupported weapon/ballistics: direct aim available","Player/camera unavailable","Mounted control pending","No eligible target","Applying aim","View/camera alignment check failed","Cover blocks aim / visibility unconfirmed"};
                ImGui::Text("Aim: %s | Writes: %llu | Alignment: %.4f",aimStates[data.aimState<8?data.aimState:2],data.aimWrites,data.centerError);
                ImGui::Text("Visibility queries: %llu | Clear: %llu | Blocked/unconfirmed: %llu",data.visibilityQueries,data.visibilityClear,data.visibilityBlocked);
                ImGui::Text("Launch observer: %u | Samples: %llu | Bullet physics: %u",data.launchHook,data.launchSamples,data.launchPhysics);
                ImGui::Text("Last launch speed: %.2f | Gravity: %.2f | Drop distance: %.2f",data.projectileSpeed,data.projectileGravity,data.projectileDropDistance);
                ImGui::Text("Active speed: %.2f | Gravity: %.2f | Drop distance: %.2f",data.activeSpeed,data.activeGravity,data.activeDrop);
                ImGui::Text("Simulation step: %.5f s | Estimated flight: %.3f s",data.simulationStep,data.flightSeconds);
                ImGui::TextWrapped("Long-range accuracy is not yet validated. Compensation assumes the current simulation step continues; changing frame timing and target motion can introduce error.");
                const char* fovStates[]{"Off","Waiting for vehicle","Applied","Another camera override is active","Camera write failed"};
                ImGui::Text("Vehicle FOV: %s | %.1f degrees",fovStates[data.vehicleFovState<5?data.vehicleFovState:4],data.vehicleFovDegrees);
                ImGui::TextUnformatted("Mounted aiming / vehicle speed: pending\nSettings persistence: preview EXE only");
                ImGui::TextWrapped("Stop the overlay with FC5MenuLoader --stop. Hooks and graphics resources are released. The DLL remains resident until the game exits so an in-flight callback cannot jump into unloaded code.");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
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
                fc5::runtime::refresh();
                fc5::runtime::setNoReload(settings.noReload);
                fc5::runtime::setUnlimitedAmmo(settings.unlimitedAmmo);
                const int keys[]{VK_RBUTTON,VK_LMENU,VK_XBUTTON1};
                fc5::runtime::aim(settings.aim,!visible&&GetForegroundWindow()==gameWindow&&(GetAsyncKeyState(keys[aimKey])&0x8000),ImGui::GetIO().DeltaTime);
                fc5::runtime::vehicleCamera(settings.vehicleFovOverride,settings.vehicleFovDegrees);
                if(diagnosticMarkers||settings.esp) {
                    auto size=ImGui::GetIO().DisplaySize;
                    auto draw=ImGui::GetBackgroundDrawList();
                    for(const auto& pawn:fc5::runtime::pawns()) {
                        const bool cult=!pawn.animal&&(pawn.faction==0||pawn.faction==1);
                        if(settings.esp&&!(pawn.animal?settings.espTargets.animals:allHumans||(cult&&settings.espTargets.enemies)))continue;
                        auto color=settings.colors[pawn.animal?2:cult?0:1];
                        auto packed=settings.esp?IM_COL32(color.r,color.g,color.b,255):IM_COL32(255,210,70,255);
                        float x{},y{};
                        if(fc5::runtime::project(pawn.position,size.x,size.y,x,y)) {
                            draw->AddCircle(ImVec2(x,y),5,packed);
                            draw->AddText(ImVec2(x+8,y),packed,pawn.animal?"Animal":cult?"Cult / Blessed":"Human");
                        }
                        if(boneMarkers) {
                            const char* labels[]{"Head","Chest","Abdomen","Pelvis"};
                            for(unsigned i=0;i<4;++i)if((pawn.boneMask&(1u<<i))&&fc5::runtime::project(pawn.bones[i],size.x,size.y,x,y)) {
                                draw->AddCircleFilled(ImVec2(x,y),3,packed);
                                draw->AddText(ImVec2(x+5,y),packed,labels[i]);
                            }
                        }
                    }
                }
                menu();fc5::input::menu(visible,gameWindow);ImGui::Render();
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
    if(active) return ERROR_ALREADY_INITIALIZED;
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
    if(!fc5::input::start())return ERROR_INVALID_FUNCTION;
    {std::lock_guard state(render);visible=true;frames=0;resizes=0;active=true;}
    MH_QueueEnableHook(presentAddress);MH_QueueEnableHook(resizeAddress);
    if(MH_ApplyQueued()!=MH_OK) {
        active=false;fc5::input::stop();MH_DisableHook(presentAddress);MH_DisableHook(resizeAddress);return ERROR_INVALID_FUNCTION;
    }
    fc5::runtime::start();
    return ERROR_SUCCESS;
}
extern "C" DWORD WINAPI FC5OverlayStop(void*) {
    std::lock_guard lock(lifecycle);
    if(!hooksCreated) return ERROR_SUCCESS;
    {
        std::lock_guard state(render);
        if(ready&&IsWindow(gameWindow)&&
           reinterpret_cast<WNDPROC>(GetWindowLongPtrW(gameWindow,GWLP_WNDPROC))!=windowProc)
            return ERROR_BUSY; // A later subclass must detach before we can restore ours.
    }
    active=false;
    fc5::input::stop();
    fc5::runtime::stop();
    MH_QueueDisableHook(presentAddress);MH_QueueDisableHook(resizeAddress);
    if(MH_ApplyQueued()!=MH_OK) return ERROR_INVALID_FUNCTION;
    std::lock_guard state(render);
    if(ready) {
        // Do not clobber a subclass installed after ours. Our retained code then
        // forwards to the original procedure without touching destroyed UI data.
        if(IsWindow(gameWindow)&&reinterpret_cast<WNDPROC>(GetWindowLongPtrW(gameWindow,GWLP_WNDPROC))==windowProc) {
            SetLastError(0);
            if(!SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(previousProc))&&GetLastError())
                return GetLastError();
        }
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
