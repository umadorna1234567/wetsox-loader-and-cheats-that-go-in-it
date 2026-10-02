#define NOMINMAX
#include "fc5/build_compatibility.hpp"
#include <cstdio>
int main() {
    bool ok=true;auto check=[&](bool value,const char* message){if(!value){std::fprintf(stderr,"%s\n",message);ok=false;}};
    const auto file=std::filesystem::temp_directory_path()/(L"wetsox-build-check-"+std::to_wstring(GetCurrentProcessId())+L".bin");
    {std::ofstream out(file,std::ios::binary);out<<"abc";}
    constexpr auto abc="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    using namespace wetsox::compatibility;
    auto result=checkFile(file,abc);
    check(result.supported()&&result.sha256==abc,"Known SHA-256 vector matches");
    result=checkFile(file,farCry4.sha256);
    check(result.status==Status::mismatch&&!result.supported(),"Unverified file is not mislabeled as verified");
    const auto message=describe(farCry4,result);
    check(message.find(abc)!=std::string::npos&&message.find(farCry4.sha256)!=std::string::npos,"Mismatch includes actual and supported fingerprints");
    check(result.canAttempt(),"A different engine hash is allowed to attempt startup");
    check(message.find("startup is allowed")!=std::string::npos,"Mismatch is informational");
    {std::ofstream out(file,std::ios::binary);}
    result=checkFile(file,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    check(result.supported(),"Empty-file SHA-256 vector matches");
    std::filesystem::remove(file);
    result=checkFile(file,farCry4.sha256);
    check(result.status==Status::unreadable,"Missing engine differs from unsupported build");
    check(describe(farCry4,result).find("cannot read")!=std::string::npos,"Read failure has actionable distinct error");
    check(!Result{}.supported()&&!Result{}.canAttempt(),"Unreadable file still fails closed");
    check(!Result{Status::hashFailure,{}}.canAttempt(),"Hash API failure differs from a mismatching hash");
    return ok?0:1;
}
