#include "fc5/menu.hpp"
#include <string>
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR command,int show) {
    return runMenu(instance,std::wstring(command)==L"--smoke-test",show);
}
