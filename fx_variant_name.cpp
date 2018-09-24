#include "stdafx.h"
#include "fx_variant_name.h"
#include "fx_view_manager.h"
#include "fx_name.h"
#include "color.h"

FXDef getDef(FXVariantName var) {
  using Name = FXVariantName;
  switch (var) {
    case Name::PEACEFULNESS:
      return {FXName::PEACEFULNESS};
    case Name::BLIND:
      return {FXName::BLIND};
    case Name::SPEED:
      return {FXName::SPEED};
    case Name::SLEEP:
      return {FXName::SLEEP};
    case Name::FLYING:
      return {FXName::FLYING};
    case Name::FIRE_SPHERE:
      return {FXName::FIRE_SPHERE};
    case Name::DEBUFF_RED:
      return {FXName::DEBUFF, Color::RED};
    case Name::DEBUFF_GREEN:
      return {FXName::DEBUFF, Color::GREEN};
    case Name::DEBUFF_BLACK:
      return {FXName::DEBUFF, Color::BLACK};
    case Name::DEBUFF_WHITE:
      return {FXName::DEBUFF, Color::WHITE};
    case Name::DEBUFF_GRAY:
      return {FXName::DEBUFF, Color::GRAY};
    case Name::DEBUFF_PINK:
      return {FXName::DEBUFF, Color::PINK};
    case Name::DEBUFF_ORANGE:
      return {FXName::DEBUFF, Color::ORANGE};
    case Name::SPIRAL_BLUE:
      return {FXName::SPIRAL, Color::BLUE};
    case Name::SPIRAL_GREEN:
      return {FXName::SPIRAL, Color::f(0.7, 1.0, 0.6)};
  }
}
