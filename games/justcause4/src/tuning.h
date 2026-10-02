#pragma once
#include <cmath>

namespace mod {
// Active boost charges also drive burst duration. Refill only between bursts
// so the native release/completion path can finish using the current charge.
inline void RefillIdleBoost(float* charges,int count,bool enabled,bool active) {
    if(!enabled || active || count<1 || count>3) return;
    for(int i=0;i<count;i++)
        if(std::isfinite(charges[i]) && charges[i]>=0 && charges[i]<=1.01f) charges[i]=1.f;
}
// Absolute override with the same ownership rule as the speed multipliers.
struct ValueOverride {
    float baseline=0, last=0;
    bool owned=false;
    bool Apply(float& current,bool enabled,float target) {
        if(!std::isfinite(current) || !std::isfinite(target)) return false;
        if(!enabled) {
            if(owned && current==last) current=baseline;
            owned=false;
            return true;
        }
        if(!owned || current!=last) baseline=current;
        current=target;last=target;owned=true;
        return true;
    }
};
// A multiplier owns only the value it last wrote. Engine updates become the new
// baseline; disabling never overwrites a value changed by another system.
struct ScalarOverride {
    float baseline=0, last=0;
    bool owned=false;
    bool Apply(float& current,float factor) {
        if(!std::isfinite(current) || !std::isfinite(factor) || factor<=0) return false;
        if(factor==1) {
            if(owned && current==last) current=baseline;
            owned=false;
            return true;
        }
        if(!owned || current!=last) baseline=current;
        float next=baseline*factor;
        if(!std::isfinite(next)) return false;
        current=next; last=next; owned=true;
        return true;
    }
};
}
