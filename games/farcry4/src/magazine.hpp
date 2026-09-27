#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>

namespace fc4 {
struct WeaponIdentity {
    std::uintptr_t player{}, entity{}, weapon{}, data{};
    std::uint64_t playerId{}, weaponId{};
    bool operator==(const WeaponIdentity&) const = default;
};
// Preserve only ammo actually observed in a live equipped firearm. No calls
// into capacity/perk routines, and no restoration through unequipped pointers.
class MagazineState {
    WeaponIdentity identity_{};
    int initial_{}, held_{};
public:
    void reset() { identity_={}; initial_=held_=0; }
    std::optional<int> update(bool requested,bool live,WeaponIdentity identity,int count) {
        if(!live||count<0||count>10000) {reset();return {};}
        if(!(identity_==identity)) {reset();}
        if(!requested) {
            const auto restore=identity_.weapon&&count==held_&&count>initial_
                ?std::optional<int>(initial_):std::nullopt;
            reset();return restore;
        }
        if(!identity_.weapon) {
            if(count==0)return {}; // Wait for a real loaded magazine.
            identity_=identity;initial_=count;held_=std::max(2,count);
        }
        held_=std::max(held_,count);
        return count<held_?std::optional<int>(held_):std::nullopt;
    }
};
}
