#pragma once

#include "fx_base.h"
#include "fx_color.h"
#include "fx_tag_id.h"
#include "util.h"
#include <limits.h>

namespace fx {

// Identifies a particluar particle system instance
class ParticleSystemId {
public:
  ParticleSystemId() : m_index(-1) {}
  ParticleSystemId(int index, int spawn_time) : m_index(index), m_spawn_time(spawn_time) {}

  bool validIndex() const { return m_index >= 0; }
  explicit operator bool() const { return validIndex(); }

  operator int() const { return m_index; }
  int index() const { return m_index; }
  int spawnTime() const { return m_spawn_time; }

private:
  int m_index;
  int m_spawn_time;
};

using AnimateParticleFunc = void (*)(AnimationContext &, Particle &);
using DrawParticleFunc = void (*)(DrawContext &, const Particle &, DrawParticle &);

// Returns number of particles to emit
// Fractionals will be accumulated over time
using PrepareEmissionFunc = float (*)(AnimationContext &, EmissionState &);
using EmitParticleFunc = void (*)(AnimationContext &, EmissionState &, Particle &);

void defaultAnimateParticle(AnimationContext &, Particle &);
float defaultPrepareEmission(AnimationContext &, EmissionState &);
void defaultEmitParticle(AnimationContext &, EmissionState &, Particle &);
void defaultDrawParticle(DrawContext &, const Particle &, DrawParticle &);

// Defines behaviour of a whole particle system
struct ParticleSystemDef {
  // TODO: is it really useful to separate particle, emitter & system definitions?
  // maybe let's just merge it into single structure? We're not reusing anything anyways...

  struct SubSystem {
    SubSystem(ParticleDefId pdef, EmitterDefId edef, float estart, float eend)
        : particle_id(pdef), emitter_id(edef), emission_start(estart), emission_end(eend) {}

    ParticleDefId particle_id;
    EmitterDefId emitter_id;

    float emission_start, emission_end;

    AnimateParticleFunc animate_func = defaultAnimateParticle;
    PrepareEmissionFunc prepare_func = defaultPrepareEmission;
    EmitParticleFunc emit_func = defaultEmitParticle;
    DrawParticleFunc draw_func = defaultDrawParticle;

    int max_active_particles = INT_MAX;
    int max_total_particles = INT_MAX; // TODO: how should we treat it in looped animations?
  };

  void serialize(IArchive &, unsigned int);
  void serialize(OArchive &, unsigned int) const;

  const SubSystem &operator[](int ssid) const { return subsystems[ssid]; }
  SubSystem &operator[](int ssid) { return subsystems[ssid]; }

  vector<SubSystem> subsystems;
  optional<float> anim_length;
  bool is_looped = false;
  string name;
};

struct Particle {
  float particleTime() const { return life / max_life; }

  // TODO: quantize members ? It may give the particles a pixelated feel
  FVec2 pos, movement, size = FVec2(1.0);
  float life = 0.0f, max_life = 1.0f;
  float rot = 0.0f, rot_speed = 0.0f;
  SVec2 tex_tile;
};

struct DrawParticle {
  // TODO: compress it somehow?
  std::array<FVec2, 4> positions;
  std::array<FVec2, 4> tex_coords;
  IColor color;
  int particle_def_id;
};

struct ParticleSystem {
  struct SubSystem {
    vector<Particle> particles;
    float emission_fract = 0.0f;
    int random_seed = 123;
    int total_particles = 0;
  };

  struct Params {
    static constexpr int max_scalars = 4, max_colors = 2, max_dirs = 2;

    float scalar[max_scalars] = {0.0f, 0.0f, 0.0f, 0.0f};
    FVec3 color[max_colors] = {FVec3(1.0), FVec3(1.0)};
    Dir dir[max_dirs] = {Dir::N, Dir::N};
  };

  ParticleSystem(FVec2 pos, ParticleSystemDefId, int spawn_time, int num_subsystems);

  int numActiveParticles() const;
  int numTotalParticles() const;

  void kill();

  const SubSystem &operator[](int ssid) const { return subsystems[ssid]; }
  SubSystem &operator[](int ssid) { return subsystems[ssid]; }

  vector<SubSystem> subsystems;
  Params params;
  FVec2 pos;

  ParticleSystemDefId def_id;
  int spawn_time;

  float anim_time = 0.0f;
  bool is_dead = false;
};

struct SubSystemContext {
  SubSystemContext(const ParticleSystem &ps, const ParticleSystemDef &, const ParticleDef &, const EmitterDef &,
                   int ssid);

  const ParticleSystem &ps;
  const ParticleSystem::SubSystem &ss;

  const ParticleSystemDef &psdef;
  const ParticleSystemDef::SubSystem &ssdef;

  const ParticleDef &pdef;
  const EmitterDef &edef;

  const int ssid;
};

struct AnimationContext : public SubSystemContext {
  AnimationContext(const SubSystemContext &, float anim_time, float time_delta);

  float uniformSpread(float spread);
  float uniform(float min, float max);
  int randomSeed();
  SVec2 randomTexTile();

  RandomGen rand;
  const float anim_time;
  const float time_delta, inv_time_delta;
};

struct DrawContext : public SubSystemContext {
  DrawContext(const SubSystemContext &, FVec2);

  FVec2 inv_tex_tile;

  array<FVec2, 4> quadCorners(FVec2 pos, FVec2 size, float rotation) const;
  array<FVec2, 4> texQuadCorners(SVec2 tex_tile) const;
};

struct EmissionState {
  EmissionState(float time) : time(time) {}
  const float time;

  float rot_min, rot_max;
  float strength_min, strength_max;

  float max_life;
  float angle, angle_spread;
  float rot_speed_min, rot_speed_max; // TODO: change to value, value_var or value_spread

  float var[128];
};
}
