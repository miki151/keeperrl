#include "fx_manager.h"

#include "fx_color.h"
#include "fx_math.h"
#include "fx_particle_system.h"
#include "fx_rect.h"
#include "clock.h"

namespace fx {

static FXManager *s_instance = nullptr;

FXManager *FXManager::getInstance() { return s_instance; }

FXManager::FXManager() {
  m_randomGen = std::make_unique<RandomGen>();
  initializeTextureDefs();
  initializeDefs();
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}
FXManager::~FXManager() { s_instance = nullptr; }

bool FXManager::valid(ParticleDefId id) const { return id < (int)m_particleDefs.size(); }
bool FXManager::valid(EmitterDefId id) const { return id < (int)m_emitterDefs.size(); }

const ParticleDef &FXManager::operator[](ParticleDefId id) const {
  DASSERT(valid(id));
  return m_particleDefs[id];
}

const EmitterDef &FXManager::operator[](EmitterDefId id) const {
  DASSERT(valid(id));
  return m_emitterDefs[id];
}

const ParticleSystemDef& FXManager::operator[](FXName name) const {
  return m_systemDefs[name];
}

const TextureDef& FXManager::operator[](TextureName name) const {
  return m_textureDefs[name];
}

SubSystemContext FXManager::ssctx(ParticleSystem &ps, int ssid) {
  const auto &psdef = (*this)[ps.defId];
  const auto &pdef = (*this)[psdef[ssid].particleId];
  const auto &edef = (*this)[psdef[ssid].emitterId];
  return {ps, psdef, pdef, edef, m_textureDefs[pdef.textureName], ssid};
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

void FXManager::addSnapshot(float animTime, const ParticleSystem& ps) {
  SnapshotKey key(ps.params);
  for (auto& group : m_snapshotGroups[ps.defId])
    if (group.key == key) {
      group.snapshots.emplace_back(ps.subSystems);
      return;
    }
  m_snapshotGroups[ps.defId].emplace_back(SnapshotGroup{key, {ps.subSystems}});
}

auto FXManager::findSnapshotGroup(FXName name, SnapshotKey key) const -> const SnapshotGroup* {
  const SnapshotGroup* best = nullptr;
  float bestDist = fconstant::inf;

  for (auto& ssGroup : m_snapshotGroups[name]) {
    float dist = key.distanceSq(ssGroup.key);
    if (dist < bestDist) {
      bestDist = dist;
      best = &ssGroup;
    }
  }

  return best;
}

void FXManager::genSnapshots(FXName name, vector<float> animTimes, vector<float> params, int randomVariants) {
  auto startTime = Clock::getRealMicros().count();
  if (params.empty())
    params = {0.0f};
  if (animTimes.empty())
    return;

  static constexpr float fps = 60.0f;
  std::sort(begin(animTimes), end(animTimes));

  int numSnapshots = 0;
  for (float param0 : params) {
    for (int r = 0; r < randomVariants; r++) {
      auto ps = makeSystem(name, 0, {});

      ps.randomize(*m_randomGen);
      ps.params.scalar[0] = param0;

      float curTime = 0.0f;
      for (auto time : animTimes) {
        if (time <= curTime)
          continue;
        float simTime = time - curTime;
        while (simTime > 0.0001f) {
          float stepTime = min(1.0f / fps, simTime);
          simulate(ps, stepTime);
          simTime -= stepTime;
        }
        curTime = time;
        addSnapshot(curTime, ps);
        numSnapshots++;
      }
    }
  }

  auto time = double(Clock::getRealMicros().count() - startTime) / 1000.0;
  float maxTime = animTimes.back();
  int numFrames = maxTime * fps;

  // Try to keep this number low
  int numFramesTotal = numFrames * params.size() * randomVariants;

  INFO << "FX: generated " << numSnapshots << " snapshots for: " << EnumInfo<FXName>::getString(name) << " In: " << time
       << " msec" << " (total frames: " << numFramesTotal << ")";
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
      auto& tdef = m_textureDefs[pdef.textureName];
      DrawContext ctx{ssctx(ps, ssid), vinv(FVec2(tdef.tiles))};

      for (auto &pinst : ss.particles) {
        DrawParticle dparticle;
        ctx.ssdef.drawFunc(ctx, pinst, dparticle);
        dparticle.texName = pdef.textureName;
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

ParticleSystem FXManager::makeSystem(FXName name, uint spawnTime, InitConfig config) {
  if (config.snapshotKey)
    if (auto* ssGroup = findSnapshotGroup(name, *config.snapshotKey)) {
      int index = m_randomGen->get(ssGroup->snapshots.size());
      auto& key = ssGroup->key;
      INFO << "FX: using snapshot: " << EnumInfo<FXName>::getString(name) << " (" << key.scalar[0] << ", "
           << key.scalar[1] << ")";
      return {name, config, spawnTime, ssGroup->snapshots[index]};
    }

  auto& def = (*this)[name];
  ParticleSystem out{name, config, spawnTime, (int)def.subSystems.size()};
  for (int ssid = 0; ssid < (int)out.subSystems.size(); ssid++) {
    auto& ss = out.subSystems[ssid];
    ss.randomSeed = m_randomGen->get(INT_MAX);
    ss.emissionFract = (*this)[def.subSystems[ssid].emitterId].initialSpawnCount;
  }

  // TODO: initial particles
  return out;
}

ParticleSystemId FXManager::addSystem(FXName name, InitConfig config) {
  auto& def = (*this)[name];

  int new_slot = -1;
  for (int n = 0; n < (int)m_systems.size(); n++)
    if (m_systems[n].isDead) {
      if (m_systems[n].spawnTime == m_spawnClock) {
        m_spawnClock++;
        if (m_spawnClock == INT_MAX)
          m_spawnClock = 1;
      }

      m_systems[n] = makeSystem(name, m_spawnClock, config);
      return ParticleSystemId(n, m_spawnClock);
    }

  m_systems.emplace_back(makeSystem(name, m_spawnClock, config));
  return ParticleSystemId(m_systems.size() - 1, m_spawnClock);
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

void FXManager::addDef(FXName name, ParticleSystemDef def) {
  m_systemDefs[name] = std::move(def);
}
}
