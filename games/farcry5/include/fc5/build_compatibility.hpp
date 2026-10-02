#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace wetsox::compatibility {
struct EngineBuild { const wchar_t* module; const char* game; const char* sha256; };
inline constexpr EngineBuild farCry4{L"FC64.dll", "Far Cry 4", "7304078fb5bfec149c93804f842295fa8a6f9913e6b280523c74389a4272803a"};
inline constexpr EngineBuild farCry5{L"FC_m64.dll", "Far Cry 5", "00833fae4d5d70213158a146ca98439b28a8e962b934885ecccd906204880af2"};
enum class Status { matched, mismatch, unreadable, hashFailure };
struct Result {
    Status status = Status::unreadable;
    std::string sha256;
    bool supported() const { return status == Status::matched; }
    bool canAttempt() const { return status == Status::matched || status == Status::mismatch; }
};
inline Result checkFile(const std::filesystem::path& path, std::string_view expected) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    struct Hash {
        BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE value{};
        ~Hash() { if(value) BCryptDestroyHash(value); if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0); }
    } hash;
    if (BCryptOpenAlgorithmProvider(&hash.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0 ||
        BCryptCreateHash(hash.algorithm,&hash.value,nullptr,0,nullptr,0,0)<0) return {Status::hashFailure,{}};
    std::array<char,65536> buffer{};
    while(input) {
        input.read(buffer.data(),buffer.size());
        if(input.gcount() && BCryptHashData(hash.value,reinterpret_cast<PUCHAR>(buffer.data()),static_cast<ULONG>(input.gcount()),0)<0)
            return {Status::hashFailure,{}};
    }
    if(input.bad() || (!input.eof() && input.fail())) return {};
    std::array<unsigned char,32> bytes{};
    if(BCryptFinishHash(hash.value,bytes.data(),static_cast<ULONG>(bytes.size()),0)<0) return {Status::hashFailure,{}};
    std::string digest;digest.reserve(64);
    constexpr char hex[]="0123456789abcdef";
    for(auto byte:bytes){digest+=hex[byte>>4];digest+=hex[byte&15];}
    return {digest==expected?Status::matched:Status::mismatch,digest};
}
inline std::string describe(const EngineBuild& build,const Result& result) {
    const std::string prefix=std::string(build.game)+": ";
    if(result.status==Status::unreadable)return prefix+"cannot read the loaded engine file to verify its build. Check file access, then retry.";
    if(result.status==Status::hashFailure)return prefix+"Windows could not calculate the engine fingerprint. No features enabled.";
    if(result.supported())return prefix+"engine fingerprint matches the supported build. SHA-256: "+result.sha256;
    return prefix+"unverified engine build; startup is allowed but compatibility is not verified. Detected SHA-256: "+result.sha256+
        ". Reference SHA-256: "+build.sha256+". Share a diagnostic report if features fail.";
}
}
