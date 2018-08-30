#pragma once

#include "unique_entity.h"
#include "util.h"
#include "fx_simple.h"
#include "color.h"
#include <unordered_map>

struct FXDef {
  FXName name;
  Color color = Color::WHITE;
  float scalar0 = 0.0f;
  float scalar1 = 0.0f;
};

class FXViewManager {
  public:
  // TODO: proper identifiers for objects
  using GenericId = long long;
  using UniqueId = UniqueEntity<Creature>::Id;

  void beginFrame();
  void addEntity(UniqueId, float x, float y);

  // Entity identified with given id must be present!
  void addFX(UniqueId, const FXDef&);
  void finishFrame();

  private:
  struct EffectInfo {
    FXId id;
    FXName name;
    bool isVisible;
    bool isDying;
  };

  struct EntityInfo {
    static constexpr int maxEffects = 4;

    void clearVisibility();
    void addFX(GenericId, const FXDef&);
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
