#pragma once

#include "fx_base.h"
#include "fx_color.h"
#include "util.h"
#include <limits.h>

namespace fx {

struct SystemParams {
  static constexpr int maxScalars = 2, maxColors = 2, maxDirs = 2;

  // These attributes can affect system behaviour in any way
  float scalar[maxScalars] = {0.0f, 0.0f};

  // These params shouldn't affect system behaviour
  // Otherwise snapshots won't work as expected
  FVec3 color[maxColors] = {FVec3(1.0), FVec3(1.0)};
  Dir dir[maxDirs] = {Dir::N, Dir::N};
};

// Snapshots allow to start animation in the middle
// Snapshots are selected based on some of the system's parameters
struct SnapshotKey {
  SnapshotKey(float s0 = 0.0f, float s1 = 0.0f) : scalar{s0, s1} {
  }
  SnapshotKey(const SystemParams&);

  static constexpr int maxScalars = SystemParams::maxScalars;

  void apply(SystemParams&) const;
  float distanceSq(const SnapshotKey&) const;
  bool operator==(const SnapshotKey& rhs) const;

  float scalar[maxScalars] = {0.0f, 0.0f};
};

// Initial configuration of spawned particle system
struct InitConfig {
  InitConfig(FVec2 pos = {}, FVec2 targetOff = {}) : pos(pos), targetOff(targetOff) {}
  InitConfig(FVec2 pos, SnapshotKey key) : pos(pos), snapshotKey(key) {}

  FVec2 pos, targetOff;
  optional<SnapshotKey> snapshotKey;
};

// Identifies a particluar particle system instance
class ParticleSystemId {
public:
  ParticleSystemId() : index(-1) {}
  ParticleSystemId(int index, uint spawnTime) : index(index), spawnTime(spawnTime) {}

  bool validIndex() const { return index >= 0; }
  explicit operator bool() const { return validIndex(); }

  operator int() const { return index; }
  int getIndex() const { return index; }
  uint getSpawnTime() const { return spawnTime; }

private:
  int index;
  uint spawnTime;
};

struct Particle {
  float particleTime() const { return life / maxLife; }

  // TODO: quantize members ? It may give the particles a pixelated feel
  FVec2 pos, movement, size = FVec2(1.0);
  float life = 0.0f, maxLife = 1.0f;
  float rot = 0.0f, rotSpeed = 0.0f;
  SVec2 texTile;
  uint randomSeed;
};

struct DrawParticle {
  // TODO: compress it somehow?
  std::array<FVec2, 4> positions;
  std::array<FVec2, 4> texCoords;
  IColor color;
  TextureName texName;
};

struct ParticleSystem {
  struct SubSystem {
    vector<Particle> particles;
    float emissionFract = 0.0f;
    uint randomSeed = 123;
    int totalParticles = 0;
  };

  ParticleSystem(FXName, const InitConfig&, uint spawnTime, int numSubSystems);
  ParticleSystem(FXName, const InitConfig&, uint spawnTime, vector<SubSystem> snapshot);
  void randomize(RandomGen&);

  int numActiveParticles() const;
  int numTotalParticles() const;

  void kill(bool immediate);

  const SubSystem &operator[](int ssid) const { return subSystems[ssid]; }
  SubSystem &operator[](int ssid) { return subSystems[ssid]; }

  vector<SubSystem> subSystems;
  SystemParams params;
  FVec2 pos, targetOff;

  FXName defId;
  uint spawnTime;

  float animTime = 0.0f;
  bool isDead = false;
  bool isDying = false;
};

struct SubSystemContext {
  SubSystemContext(const ParticleSystem& ps, const ParticleSystemDef&, const ParticleDef&, const EmitterDef&,
                   const TextureDef&, int ssid);

  const ParticleSystem &ps;
  const ParticleSystem::SubSystem &ss;

  const ParticleSystemDef &psdef;
  const SubSystemDef& ssdef;

  const ParticleDef &pdef;
  const EmitterDef &edef;
  const TextureDef& tdef;

  const int ssid;
};

struct AnimationContext : public SubSystemContext {
  AnimationContext(const SubSystemContext &, float animTime, float timeDelta);

  float uniformSpread(float spread);
  float uniform(float min, float max);
  uint randomSeed();
  SVec2 randomTexTile();

  RandomGen rand;
  const float animTime;
  const float timeDelta, invTimeDelta;
};

struct DrawContext : public SubSystemContext {
  DrawContext(const SubSystemContext &, FVec2);

  FVec2 invTexTile;

  array<FVec2, 4> quadCorners(FVec2 pos, FVec2 size, float rotation) const;
  array<FVec2, 4> texQuadCorners(SVec2 texTile) const;
};

struct EmissionState {
  EmissionState(float time) : time(time) {}
  const float time;

  float maxLife;
  float strength, strengthSpread;
  float direction, directionSpread;
  float rotSpeed, rotSpeedSpread;

  float var[128];
};
}
