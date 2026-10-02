#include "FeatureHotkeys.h"
#include <cstdio>
int main(){
 bool ok=true;auto check=[&](bool value,const char* message){if(!value){std::fprintf(stderr,"%s\n",message);ok=false;}};
 using namespace wetsox;
 HotkeyState state;HotkeyRule hold{true,false,75,0},toggle{true,true,75,0};
 check(state.evaluate({},true,false,true),"Hotkey disabled leaves enabled feature on");
 check(!state.evaluate({},false,true,true),"Hotkey disabled leaves disabled feature off");
 check(!state.evaluate(hold,true,true,true),"New binding ignores a held key");
 check(!state.evaluate(hold,true,false,true),"Hold waits for press");
 check(state.evaluate(hold,true,true,true),"Hold activates on press");
 check(!state.evaluate(hold,true,false,true),"Hold deactivates on release");
 check(!state.evaluate(toggle,true,false,true),"Toggle starts off");
 check(state.evaluate(toggle,true,true,true),"Toggle activates on first press");
 check(state.evaluate(toggle,true,true,true),"Holding toggle does not repeat");
 check(state.evaluate(toggle,true,false,true),"Toggle persists on release");
 check(!state.evaluate(toggle,true,true,true),"Second press switches off");
 state.evaluate(toggle,true,false,false);
 check(!state.evaluate(toggle,true,true,true),"Returning to game with key held does not trigger");
 state.evaluate(toggle,true,false,true);
 check(state.evaluate(toggle,true,true,true),"Fresh press after returning triggers");
 check(!state.evaluate(toggle,false,true,true),"Master feature switch always disables hotkey feature");
 check(!state.evaluate(toggle,true,true,true),"Re-enabling while held requires release");
 check(state.evaluate({},true,false,true),"Disabling rule restores always-on feature");
 check(!state.evaluate({true,false,0,0},true,false,true),"Enabled but unbound feature does not activate");
 state.evaluate(hold,true,false,true);state.evaluate(hold,true,true,true);
 check(!state.evaluate(hold,true,true,false),"Losing focus releases Hold");
 check(!state.evaluate(hold,true,true,true),"Hold requires release after focus returns");
 return ok?0:1;
}
