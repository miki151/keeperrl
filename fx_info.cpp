#include "stdafx.h"
#include "fx_info.h"
#include "fx_name.h"
#include "color.h"

#include "workshop_type.h"
#include "fx_variant_name.h"
#include "view_id.h"

FXInfo getFXInfo(FXVariantName var) {
  using Name = FXVariantName;
  switch (var) {
    case Name::BLIND:
      return {FXName::BLIND};
    case Name::SLEEP:
      return {FXName::SLEEP};
    case Name::FLYING:
      return {FXName::FLYING};
    case Name::FIRE_SPHERE:
      return {FXName::FIRE_SPHERE};
    case Name::FIRE:
      return {FXName::FIRE};
    case Name::SPIRAL_BLUE:
      return {FXName::SPIRAL, Color::BLUE};
    case Name::SPIRAL_GREEN:
      return {FXName::SPIRAL, Color::f(0.7, 1.0, 0.6)};
    case Name::LABORATORY:
      return {FXName::LABORATORY, Color::GREEN};
    case Name::PEACEFULNESS:
      return {FXName::LOVE_LOOPED, Color::RED};
    case Name::FORGE:
      return {FXName::FORGE, Color(252, 142, 30)};
    case Name::WORKSHOP:
      return {FXName::WORKSHOP};
    case Name::JEWELLER:
      return {FXName::JEWELLER, Color(253, 247, 140)};

    case Name::BUFF_RED:     return {FXName::BUFF,   Color(250, 40, 40), 0.0f, FXStackId::buff};
    case Name::BUFF_WHITE:   return {FXName::BUFF,   Color(255, 255, 255), 0.0f, FXStackId::buff};
    case Name::BUFF_YELLOW:  return {FXName::BUFF,   Color(255, 255, 100), 0.0f, FXStackId::buff};
    case Name::BUFF_BLUE:    return {FXName::BUFF,   Color(70, 130, 225), 0.0f, FXStackId::buff};
    case Name::BUFF_PINK:    return {FXName::BUFF,   Color(255, 100, 255), 0.0f, FXStackId::buff};
    case Name::BUFF_ORANGE:  return {FXName::BUFF,   Color::ORANGE, 0.0f, FXStackId::buff};
    case Name::BUFF_GREEN1:  return {FXName::BUFF,   Color(0, 160, 30), 0.0f, FXStackId::buff};
    case Name::BUFF_GREEN2:  return {FXName::BUFF,   Color(80, 255, 120), 0.0f, FXStackId::buff};
    case Name::BUFF_SKY_BLUE:return {FXName::BUFF,   Color::SKY_BLUE, 0.0f, FXStackId::buff};
    case Name::BUFF_BROWN:   return {FXName::BUFF,   Color::BROWN, 0.0f, FXStackId::buff};
    case Name::BUFF_PURPLE:  return {FXName::BUFF,   Color::PURPLE, 0.0f, FXStackId::buff};

    case Name::DEBUFF_RED:   return {FXName::DEBUFF, Color(190, 30, 30), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BLUE:  return {FXName::DEBUFF, Color(30, 60, 230), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GREEN1:return {FXName::DEBUFF, Color(0, 160, 30), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GREEN2:return {FXName::DEBUFF, Color(80, 255, 120), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_PINK:  return {FXName::DEBUFF, Color(160, 10, 180), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_ORANGE:return {FXName::DEBUFF, Color::ORANGE, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BROWN: return {FXName::DEBUFF, Color::BROWN, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BLACK: return {FXName::DEBUFF, Color::BLACK, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_WHITE: return {FXName::DEBUFF, Color::WHITE, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_YELLOW: return {FXName::DEBUFF, Color::YELLOW, 0.0f, FXStackId::debuff};
  }
}

optional<FXInfo> getOverlayFXInfo(ViewId id) {
  if (isOneOf(id, ViewId("gold_ore"), ViewId("gold"), ViewId("throne"), ViewId("minion_statue"), ViewId("demon_shrine")))
    return FXInfo{FXName::GLITTERING, Color(253, 247, 172)};
  if (id == ViewId("adamantium_ore"))
    return FXInfo{FXName::GLITTERING, Color::LIGHT_BLUE};
  if (id == ViewId("magma"))
    return FXInfo{FXName::MAGMA_FIRE};
  return none;
}

FXInfo::FXInfo(FXName name, Color color, float strength, FXStackId sid)
  : name(name), color(color), strength(strength), stackId(sid) { }

bool FXInfo::empty() const {
  return name == FXName(0);
}

FXInfo& FXInfo::setReversed() {
  reversed = true;
  return *this;
}

FXSpawnInfo::FXSpawnInfo(FXInfo info, Vec2 pos, Vec2 offset)
  : position(pos), targetOffset(offset), info(info) { }
