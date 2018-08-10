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
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}
FXManager::~FXManager() { s_instance = nullptr; }

bool FXManager::valid(ParticleDefId id) const { return id < (int)m_particleDefs.size(); }
bool FXManager::valid(EmitterDefId id) const { return id < (int)m_emitterDefs.size(); }
bool FXManager::valid(ParticleSystemDefId id) const { return id < (int)m_systemDefs.size(); }

const ParticleDef &FXManager::operator[](ParticleDefId id) const {
  DASSERT(valid(id));
  return m_particleDefs[id];
}
const EmitterDef &FXManager::operator[](EmitterDefId id) const {
  DASSERT(valid(id));
  return m_emitterDefs[id];
}
const ParticleSystemDef &FXManager::operator[](ParticleSystemDefId id) const {
  DASSERT(valid(id));
  return m_systemDefs[id];
}

optional<ParticleSystemDefId> FXManager::findSystem(const char *name) const {
  PASSERT(name);
  for (int n = 0; n < (int)m_systemDefs.size(); n++)
    if (m_systemDefs[n].name == name)
      return ParticleSystemDefId(n);
  return none;
}

SubSystemContext FXManager::ssctx(ParticleSystem &ps, int ssid) {
  const auto &psdef = (*this)[ps.defId];
  const auto &pdef = (*this)[psdef[ssid].particleId];
  const auto &edef = (*this)[psdef[ssid].emitterId];
  return {ps, psdef, pdef, edef, ssid};
}

void FXManager::simulateStableTime(double time, int desiredFps) {
  double diff = m_oldTime < 0 ? 1.0 / 60.0 : time - m_oldTime;
  simulateStable(diff, desiredFps);
  m_oldTime = time;
}

void FXManager::simulateStable(double timeDelta, int desiredFps) {
  timeDelta += m_accumFrameTime;

  double desiredDelta = 1.0 / desiredFps;
  int numSteps = 0;

  // TODO: limit number of steps?
  while (timeDelta > desiredDelta) {
    simulate(desiredDelta);
    timeDelta -= desiredDelta;
    numSteps++;
  }

  if (timeDelta > desiredDelta * 0.1) {
    numSteps++;
    simulate(timeDelta);
    m_accumFrameTime = 0.0;
  } else {
    m_accumFrameTime = timeDelta;
  }
}

void FXManager::simulate(ParticleSystem &ps, float timeDelta) {
  auto &psdef = (*this)[ps.defId];

  // Animating live particles
  for (int ssid = 0; ssid < (int)psdef.subSystems.size(); ssid++) {
    auto &ss = ps[ssid];
    auto &ssdef = psdef[ssid];

    AnimationContext ctx(ssctx(ps, ssid), ps.animTime, timeDelta);
    ctx.rand.init(ss.randomSeed);

    for (auto &pinst : ss.particles)
      ssdef.animateFunc(ctx, pinst);

    ss.randomSeed = ctx.randomSeed();
  }

  // Removing dead particles
  for (auto &ssinst : ps.subSystems)
    for (int n = 0; n < (int)ssinst.particles.size(); n++) {
      auto &pinst = ssinst.particles[n];
      if (pinst.life > pinst.maxLife) {
        pinst = ssinst.particles.back();
        ssinst.particles.pop_back();
        n--;
      }
    }

  // Emitting new particles
  for (int ssid = 0; ssid < (int)psdef.subSystems.size(); ssid++) {
    const auto &ssdef = psdef[ssid];

    if (ps.isDying)
      break;

    float emissionTimeSpan = ssdef.emissionEnd - ssdef.emissionStart;
    float emissionTime = (ps.animTime - ssdef.emissionStart) / emissionTimeSpan;
    auto &ss = ps[ssid];

    if (emissionTime < 0.0f || emissionTime > 1.0f)
      continue;

    AnimationContext ctx(ssctx(ps, ssid), ps.animTime, timeDelta);
    ctx.rand.init(ss.randomSeed);
    EmissionState em{emissionTime};

    float emission = ssdef.prepareFunc(ctx, em) + ss.emissionFract;
    int numParticles = int(emission);
    ss.emissionFract = emission - float(numParticles);

    int maxParticles =
        min(ssdef.maxActiveParticles - (int)ss.particles.size(), ssdef.maxTotalParticles - ss.totalParticles);
    numParticles = min(numParticles, maxParticles);

    for (int n = 0; n < numParticles; n++) {
      Particle newInst;
      ssdef.emitFunc(ctx, em, newInst);
      ss.particles.emplace_back(newInst);
      ss.totalParticles++;
    }
    ss.randomSeed = ctx.randomSeed(); // TODO: save random state properly?
  }

  int numActive = 0;
  for (auto &ss : ps.subSystems)
    numActive += (int)ss.particles.size();

  if (ps.isDying && numActive == 0)
    ps.kill(true);

  ps.animTime += timeDelta;

  bool finishedAnim = false;

  if (psdef.animLength) {
    finishedAnim = ps.animTime >= *psdef.animLength;
  } else {
    float endEmissionTime = 0.0f;
    for (int ssid = 0; ssid < (int)psdef.subSystems.size(); ssid++)
      endEmissionTime = max(endEmissionTime, psdef[ssid].emissionEnd);
    finishedAnim = numActive == 0 && ps.animTime >= endEmissionTime;
  }

  if (finishedAnim) {
    // TODO: accurate animation loop
    if (psdef.isLooped)
      ps.animTime = 0.0f;
    else
      ps.kill(true);
  }
}

