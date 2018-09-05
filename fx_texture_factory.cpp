#include "fx_manager.h"

#include "fx_defs.h"
#include "fx_rect.h"

namespace fx {

void FXManager::initializeTextureDef(TextureName name, TextureDef& def) {
  switch (name) {
  case TextureName::CIRCULAR:
    def.fileName = "circular.png";
    break;

  case TextureName::CIRCULAR_STRONG:
    def.fileName = "circular_strong.png";
    break;

  case TextureName::FLAKES_BORDERS:
    def.fileName = "flakes_4x4_borders.png";
    def.tiles = {4, 4};
    break;

  case TextureName::CLOUDS_SOFT:
    def.fileName = "clouds_soft_4x4.png";
    def.tiles = {4, 4};
    break;
  case TextureName::CLOUDS_SOFT_BORDERS:
    def.fileName = "clouds_soft_borders_4x4.png";
    def.tiles = {4, 4};
    break;

  case TextureName::CLOUDS_ADD:
    def.fileName = "clouds_grayscale_4x4.png";
    def.tiles = {4, 4};
    def.blendMode = BlendMode::additive;
    break;

  case TextureName::TORUS:
    def.fileName = "torus.png";
    break;

  case TextureName::TORUS_BOTTOM:
    def.fileName = "torus_bottom.png";
    break;

  case TextureName::TORUS_BOTTOM_BLURRED:
    def.fileName = "torus_bottom_blurred.png";
    break;

  case TextureName::MAGIC_MISSILE:
    def.fileName = "magic_missile_4x4.png";
    def.tiles = {4, 4};
    def.blendMode = BlendMode::additive;
    break;

  case TextureName::FLAMES:
    def.fileName = "flames_4x4.png";
    def.tiles = {4, 4};
    def.blendMode = BlendMode::additive;
    break;

  case TextureName::FLAMES_BLURRED:
    def.fileName = "flames_blurred_4x4.png";
    def.tiles = {4, 4};
    def.blendMode = BlendMode::additive;
    break;

  case TextureName::SPECIAL:
    def.fileName = "special_4x2.png";
    def.tiles = {4, 2};
    break;
  }
}

void FXManager::initializeTextureDefs() {
  for (auto name : ENUM_ALL(TextureName)) {
    initializeTextureDef(name, textureDefs[name]);
    textureDefs[name].validate();
  }
}

}
