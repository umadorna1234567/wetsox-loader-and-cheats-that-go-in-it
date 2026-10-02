#include "render.h"
#include "menu.h"
#include "game.h"
#include <d3d11.h>
#include <CFW1StateSaver.h>
#include <FW1FontWrapper.h>
#include "game/debug_renderer_impl.h"
#include "combat_math.h"
#include <cstdio>

namespace mod {
namespace {
IFW1FontWrapper* font=nullptr;
jc::HDevice_t* device=nullptr;
FW1FontWrapper::CFW1StateSaver saver;
ID3D11Device* fontDevice=nullptr;
uintptr_t aimTarget=0;
ULONGLONG lastAim=0;
float aimRemainderX=0,aimRemainderY=0;
}
float Aspect() {return device && device->m_screenHeight>0 ? float(device->m_screenWidth)/device->m_screenHeight : 1.77778f;}
void Text(const std::string& text,float x,float y,float size,uint32_t color) {
    if(!font || !device) return;
    std::wstring wide(text.begin(),text.end());
    font->DrawString(device->m_deviceContext,wide.c_str(),size*device->m_screenHeight,
        x*device->m_screenWidth,y*device->m_screenHeight,color,0);
}
void Rect(float x,float y,float w,float h,uint32_t color) {
    auto engine=*reinterpret_cast<uintptr_t*>(Address(0x142c84be8));
    if(!engine) return;
    auto renderer=*reinterpret_cast<jc::DebugRendererImpl**>(engine+0x2bc8);
    if(renderer) renderer->DebugRectGradient({x,y},{x+w,y+h},color,color);
}
void Line(float x,float y,float endX,float endY,uint32_t color) {
    auto engine=*reinterpret_cast<uintptr_t*>(Address(0x142c84be8));
    if(!engine) return;
    auto renderer=*reinterpret_cast<jc::DebugRendererImpl**>(engine+0x2bc8);
    if(renderer) renderer->DebugLine({x,y},{endX,endY},color,color);
}
void DrawCombat() {
    // Called under stateMutex. Projection uses the current render camera;
    // character data is copied by the game update, never followed from here.
    auto now=GetTickCount64();
    auto resetAim=[&](){aimTarget=0;aimRemainderX=aimRemainderY=0;lastAim=now;};
    CMatrix4f camera;
    if(!device || !snapshot.ready || !InGameplay() || now-snapshot.charactersTime>150 ||
       (!settings.esp && !settings.aim) || !ReadCamera(camera)) {resetAim();return;}
    float aspect=Aspect(),radius=AimRadius(camera,settings.aimFov);
    if(settings.aim && radius>0) {
        for(int i=0;i<96;i++) {
            float a=i*6.2831853f/96,b=(i+1)*6.2831853f/96;
            Line(.5f+std::cos(a)*radius/aspect,.5f+std::sin(a)*radius,
                 .5f+std::cos(b)*radius/aspect,.5f+std::sin(b)*radius,0x99ffffff);
        }
    }
    const EspCharacter* best=nullptr;CVector2f bestPoint{};float bestDistance=radius;
    for(const auto& entity:snapshot.characters) {
        CVector2f top,feet,aim;
        if(!Project(camera,entity.feet,feet) || !Project(camera,entity.top,top) || !Project(camera,entity.aimPoint,aim)) continue;
        bool onScreen=aim.x>=0 && aim.x<=1 && aim.y>=0 && aim.y<=1;
        float distance=AimDistance(aim,aspect);
        if(settings.aim && entity.enemy && onScreen && distance<=radius &&
           (!best || (best->id!=aimTarget && (entity.id==aimTarget || distance<bestDistance)))) {
            best=&entity;bestPoint=aim;bestDistance=distance;
        }
        if(!settings.esp || (settings.espEnemiesOnly && !entity.enemy) || !onScreen) continue;
        float height=std::abs(feet.y-top.y);
        if(height<.002f || height>2) continue;
        float halfWidth=height*.23f/aspect,center=(feet.x+top.x)*.5f;
        float left=std::clamp(center-halfWidth,0.f,1.f),right=std::clamp(center+halfWidth,0.f,1.f);
        float upper=std::clamp(std::min(top.y,feet.y),0.f,1.f),lower=std::clamp(std::max(top.y,feet.y),0.f,1.f);
        uint32_t color=entity.enemy?0xff6666ff:0xffffbb66;
        float px=1.f/device->m_screenWidth,py=1.f/device->m_screenHeight;
        for(int border=2;border>=1;--border) {
            float tx=px*border,ty=py*border;auto c=border==2?0xdd000000:color;
            Rect(left,upper,right-left,ty,c);Rect(left,lower-ty,right-left,ty,c);
            Rect(left,upper,tx,lower-upper,c);Rect(right-tx,upper,tx,lower-upper,c);
        }
        char label[80];std::snprintf(label,sizeof(label),"%s  %.0f m  |  %d HP",entity.enemy?"Enemy":"Other",entity.distance,entity.health);
        Text(label,std::clamp(left,0.f,.80f),std::clamp(upper-.018f,0.f,.96f),.013f,color);
        if(settings.espTracers) Line(.5f,.98f,std::clamp(feet.x,0.f,1.f),lower,color);
    }
    if(settings.aim && best) {
        float r=.005f;
        Line(bestPoint.x-r/aspect,bestPoint.y,bestPoint.x+r/aspect,bestPoint.y,0xff6dffbb);
        Line(bestPoint.x,bestPoint.y-r,bestPoint.x,bestPoint.y+r,0xff6dffbb);
    }
    DWORD foregroundPid=0;
    auto foreground=GetForegroundWindow();GetWindowThreadProcessId(foreground,&foregroundPid);
    if(!settings.aim || MenuOpen() || foregroundPid!=GetCurrentProcessId() ||
       !AimInputHeld() || !best) {resetAim();return;}
    if(aimTarget!=best->id) {aimRemainderX=aimRemainderY=0;aimTarget=best->id;}
    if(!lastAim) lastAim=now;
    if(now-lastAim<8) return;
    float dt=static_cast<float>(now-lastAim)/1000;lastAim=now;
    auto step=AimStep((bestPoint.x-.5f)*device->m_screenWidth,(bestPoint.y-.5f)*device->m_screenHeight,settings.aimSmooth,dt);
    aimRemainderX+=step.x;aimRemainderY+=step.y;
    LONG dx=static_cast<LONG>(aimRemainderX),dy=static_cast<LONG>(aimRemainderY);
    aimRemainderX-=dx;aimRemainderY-=dy;
    if(dx || dy) {
        INPUT input{};input.type=INPUT_MOUSE;input.mi.dx=dx;input.mi.dy=dy;input.mi.dwFlags=MOUSEEVENTF_MOVE;
        SendInput(1,&input,sizeof(input));
    }
}
void Render(jc::HDevice_t* current) {
    if(!current || !current->m_device || !current->m_deviceContext) return;
    if(fontDevice && fontDevice!=current->m_device && font) {
        font->Release(); font=nullptr; saver.releaseSavedState();
    }
    device=current;
    if(!font) {
        IFW1Factory* factory=nullptr;
        if(FAILED(FW1CreateFactory(FW1_VERSION,&factory))) return;
        factory->CreateFontWrapper(device->m_device,L"Segoe UI",&font);
        factory->Release();
        if(!font) return;
        fontDevice=device->m_device;
        Log("Renderer initialized.");
    }
    if(FAILED(saver.saveCurrentState(device->m_deviceContext))) return;
    DrawOverlay();
    saver.restoreSavedState();
}
}
