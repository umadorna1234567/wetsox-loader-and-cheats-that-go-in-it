#pragma once
#include <cstdint>
#include <string>
#include "game/device.h"
namespace mod {
void Render(jc::HDevice_t* device);
void Text(const std::string& text, float x, float y, float size, uint32_t color=0xffffffff);
void Rect(float x,float y,float w,float h,uint32_t color);
void Line(float x,float y,float endX,float endY,uint32_t color);
void DrawCombat();
float Aspect();
}
