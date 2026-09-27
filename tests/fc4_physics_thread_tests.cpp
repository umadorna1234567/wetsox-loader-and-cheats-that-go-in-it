#include "physics_thread.hpp"
#include <cstdio>
#include <string>

namespace {
DWORD routerSlot, monitorSlot;
std::string calls;
int memorySystem, monitor;
bool publishMonitor=true;
void construct(void*) { calls+='C'; }
void destroy(void*) { calls+='D'; }
void memoryInit(void*,void*,const char*,unsigned flags) { calls+=flags==3?'I':'!'; }
void memoryQuit(void*,void*,unsigned flags) { calls+=flags==3?'Q':'!'; }
void threadInit(void*,void* router) {
    calls+='T'; TlsSetValue(routerSlot,router);
    if(publishMonitor)TlsSetValue(monitorSlot,&monitor);
}
void threadQuit(void*) {
    calls+='U';TlsSetValue(routerSlot,nullptr);TlsSetValue(monitorSlot,nullptr);
}
bool check(bool value,const char* message) { if(!value)std::fprintf(stderr,"%s\n",message);return value; }
}
int main() {
    routerSlot=TlsAlloc();monitorSlot=TlsAlloc();
    if(routerSlot==TLS_OUT_OF_INDEXES||monitorSlot==TLS_OUT_OF_INDEXES)return 1;
    fc4::PhysicsThreadBindings bindings{routerSlot,monitorSlot,&memorySystem,construct,destroy,memoryInit,memoryQuit,threadInit,threadQuit};
    bool ok=true;
    {
        fc4::PhysicsThreadScope scope(bindings);
        ok &= check(bool(scope)&&calls=="CIT","Missing TLS must initialize before the query");
        {fc4::PhysicsThreadScope nested(bindings);ok &= check(bool(nested),"Nested scope must reuse the thread context");}
        ok &= check(calls=="CIT","Nested scope must not clean up the owner's context");
    }
    ok &= check(calls=="CITUQD"&&!TlsGetValue(routerSlot)&&!TlsGetValue(monitorSlot),"Owned context must clean up in native order");
    calls.clear();TlsSetValue(routerSlot,&memorySystem);TlsSetValue(monitorSlot,&monitor);
    {fc4::PhysicsThreadScope scope(bindings);ok &= check(bool(scope),"Existing context should be usable");}
    ok &= check(calls.empty()&&TlsGetValue(routerSlot)==&memorySystem&&TlsGetValue(monitorSlot)==&monitor,"Existing context must remain untouched");
    TlsSetValue(monitorSlot,nullptr);
    {fc4::PhysicsThreadScope scope(bindings);ok &= check(!scope,"Partial context must reject the query");}
    ok &= check(calls.empty()&&TlsGetValue(routerSlot)==&memorySystem,"Partial context must not be replaced");
    TlsSetValue(routerSlot,nullptr);publishMonitor=false;
    {fc4::PhysicsThreadScope scope(bindings);ok &= check(!scope,"Missing monitor after initialization must reject the query");}
    ok &= check(calls=="CITUQD"&&!TlsGetValue(routerSlot),"Rejected owned context must still clean up");
    TlsFree(routerSlot);TlsFree(monitorSlot);
    return ok?0:1;
}