void FXManager::simulate(float delta) {
  for (auto &inst : m_systems)
    if (!inst.isDead)
      simulate(inst, delta);
}

vector<DrawParticle> FXManager::genQuads() {
  vector<DrawParticle> out;
  // TODO(opt): reserve

  for (auto &ps : m_systems) {
    if (ps.isDead)
      continue;
    auto &psdef = (*this)[ps.defId];

    for (int ssid = 0; ssid < (int)psdef.subSystems.size(); ssid++) {
      auto &ss = ps[ssid];
      auto &ssdef = psdef[ssid];
      auto &pdef = m_particleDefs[ssdef.particleId];
      DrawContext ctx{ssctx(ps, ssid), vinv(FVec2(pdef.textureTiles))};

      for (auto &pinst : ss.particles) {
        DrawParticle dparticle;
        ctx.ssdef.drawFunc(ctx, pinst, dparticle);
        dparticle.particleDefId = ssdef.particleId;
        dparticle.blendMode = pdef.blendMode;
        out.emplace_back(dparticle);
      }
    }
  }

  return out;
}

bool FXManager::valid(ParticleSystemId id) const {
  return id >= 0 && id < (int)m_systems.size() && m_systems[id].spawnTime == id.spawnTime();
}

bool FXManager::dead(ParticleSystemId id) const { return !valid(id) || m_systems[id].isDead; }
bool FXManager::alive(ParticleSystemId id) const { return valid(id) && !m_systems[id].isDead; }

void FXManager::kill(ParticleSystemId id, bool immediate) {
  if (!dead(id))
    m_systems[id].kill(immediate);
}

ParticleSystem &FXManager::get(ParticleSystemId id) {
  DASSERT(valid(id));
  return m_systems[id];
}

const ParticleSystem &FXManager::get(ParticleSystemId id) const {
  DASSERT(valid(id));
  return m_systems[id];
}

ParticleSystemId FXManager::addSystem(ParticleSystemDefId defId, FVec2 pos) { return addSystem(defId, pos, {}); }

ParticleSystemId FXManager::addSystem(ParticleSystemDefId defId, FVec2 pos, FVec2 targetOff) {
  auto &def = (*this)[defId];

  for (int n = 0; n < (int)m_systems.size(); n++)
    if (m_systems[n].isDead) {
      if (m_systems[n].spawnTime == m_spawnClock)
        m_spawnClock++;

      m_systems[n] = {pos, targetOff, defId, m_spawnClock, (int)def.subSystems.size()};
      initialize(def, m_systems[n]);
      return ParticleSystemId(n, m_spawnClock);
    }

  m_systems.emplace_back(pos, targetOff, defId, m_spawnClock, (int)def.subSystems.size());
  initialize(def, m_systems.back());
  return ParticleSystemId(m_systems.size() - 1, m_spawnClock);
}

void FXManager::initialize(const ParticleSystemDef &def, ParticleSystem &ps) {
  for (int ssid = 0; ssid < (int)ps.subSystems.size(); ssid++) {
    auto &ss = ps.subSystems[ssid];
    ss.randomSeed = m_randomSeed++;
    ss.emissionFract = (*this)[def.subSystems[ssid].emitterId].initialSpawnCount;
  }
  // TODO: initial particles
}

vector<ParticleSystemId> FXManager::aliveSystems() const {
  vector<ParticleSystemId> out;
  out.reserve(m_systems.size());

  for (int n = 0; n < (int)m_systems.size(); n++)
    if (!m_systems[n].isDead)
      out.emplace_back(ParticleSystemId(n, m_systems[n].spawnTime));

  return out;
}

ParticleDefId FXManager::addDef(ParticleDef def) {
  m_particleDefs.emplace_back(std::move(def));
  return ParticleDefId(m_particleDefs.size() - 1);
}

EmitterDefId FXManager::addDef(EmitterDef def) {
  m_emitterDefs.emplace_back(std::move(def));
  return EmitterDefId(m_emitterDefs.size() - 1);
}

ParticleSystemDefId FXManager::addDef(ParticleSystemDef def) {
  m_systemDefs.emplace_back(std::move(def));
  return ParticleSystemDefId(m_systemDefs.size() - 1);
}
}
