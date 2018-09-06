#include "fx_manager.h"

#include "fx_defs.h"
#include "fx_rect.h"

namespace fx {

static const char* texFileName(TextureName name) {
  switch (name) {
#define CASE(id, fileName)                                                                                             \
  case TextureName::id:                                                                                                \
    return fileName;
    CASE(CIRCULAR, "circular.png")
    CASE(CIRCULAR_STRONG, "circular_strong.png")
    CASE(FLAKES_BORDERS, "flakes_4x4_borders.png")
    CASE(WATER_DROPS, "water_drops_4x4.png")
    CASE(CLOUDS_SOFT, "clouds_soft_4x4.png")
    CASE(CLOUDS_SOFT_BORDERS, "clouds_soft_borders_4x4.png")
    CASE(CLOUDS_ADD, "clouds_grayscale_4x4.png")
    CASE(TORUS, "torus.png")
    CASE(TORUS_BOTTOM, "torus_bottom.png")
    CASE(TORUS_BOTTOM_BLURRED, "torus_bottom_blurred.png")
    CASE(MAGIC_MISSILE, "magic_missile_4x4.png")
    CASE(FLAMES, "flames_4x4.png")
    CASE(FLAMES_BLURRED, "flames_blurred_4x4.png")
    CASE(SPECIAL, "special_4x2.png")
#undef CASE
  }
  return nullptr;
}

void FXManager::initializeTextureDef(TextureName name, TextureDef& def) {
  def.fileName = texFileName(name);
  if (strstr(def.fileName, "4x4"))
    def.tiles = {4, 4};
  if (strstr(def.fileName, "4x2"))
    def.tiles = {4, 2};
  if (name == TextureName::CLOUDS_ADD || name == TextureName::FLAMES || name == TextureName::FLAMES_BLURRED ||
      name == TextureName::MAGIC_MISSILE)
    def.blendMode = BlendMode::additive;
}

void FXManager::initializeTextureDefs() {
  for (auto name : ENUM_ALL(TextureName)) {
    initializeTextureDef(name, textureDefs[name]);
    textureDefs[name].validate();
  }
}

}
