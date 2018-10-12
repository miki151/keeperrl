#pragma once

#include "unique_entity.h"
#include "util.h"
#include "fx_info.h"
#include "color.h"
#include <unordered_map>

namespace fx {
  class FXManager;
  class FXRenderer;
  class ParticleSystemId;
}
class Renderer;

// TODO: better name
class FXViewManager {
  public:
  using FXId = fx::ParticleSystemId;

  FXViewManager(fx::FXManager*, fx::FXRenderer*);
  ~FXViewManager();

  FXViewManager(const FXViewManager&) = delete;
  void operator=(const FXViewManager&) = delete;

  void beginFrame(Renderer&, float zoom, float ox, float oy);
  void finishFrame();

  void addEntity(GenericId, float x, float y);
  // Entity identified with given id must be present!
  void addFX(GenericId, const FXInfo&);
  void addFX(GenericId, FXVariantName);

  // Unmanaged & unordered FXes are the same
  void addUnmanagedFX(const FXSpawnInfo&);

  void drawFX(Renderer&, GenericId);
  void drawUnorderedBackFX(Renderer&);
  void drawUnorderedFrontFX(Renderer&);

  private:
  using TypeId = variant<FXName, FXVariantName>;
  void updateParams(const FXInfo&, FXId);
  FXId spawnOrderedEffect(const FXInfo&, bool snapshot);
  FXId spawnUnorderedEffect(FXName, float x, float y, Vec2 dir);

  struct EffectInfo;
  struct EntityInfo;
  using EntityMap = std::unordered_map<GenericId, EntityInfo>;

  std::unique_ptr<EntityMap> entities;
  fx::FXManager* fxManager = nullptr;
  fx::FXRenderer* fxRenderer = nullptr;
  float m_zoom = 1.0f;
};
