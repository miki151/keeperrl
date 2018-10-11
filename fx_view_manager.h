#pragma once

#include "unique_entity.h"
#include "util.h"
#include "fx_info.h"
#include "color.h"
#include <unordered_map>

namespace fx {
  class FXManager;
  class FXRenderer;
}
class Renderer;

// TODO: fx_interface is not really needed, all fx spawning goes through FXViewManager
class FXViewManager {
  public:
  FXViewManager(fx::FXManager*, fx::FXRenderer*);
  ~FXViewManager();

  void beginFrame(Renderer&, float zoom, float ox, float oy);
  void finishFrame();

  void addEntity(GenericId, float x, float y);
  // Entity identified with given id must be present!
  void addFX(GenericId, const FXInfo&);
  void addFX(GenericId, FXVariantName);

  void addUnmanagedFX(const FXSpawnInfo&);

  void drawFX(Renderer&, GenericId);
  void drawUnorderedBackFX(Renderer&);
  void drawUnorderedFrontFX(Renderer&);

  private:
  using TypeId = variant<FXName, FXVariantName>;
  static void updateParams(fx::FXManager*, const FXInfo&, pair<int, int>);

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
    void addFX(fx::FXManager*, GenericId, TypeId, const FXInfo&);
    void updateFX(fx::FXManager*, GenericId);

    float x, y;
    EffectInfo effects[maxEffects];
    int numEffects = 0;
    bool isVisible = true;
    bool justShown = true;
  };

  std::unordered_map<GenericId, EntityInfo> entities;
  fx::FXManager* fxManager = nullptr;
  fx::FXRenderer* fxRenderer = nullptr;
  float m_zoom = 1.0f;
};
