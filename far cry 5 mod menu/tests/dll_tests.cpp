#include <windows.h>
#include <iostream>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2) return 1;
    for(int cycle=0;cycle<3;++cycle) {
        HMODULE dll=LoadLibraryW(argv[1]);
        if(!dll) return 2;
        auto run=reinterpret_cast<DWORD(WINAPI*)(DWORD)>(GetProcAddress(dll,"FC5MenuRun"));
        if(!run) {FreeLibrary(dll);return 3;}
        if(run(2)!=ERROR_INVALID_PARAMETER) {FreeLibrary(dll);return 4;}
        // Reopening must not leave a registered WndProc or stale WM_QUIT.
        if(run(1)!=0||run(1)!=0) {FreeLibrary(dll);return 5;}
        if(!FreeLibrary(dll)) return 6;
    }
    std::cout<<"DLL load, reopen, and unload checks passed.\n";
}
