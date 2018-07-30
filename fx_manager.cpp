#include "fx_manager.h"

#include "fx_color.h"
#include "fx_emitter_def.h"
#include "fx_math.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"
#include "fx_rect.h"

namespace fx {

static FXManager *s_instance = nullptr;

FXManager *FXManager::getInstance() { return s_instance; }

FXManager::FXManager() {
  addDefaultDefs();
  //saveDefs();

  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}
FXManager::~FXManager() { s_instance = nullptr; }

bool FXManager::valid(ParticleDefId id) const { return id < (int)m_particle_defs.size(); }
bool FXManager::valid(EmitterDefId id) const { return id < (int)m_emitter_defs.size(); }
bool FXManager::valid(ParticleSystemDefId id) const { return id < (int)m_system_defs.size(); }

const ParticleDef &FXManager::operator[](ParticleDefId id) const {
  DASSERT(valid(id));
  return m_particle_defs[id];
}
const EmitterDef &FXManager::operator[](EmitterDefId id) const {
  DASSERT(valid(id));
  return m_emitter_defs[id];
}
const ParticleSystemDef &FXManager::operator[](ParticleSystemDefId id) const {
  DASSERT(valid(id));
  return m_system_defs[id];
}

optional<ParticleSystemDefId> FXManager::findSystem(const char *name) const {
  PASSERT(name);
  for(int n = 0; n < (int)m_system_defs.size(); n++)
    if(m_system_defs[n].name == name)
      return ParticleSystemDefId(n);
  return none;
}

SubSystemContext FXManager::ssctx(ParticleSystem &ps, int ssid) {
  const auto &psdef = (*this)[ps.def_id];
  const auto &pdef = (*this)[psdef[ssid].particle_id];
  const auto &edef = (*this)[psdef[ssid].emitter_id];
  return {ps, psdef, pdef, edef, ssid};
}

void FXManager::simulateStableTime(double time, int desired_fps) {
  double diff = m_old_time < 0 ? 1.0 / 60.0 : time - m_old_time;
  simulateStable(diff, desired_fps);
  m_old_time = time;
}

void FXManager::simulateStable(double time_delta, int desired_fps) {
  time_delta += m_accum_frame_time;

  double desired_delta = 1.0 / desired_fps;
  int num_steps = 0;

  // TODO: czy chcemy jakoś przewidywać klatki?
  while(time_delta > desired_delta) {
    simulate(desired_delta);
    time_delta -= desired_delta;
    num_steps++;
  }

  if(time_delta > desired_delta * 0.1) {
    num_steps++;
    simulate(time_delta);
    m_accum_frame_time = 0.0;
  } else {
    m_accum_frame_time = time_delta;
  }

  //print("steps: %\n", num_steps);
}

void FXManager::simulate(ParticleSystem &ps, float time_delta) {
  auto &psdef = (*this)[ps.def_id];

  // Animating live particles
  for(int ssid = 0; ssid < (int)psdef.subsystems.size(); ssid++) {
    auto &ss = ps[ssid];
    auto &ssdef = psdef[ssid];

    AnimationContext ctx(ssctx(ps, ssid), ps.anim_time, time_delta);
    ctx.rand.init(ss.random_seed);

    for(auto &pinst : ss.particles)
      ssdef.animate_func(ctx, pinst);

    ss.random_seed = ctx.randomSeed();
  }

  // Removing dead particles
  for(auto &ssinst : ps.subsystems)
    for(int n = 0; n < (int)ssinst.particles.size(); n++) {
      auto &pinst = ssinst.particles[n];
      if(pinst.life > pinst.max_life) {
        pinst = ssinst.particles.back();
        ssinst.particles.pop_back();
        n--;
      }
    }

  // Emitting new particles
  for(int ssid = 0; ssid < (int)psdef.subsystems.size(); ssid++) {
    const auto &ssdef = psdef[ssid];

    float emission_time_span = ssdef.emission_end - ssdef.emission_start;
    float emission_time = (ps.anim_time - ssdef.emission_start) / emission_time_span;
    if(emission_time < 0.0f || emission_time > 1.0f)
      continue;

    auto &ss = ps[ssid];
    AnimationContext ctx(ssctx(ps, ssid), ps.anim_time, time_delta);
    ctx.rand.init(ss.random_seed);
    EmissionState em{emission_time};

    float emission = ssdef.prepare_func(ctx, em) + ss.emission_fract;
    int num_particles = int(emission);
    ss.emission_fract = emission - float(num_particles);

    int max_particles =
        min(ssdef.max_active_particles - (int)ss.particles.size(), ssdef.max_total_particles - ss.total_particles);
    num_particles = min(num_particles, max_particles);

    for(int n = 0; n < num_particles; n++) {
      Particle new_inst;
      ssdef.emit_func(ctx, em, new_inst);
      ss.particles.emplace_back(new_inst);
      ss.total_particles++;
    }
    ss.random_seed = ctx.randomSeed(); // TODO: save random state properly?
  }

  ps.anim_time += time_delta;

  bool finished_anim = false;

  if(psdef.anim_length) {
    finished_anim = ps.anim_time >= *psdef.anim_length;
  } else {
    float end_emission_time = 0.0f;
    int num_active = 0;

    for(int ssid = 0; ssid < (int)psdef.subsystems.size(); ssid++) {
      auto &ss = ps[ssid];
      num_active += (int)ps[ssid].particles.size();
      end_emission_time = max(end_emission_time, psdef[ssid].emission_end);
    }

    finished_anim = num_active == 0 && ps.anim_time >= end_emission_time;
  }

  if(finished_anim) {
    // TODO: accurate animation loop
    if(psdef.is_looped)
      ps.anim_time = 0.0f;
    else
      ps.kill();
  }
}

void FXManager::simulate(float delta) {
  for(auto &inst : m_systems)
    if(!inst.is_dead)
      simulate(inst, delta);
}

vector<DrawParticle> FXManager::genQuads() {
  vector<DrawParticle> out;
  // TODO(opt): reserve

  for(auto &ps : m_systems) {
    if(ps.is_dead)
      continue;
    auto &psdef = (*this)[ps.def_id];

    for(int ssid = 0; ssid < (int)psdef.subsystems.size(); ssid++) {
      auto &ss = ps[ssid];
      auto &ssdef = psdef[ssid];
      auto &pdef = m_particle_defs[ssdef.particle_id];
      DrawContext ctx{ssctx(ps, ssid), vinv(FVec2(pdef.texture_tiles))};

      for(auto &pinst : ss.particles) {
        DrawParticle dparticle;
        ctx.ssdef.draw_func(ctx, pinst, dparticle);
        dparticle.particle_def_id = ssdef.particle_id;
        out.emplace_back(dparticle);
      }
    }
  }

  return out;
}

bool FXManager::valid(ParticleSystemId id) const {
  return id >= 0 && id < (int)m_systems.size() && m_systems[id].spawn_time == id.spawnTime();
}

bool FXManager::dead(ParticleSystemId id) const { return !valid(id) || m_systems[id].is_dead; }
bool FXManager::alive(ParticleSystemId id) const { return valid(id) && !m_systems[id].is_dead; }

void FXManager::kill(ParticleSystemId id) {
  if(!dead(id))
    m_systems[id].kill();
}

ParticleSystem &FXManager::get(ParticleSystemId id) {
  DASSERT(valid(id));
  return m_systems[id];
}

const ParticleSystem &FXManager::get(ParticleSystemId id) const {
  DASSERT(valid(id));
  return m_systems[id];
}

ParticleSystemId FXManager::addSystem(ParticleSystemDefId def_id, FVec2 pos) {
  auto &def = (*this)[def_id];

  for(int n = 0; n < (int)m_systems.size(); n++)
    if(m_systems[n].is_dead) {
      if(m_systems[n].spawn_time == m_spawn_clock)
        m_spawn_clock++;

      m_systems[n] = {pos, def_id, m_spawn_clock, (int)def.subsystems.size()};
      initialize(def, m_systems[n]);
      return ParticleSystemId(n, m_spawn_clock);
    }

  m_systems.emplace_back(pos, def_id, m_spawn_clock, (int)def.subsystems.size());
  initialize(def, m_systems.back());
  return ParticleSystemId(m_systems.size() - 1, m_spawn_clock);
}

void FXManager::initialize(const ParticleSystemDef &def, ParticleSystem &ps) {
  for(int ssid = 0; ssid < (int)ps.subsystems.size(); ssid++) {
    auto &ss = ps.subsystems[ssid];
    ss.random_seed = m_random_seed++;
    ss.emission_fract = (*this)[def.subsystems[ssid].emitter_id].initial_spawn_count;
  }
  // TODO: initial particles
}

vector<ParticleSystemId> FXManager::aliveSystems() const {
  vector<ParticleSystemId> out;
  out.reserve(m_systems.size());

  for(int n = 0; n < (int)m_systems.size(); n++)
    if(!m_systems[n].is_dead)
      out.emplace_back(ParticleSystemId(n, m_systems[n].spawn_time));

  return out;
}

ParticleDefId FXManager::addDef(ParticleDef def) {
  m_particle_defs.emplace_back(std::move(def));
  return ParticleDefId(m_particle_defs.size() - 1);
}

EmitterDefId FXManager::addDef(EmitterDef def) {
  m_emitter_defs.emplace_back(std::move(def));
  return EmitterDefId(m_emitter_defs.size() - 1);
}

ParticleSystemDefId FXManager::addDef(ParticleSystemDef def) {
  m_system_defs.emplace_back(std::move(def));
  return ParticleSystemDefId(m_system_defs.size() - 1);
}
}
