#pragma once
#include <windows.h>
#include <cstdint>
namespace fc5::input {
bool start(); // MinHook must already be initialized.
void stop();
void menu(bool open,HWND window);
bool capturing();
std::uint64_t blockedPolls();
}
