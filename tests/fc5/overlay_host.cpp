#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include "fc5/overlay.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using Microsoft::WRL::ComPtr;
void check(bool ok,const char* message){if(!ok) throw std::runtime_error(message);}
LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(hwnd,msg,w,l);
}
// Readback proves that the hook rendered into the actual swap-chain buffer.
std::size_t capture(ID3D11Device* device,ID3D11DeviceContext* context,IDXGISwapChain* chain,bool save) {
    ComPtr<ID3D11Texture2D> back,staging;
    check(SUCCEEDED(chain->GetBuffer(0,IID_PPV_ARGS(&back))),"GetBuffer capture");
    D3D11_TEXTURE2D_DESC desc{};back->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)),"Create staging texture");
    context->CopyResource(staging.Get(),back.Get());
    D3D11_MAPPED_SUBRESOURCE map{};check(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map)),"Map capture");
    std::vector<unsigned char> pixels(desc.Width*desc.Height*4);std::size_t changed=0;
    for(UINT y=0;y<desc.Height;++y) for(UINT x=0;x<desc.Width;++x) {
        auto* source=static_cast<unsigned char*>(map.pData)+y*map.RowPitch+x*4;
        auto* dest=pixels.data()+(y*desc.Width+x)*4;
        dest[0]=source[2];dest[1]=source[1];dest[2]=source[0];dest[3]=255;
        if(source[0]>25||source[1]>25||source[2]>25) ++changed;
    }
    context->Unmap(staging.Get(),0);
    if(save) {
        BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+static_cast<DWORD>(pixels.size());
        BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=desc.Width;info.biHeight=-static_cast<LONG>(desc.Height);info.biPlanes=1;info.biBitCount=32;
        std::ofstream out("overlay-test.bmp",std::ios::binary);
        out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));
        out.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
    }
    return changed;
}
int wmain(int argc,wchar_t** argv) try {
    bool serve=argc>1&&std::wstring(argv[1])==L"--serve";
    bool interactive=argc>1&&std::wstring(argv[1])==L"--interactive";
    bool test=!serve&&!interactive;
    check(argc>1,"Supply DLL path, --serve, or --interactive");
    HINSTANCE instance=GetModuleHandleW(nullptr);
    WNDCLASSW wc{};wc.hInstance=instance;wc.lpszClassName=L"FC5OverlayTestHost";wc.lpfnWndProc=proc;
    check(RegisterClassW(&wc)!=0,"Register host class");
    HWND hwnd=CreateWindowW(wc.lpszClassName,L"FC5 DirectX overlay test host",WS_OVERLAPPEDWINDOW,
        interactive?100:-2000,interactive?100:-2000,960,720,nullptr,nullptr,instance,nullptr);
    check(hwnd!=nullptr,"Create host window");ShowWindow(hwnd,SW_SHOWNOACTIVATE);
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=1;desc.BufferDesc.Width=960;desc.BufferDesc.Height=720;
    desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow=hwnd;desc.SampleDesc.Count=1;desc.Windowed=TRUE;
    // Sequential swap effect preserves the presented backbuffer for readback.
    desc.SwapEffect=DXGI_SWAP_EFFECT_SEQUENTIAL;
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;ComPtr<IDXGISwapChain> chain;
    HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&chain,&device,nullptr,&context);
    if(FAILED(hr)) hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&chain,&device,nullptr,&context);
    check(SUCCEEDED(hr),"Create host DX11 device");
    using Call=DWORD(WINAPI*)(void*);using Query=DWORD(WINAPI*)(OverlayStatus*);
    Call start{},stop{};Query query{};HMODULE dll{};
    if(!serve) {
        wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);
        auto dllPath=interactive?std::filesystem::path(path).parent_path()/L"FC5Menu.dll":std::filesystem::path(argv[1]);
        dll=LoadLibraryW(dllPath.c_str());check(dll!=nullptr,"Load overlay DLL");
        start=reinterpret_cast<Call>(GetProcAddress(dll,"FC5OverlayStart"));stop=reinterpret_cast<Call>(GetProcAddress(dll,"FC5OverlayStop"));
        query=reinterpret_cast<Query>(GetProcAddress(dll,"FC5OverlayStatus"));check(start&&stop&&query,"Resolve overlay exports");
        check(start(nullptr)==0,"Start overlay");
    }
    OverlayStatus status;
    for(int frame=0;frame<(test?80:36000);++frame) {
        MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            if(message.message==WM_QUIT) goto finished;
            TranslateMessage(&message);DispatchMessageW(&message);
        }
        if(test&&frame==15) {
            check(query(&status)==0&&status.initialized&&status.frames>0&&status.visible,"Hook did not render");
            check(capture(device.Get(),context.Get(),chain.Get(),true)>1000,"No overlay pixels in swap chain");
            SendMessageW(hwnd,WM_KEYUP,VK_INSERT,0);
            query(&status);check(!status.visible,"Insert failed to hide");
        }
        if(test&&frame==20) {
            check(capture(device.Get(),context.Get(),chain.Get(),false)==0,"Hidden overlay still draws");
            SendMessageW(hwnd,WM_KEYUP,VK_INSERT,0);query(&status);check(status.visible,"Insert failed to reopen");
        }
        if(test&&frame==30) {
            context->OMSetRenderTargets(0,nullptr,nullptr);
            check(SUCCEEDED(chain->ResizeBuffers(1,900,650,DXGI_FORMAT_UNKNOWN,0)),"ResizeBuffers failed (retained backbuffer?)");
        }
        if(test&&frame==45) {
            query(&status);check(status.resizes==1&&status.frames>30,"Resize recovery failed");
            check(stop(nullptr)==0,"Stop overlay");query(&status);check(!status.running&&!status.initialized,"Stop did not release state");
            check(reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd,GWLP_WNDPROC))==proc,"Window procedure was not restored");
        }
        if(test&&frame==50) {
            check(capture(device.Get(),context.Get(),chain.Get(),false)==0,"Stopped overlay still draws");
            check(start(nullptr)==0,"Restart overlay");
        }
        ComPtr<ID3D11Texture2D> buffer;ComPtr<ID3D11RenderTargetView> view;
        check(SUCCEEDED(chain->GetBuffer(0,IID_PPV_ARGS(&buffer))),"GetBuffer render");
        check(SUCCEEDED(device->CreateRenderTargetView(buffer.Get(),nullptr,&view)),"Create host RTV");
        auto ptr=view.Get();context->OMSetRenderTargets(1,&ptr,nullptr);
        const float background[]{.02f,.03f,.04f,1};context->ClearRenderTargetView(view.Get(),background);
        check(SUCCEEDED(chain->Present(0,0)),"Present failed");
        context->OMSetRenderTargets(0,nullptr,nullptr);
        Sleep(test?1:16);
    }
finished:
    if(!serve) {
        if(test){query(&status);check(status.frames>10,"Restart did not resume rendering");}
        check(stop(nullptr)==0,"Final stop failed");FreeLibrary(dll);
    }
    context->ClearState();chain.Reset();context.Reset();device.Reset();
    if(IsWindow(hwnd)) DestroyWindow(hwnd);UnregisterClassW(wc.lpszClassName,instance);
    std::cout<<"Overlay rendering, input toggle, resize, shutdown and restart checks passed.\n";return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
