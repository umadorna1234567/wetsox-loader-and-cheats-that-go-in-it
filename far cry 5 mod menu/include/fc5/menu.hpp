#pragma once
#include <windows.h>

// Blocking UI loop. Call on a dedicated host thread, outside DllMain.
// Only one instance may run per module. Returns zero on a clean close.
int runMenu(HINSTANCE module, bool smoke, int show);
