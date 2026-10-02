#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <algorithm>
namespace kf2 {
using Ptr=std::uintptr_t;
inline bool copyReadable(Ptr p,void* output,size_t bytes){
 if(!p)return false;
 __try {std::memcpy(output,reinterpret_cast<const void*>(p),bytes);return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T>T read(Ptr p){T value{};return copyReadable(p,&value,sizeof(value))?value:T{};}
inline bool write(Ptr p,const void* data,size_t bytes){SIZE_T n{};return p&&WriteProcessMemory(GetCurrentProcess(),(void*)p,data,bytes,&n)&&n==bytes;}
struct Vec {float x{},y{},z{};};
struct Rot {int pitch{},yaw{},roll{};};
// Core.Object.EAxis is a bit-valued enum, not a zero-based vector index.
enum class BoneAxis : unsigned char { X=1,Y=2,Z=4 };
struct Name {int index{},number{};};
struct Array {Ptr data{};int count{},capacity{};};
using Event=void(*)(Ptr,Ptr,void*,void*);
inline Ptr objects{},names{};
inline Event processEvent{};
inline std::unordered_map<int,std::string> nameCache;
inline std::string name(int index){
 if(index<0||index>=read<int>(names+8))return {};
 if(auto it=nameCache.find(index);it!=nameCache.end())return it->second;
 auto entry=read<Ptr>(read<Ptr>(names)+8ull*index);char text[256]{};SIZE_T n{};
 if(!entry||!ReadProcessMemory(GetCurrentProcess(),(void*)(entry+20),text,sizeof(text)-1,&n))return {};
 return nameCache[index]=std::string(text,strnlen(text,sizeof(text)-1));
}
inline Name findName(std::string_view value){
 const auto count=read<int>(names+8);if(count<1||count>2000000)return {};
 for(int i=0;i<count;++i)if(name(i)==value)return {i,0};return {};
}
inline std::string objectName(Ptr object){return object?name(read<Name>(object+0x48).index):std::string{};}
inline Ptr objectClass(Ptr object){return read<Ptr>(object+0x50);}
inline bool isA(Ptr object,std::string_view cls){
 for(auto type=objectClass(object),n=Ptr{};type&&n<64;type=read<Ptr>(type+0x78),++n)if(objectName(type)==cls)return true;
 return false;
}
struct Field {Ptr object{};int offset{},size{},count{};unsigned mask{};std::string type;explicit operator bool()const{return object!=0;}};
inline std::unordered_map<std::string,Field> fields;
inline Field field(Ptr type,std::string_view key){
 const auto cacheKey=std::to_string(type)+":"+std::string(key);
 if(auto it=fields.find(cacheKey);it!=fields.end())return it->second;
 for(unsigned depth=0;type&&depth<64;++depth,type=read<Ptr>(type+0x78)) {
  auto f=read<Ptr>(type+0x80);
  for(unsigned count=0;f&&count<4096;++count,f=read<Ptr>(f+0x60))if(objectName(f)==key){
   Field result{f,read<int>(f+0x8c),read<int>(f+0x6c),read<int>(f+0x68),read<unsigned>(f+0xa8),objectName(objectClass(f))};
   if(result.type=="Function"||(result.offset>=0&&result.offset<0x100000&&result.size>0&&result.size<0x100000))return fields[cacheKey]=result;
   return {};
  }
 }
 return fields[cacheKey]={};
}
template<class T>T get(Ptr object,std::string_view key){auto f=field(objectClass(object),key);return f&&f.size>=int(sizeof(T))?read<T>(object+f.offset):T{};}
inline bool flag(Ptr object,std::string_view key){auto f=field(objectClass(object),key);return f&&(read<unsigned>(object+f.offset)&f.mask)!=0;}
inline Ptr function(Ptr object,std::string_view key){auto f=field(objectClass(object),key);return f.type=="Function"?f.object:0;}
struct Call {
 Ptr fn{};std::vector<unsigned char> bytes;
 Call(Ptr object,std::string_view key):fn(function(object,key)){
  const int size=read<int>(fn+0x88);if(!fn||size<0||size>16384){fn=0;return;}bytes.resize(std::max(4,size));
 }
 template<class T>bool set(std::string_view key,T value){auto f=field(fn,key);if(!f||f.offset+sizeof(T)>bytes.size())return false;std::memcpy(bytes.data()+f.offset,&value,sizeof(T));return true;}
 template<class T>T result(std::string_view key="ReturnValue"){auto f=field(fn,key);T value{};if(f&&f.offset+sizeof(T)<=bytes.size())std::memcpy(&value,bytes.data()+f.offset,sizeof(T));return value;}
 bool run(Ptr object){
  if(!fn||!processEvent||!object)return false;
  // KF2 ProcessEvent exits early for nonzero UFunction.iNative. Temporarily
  // dispatch numbered natives via their Func entry, then restore the index.
  // All Call users execute synchronously on the game's script/HUD thread.
  const auto index=read<unsigned short>(fn+0xd4);
  if(index&&!(read<unsigned>(fn+0xd0)&0x400u))return false;
  struct RestoreIndex {Ptr address;unsigned short value;~RestoreIndex(){if(value)write(address,&value,sizeof(value));}} restore{fn+0xd4,index};
  if(index){const unsigned short zero=0;if(!write(fn+0xd4,&zero,sizeof(zero)))return false;}
  processEvent(object,fn,bytes.data(),nullptr);return true;
 }
};
inline Ptr signature(Ptr base,const unsigned char* pattern,const char* mask){
 const auto dos=read<IMAGE_DOS_HEADER>(base);if(dos.e_magic!=IMAGE_DOS_SIGNATURE)return 0;
 const auto nt=read<IMAGE_NT_HEADERS64>(base+dos.e_lfanew);if(nt.Signature!=IMAGE_NT_SIGNATURE)return 0;
 const Ptr table=base+dos.e_lfanew+24+nt.FileHeader.SizeOfOptionalHeader;Ptr found=0;size_t length=std::strlen(mask);
 for(unsigned j=0;j<nt.FileHeader.NumberOfSections;++j){auto section=read<IMAGE_SECTION_HEADER>(table+j*sizeof(IMAGE_SECTION_HEADER));if(!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)||section.Misc.VirtualSize<length)continue;
  auto data=(const unsigned char*)(base+section.VirtualAddress);
  for(size_t i=0;i+length<=section.Misc.VirtualSize;++i){bool match=true;for(size_t k=0;k<length;++k)if(mask[k]=='x'&&data[i+k]!=pattern[k]){match=false;break;}if(match){if(found)return 0;found=(Ptr)(data+i);}}
 }return found;
}
inline bool scriptExecutorAbi(const unsigned char* code,size_t size){
 // The leading REX prefix belongs to push rbx and is present in KF2's x64 build.
 constexpr unsigned char prologue[]{0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x48,0x81,0xec,0x90,0,0,0};
 constexpr unsigned char nodeLoad[]{0x48,0x8b,0x52,0x14};
 return code&&size>=0x36&&!std::memcmp(code,prologue,sizeof(prologue))&&!std::memcmp(code+0x32,nodeLoad,sizeof(nodeLoad));
}
inline Ptr initializeReflection(){
 const auto base=(Ptr)GetModuleHandleW(L"KFGame.exe");if(!base)return 0;
 const auto o=signature(base,(const unsigned char*)"\x3B\x00\x00\x00\x00\x00\x7D\x00\x48\x8B\xC8\x48\x8B\x00\x00\x00\x00\x00\x48\x8B\x0C\xC8\xE8\x00\x00\x00\x00\x48\x8B\xC8","x?????x?xxxxx?????xxxxx????xxx");
 const auto n=signature(base,(const unsigned char*)"\x48\x8B\x0D\x00\x00\x00\x00\x48\x83\x3C\xF9","xxx????xxxx");
 const auto e=signature(base,(const unsigned char*)"\x40\x55\x41\x56\x41\x57\x48\x81\xEC\xB0\x00\x00\x00\x48\x8D\x6C\x24\x20","xxxxxxxxxxxxxxxxxx");
 if(!o||!n||!e)return 0;objects=o+6+read<int>(o+2)-8;names=n+7+read<int>(n+3);
 if(read<int>(objects+8)<1000||read<int>(objects+8)>2000000||name(0)!="None"||name(1)!="ByteProperty")return 0;
 return e;
}
}
