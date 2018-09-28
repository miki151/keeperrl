#pragma once

#include "unique_entity.h"
#include "util.h"
#include "fx_info.h"
#include "color.h"
#include <unordered_map>

class Renderer;

// TODO: fx_interface is not really needed, all fx spawning goes through FXViewManager
class FXViewManager {
  public:
  void beginFrame();
  void addEntity(GenericId, float x, float y);
  // Entity identified with given id must be present!
  void addFX(GenericId, const FXInfo&);
  void addFX(GenericId, FXVariantName);
  void finishFrame();

  void drawFX(Renderer&, GenericId);

  void addUnmanagedFX(const FXSpawnInfo&);

  private:
  using TypeId = variant<FXName, FXVariantName>;
  static void updateParams(const FXInfo&, pair<int, int>);

  struct EffectInfo {
    string name() const;

    pair<int, int> id;
    TypeId typeId;
    FXStackId stackId;
    bool isVisible;
    bool isDying;
  };

  struct EntityInfo {
    static constexpr int maxEffects = 8;

    void clearVisibility();
    void addFX(GenericId, TypeId, const FXInfo&);
    void updateFX(GenericId);

    float x, y;
    EffectInfo effects[maxEffects];
    int numEffects = 0;
    bool isVisible = true;
    bool justShown = true;
  };

  std::unordered_map<GenericId, EntityInfo> entities;
};
