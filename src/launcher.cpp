#include <windows.h>
#include <string>
#include <filesystem>
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR arguments,int) {
 wchar_t path[32768]{};
 if(!GetModuleFileNameW(nullptr,path,32768))return 1;
 const auto root=std::filesystem::path(path).parent_path();
 const auto app=root/L"backend"/L"WetsoxApp.exe";
 if(!std::filesystem::is_regular_file(app)) {
  MessageBoxW(nullptr,L"The backend folder is missing. Extract the complete Wetsox loader download beside Wetsox.exe.",L"Wetsox",MB_ICONERROR);return 1;
 }
 std::wstring command=L"\""+app.wstring()+L"\" "+arguments;
 STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
 if(!CreateProcessW(app.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,root.c_str(),&startup,&process)) {
  MessageBoxW(nullptr,L"Wetsox could not start. Keep the full backend folder with the launcher and install the included Visual C++ runtime if needed.",L"Wetsox",MB_ICONERROR);return 1;
 }
 CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);
 DWORD code=1;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);return int(code);
}
