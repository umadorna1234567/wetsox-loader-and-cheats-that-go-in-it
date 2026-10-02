#pragma once
#include <Windows.h>
namespace mod {
bool MenuEvent(HWND,UINT,WPARAM,LPARAM);
bool MenuOpen();
void DrawOverlay();
void ExchangeSession();
bool AimInputHeld();
}
