#pragma once
#include <array>
namespace wetsox::kf2 {
struct Color {unsigned char r{},g{},b{},a{255};};
struct Esp {
 bool enabled{},skeleton{true},outline{},health{true},gradient{true},snaplines{};
 int origin{2};
 int healthPosition{2};
 Color skeletonColor{255,89,94},outlineColor{255,89,94},healthColor{90,230,146},low{255,89,94},high{90,230,146},line{255,89,94};
};
struct Settings {
 Esp zed;
 Esp player{false,true,false,true,true,false,2,2,{102,194,255},{102,194,255},{90,230,146},{255,89,94},{90,230,146},{102,194,255}};
 bool aim{},silent{},showFov{},visibility{true},lootEsp{},god{},syringe{},carry{},rapid{},spread{},recoil{},sway{},wallShots{},reload{},ammo{},moveSpeed{},noclip{},playerFov{};
 float smooth{.12f},fov{30},fireRate{3},moveMultiplier{1},noclipMultiplier{1},playerDegrees{90};
 int priority{},doshAmount{1000};
 Color lootColor{255,209,102},fovColor{174,120,255};
 unsigned actionSequence{};
};
}
