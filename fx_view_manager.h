#pragma once

#include "unique_entity.h"
#include "util.h"
#include "fx_interface.h"
#include "fx_name.h"
#include "color.h"
#include <unordered_map>

RICH_ENUM(FXStackId, debuff, buff, generic);

struct FXDef {
  FXName name = FXName::DUMMY;
  Color color = Color::WHITE;
  float strength = 0.0f;
  FXStackId stackId = FXStackId::generic;
};

FXDef getDef(FXVariantName);

class FXViewManager {
  public:
  void beginFrame();
  void addEntity(GenericId, float x, float y);

  // Entity identified with given id must be present!
  void addFX(GenericId, const FXDef&);
  void addFX(GenericId, FXVariantName);
  void finishFrame();

  private:
  using TypeId = variant<FXName, FXVariantName>;

  struct EffectInfo {
    string name() const;

    FXId id;
    TypeId typeId;
    FXStackId stackId;
    bool isVisible;
    bool isDying;
  };

  struct EntityInfo {
    static constexpr int maxEffects = 8;

    void clearVisibility();
    void addFX(GenericId, TypeId, const FXDef&);
    void updateFX(GenericId);
    void updateParams(const FXDef&, FXId);

    float x, y;
    EffectInfo effects[maxEffects];
    int numEffects = 0;
    bool isVisible = true;
    bool justShown = true;
  };

  std::unordered_map<GenericId, EntityInfo> entities;
};
