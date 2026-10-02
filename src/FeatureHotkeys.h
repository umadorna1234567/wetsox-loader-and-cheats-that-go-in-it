#pragma once
#include <array>
#include <string_view>

namespace wetsox {
inline constexpr std::array<std::string_view,51> featureKeys{
 "aim","showFov","aimEnemies","aimHumans","aimAnimals","sticky","travel","prediction","drop",
 "esp","allHumans","espEnemies","espAnimals","boneEsp","boxEsp","espOutline","ammo","reload",
 "playerFov","vehicleFov","god","vehicleBoost","boost","rockets","grappleRange","enemiesOnly","tracers","zedEsp","zedSkeleton","zedOutline","zedHealth","zedGradient","zedSnaplines","playerEsp","playerSkeleton","playerOutline","playerHealth","playerGradient","playerSnaplines","silent","visibility","lootEsp","syringe","carry","rapid","spread","recoil","sway","wallShots","moveSpeed","noclip"};
struct HotkeyRule {
 bool enabled{},toggle{};
 unsigned key{},modifiers{};
 bool operator==(const HotkeyRule&) const = default;
};
struct HotkeyState {
 HotkeyRule rule{};
 bool initialized{},wasDown{},armed{},latched{},base{};
 bool evaluate(HotkeyRule next,bool normal,bool down,bool active) {
  if(!initialized||rule!=next||base!=normal) {
   rule=next;initialized=true;base=normal;latched=false;wasDown=down;armed=!down;
  }
  if(!rule.enabled) {wasDown=down;armed=!down;return normal;}
  if(!active) {wasDown=down;armed=false;return normal&&rule.toggle&&latched;}
  if(!normal){latched=false;wasDown=down;armed=!down;return false;}
  if(!rule.key)return false;
  if(!down)armed=true;
  if(rule.toggle&&armed&&down&&!wasDown)latched=!latched;
  wasDown=down;
  return rule.toggle?latched:(armed&&down);
 }
};
}
