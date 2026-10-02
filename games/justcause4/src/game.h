#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <array>
#include <mutex>
#include <vector>
#include "vector.h"

namespace mod {
struct Settings {
    bool god=false, ammo=false, boost=false, rockets=false, grappleRange=false, vehicleBoost=false;
    float speed=1.0f, wingsuitSpeed=1.0f, hoverboardSpeed=1.0f, grappleSpeed=1.0f;
    bool esp=false, espEnemiesOnly=true, espTracers=false;
    bool aim=false;
    float espRange=300, aimFov=15, aimSmooth=6;
};
struct EspCharacter {
    uintptr_t id=0;
    CVector3f feet{},top{},aimPoint{};
    float distance=0;
    int health=0;
    bool enemy=false;
};
struct Snapshot {
    bool ready=false, boostAvailable=false, rocketsAvailable=false, movementAvailable=false;
    CVector3f position{};
    int boostCount=0;
    int rocketCount=0;
    std::vector<EspCharacter> characters;
    ULONGLONG charactersTime=0;
    std::string status="Load a save to begin.";
};

extern std::mutex stateMutex;
extern Settings settings;
extern Snapshot snapshot;
extern std::array<unsigned,3> teleportKeys; // waypoint, objective, hold-to-aim
extern std::wstring dataDirectory;
uintptr_t Address(uintptr_t preferred);
bool InitializeGame();
void GameTick();
bool InGameplay();
void SetMenuFocus(bool open);
void RequestAction(int action);
void Log(const std::string& text);
void LoadHotkeys();
bool SaveHotkeys();
bool ValidPosition(const CVector3f& position);
bool ReadCamera(CMatrix4f& matrix);
}
