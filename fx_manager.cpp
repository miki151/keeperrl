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
  randomGen = unique<RandomGen>();
  initializeTextureDefs();
  initializeDefs();
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}
FXManager::~FXManager() { s_instance = nullptr; }

const ParticleSystemDef& FXManager::operator[](FXName name) const {
  return systemDefs[name];
}

const TextureDef& FXManager::operator[](TextureName name) const {
  return textureDefs[name];
}

SubSystemContext FXManager::ssctx(ParticleSystem &ps, int ssid) {
  const auto &psdef = (*this)[ps.defId];
  const auto& pdef = psdef[ssid].particle;
  const auto& edef = psdef[ssid].emitter;
  return {ps, psdef, pdef, edef, textureDefs[pdef.textureName], ssid};
}

void FXManager::simulateStableTime(double time, int visibleFps, int simulateFps) {
  double diff = oldTime < 0 ? 1.0 / 60.0 : time - oldTime;
  if (diff < 0)
    diff = 1.0 / 60.0;
  simulateStable(diff, visibleFps, simulateFps);
  oldTime = time;
}

void FXManager::simulateStable(double timeDelta, int visibleFps, int simulateFps) {
  PROFILE;
  PASSERT(timeDelta >= 0.0);
  PASSERT(visibleFps <= simulateFps);
  PASSERT(simulateFps % visibleFps == 0);
  timeDelta += accumFrameTime;

  double drawDelta = 1.0 / visibleFps;
  double simulationDelta = 1.0 / simulateFps;
  int numSimSteps = simulateFps / visibleFps;
  int numSteps = 0;

  // TODO: limit number of steps?
  while (timeDelta > drawDelta) {
    for (int n = 0; n < numSimSteps; n++)
      simulate(simulationDelta);
    timeDelta -= drawDelta;
    numSteps++;
  }

  accumFrameTime = timeDelta;
}

void FXManager::simulate(ParticleSystem &ps, float timeDelta) {
  PROFILE;
  auto &psdef = (*this)[ps.defId];

  // Animating live particles
  for (int ssid = 0; ssid < (int)psdef.subSystems.size(); ssid++)
    if (!ps[ssid].particles.empty()) {
      auto &ss = ps[ssid];
      auto &ssdef = psdef[ssid];
      AnimationContext ctx(ssctx(ps, ssid), globalSimTime, ps.animTime, timeDelta);

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

    AnimationContext ctx(ssctx(ps, ssid), globalSimTime, ps.animTime, timeDelta);
    EmissionState em{emissionTime};
    memcpy(em.animationVars, ss.animationVars, sizeof(em.animationVars));

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
    memcpy(ss.animationVars, em.animationVars, sizeof(em.animationVars));
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
  PROFILE;
  for (auto& inst : systems)
    if (!inst.isDead)
      simulate(inst, delta);
  globalSimTime += delta;
}

void FXManager::addSnapshot(float animTime, const ParticleSystem& ps) {
  SnapshotKey key(ps.params);
  for (auto& group : snapshotGroups[ps.defId])
    if (group.key == key) {
      group.snapshots.emplace_back(ps.subSystems);
      return;
    }
  snapshotGroups[ps.defId].emplace_back(SnapshotGroup{key, {ps.subSystems}});
}

auto FXManager::findSnapshotGroup(FXName name, SnapshotKey key) const -> const SnapshotGroup* {
  const SnapshotGroup* best = nullptr;
  float bestDist = fconstant::inf;

  for (auto& ssGroup : snapshotGroups[name]) {
    float dist = key.distanceSq(ssGroup.key);
    if (dist < bestDist) {
      bestDist = dist;
      best = &ssGroup;
    }
  }

  return best;
}

void FXManager::genSnapshots(FXName name, vector<float> animTimes, vector<float> params, int randomVariants) {
  PROFILE;
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

      ps.randomize(*randomGen);
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

  INFO << "FX: generated " << numSnapshots << " snapshots for: " << ENUM_STRING(name) << " In: " << time << " msec"
       << " (total frames: " << numFramesTotal << ")";
}

void FXManager::genQuads(vector<DrawParticle>& out, int id, int ssid) {
  PROFILE;
  auto& ps = systems[id];
  if (ps.isDead)
    return;

  auto& psdef = (*this)[ps.defId];
  auto& ss = ps[ssid];
  auto& ssdef = psdef[ssid];

  auto& pdef = ssdef.particle;
  auto& tdef = textureDefs[pdef.textureName];
  DrawContext ctx{ssctx(ps, ssid), vinv(FVec2(tdef.tiles))};

  if (ctx.ssdef.multiDrawFunc)
    for (auto& pinst : ss.particles) {
      ctx.ssdef.multiDrawFunc(ctx, pinst, out, ps.color);
    }
  else
    for (auto& pinst : ss.particles) {
      DrawParticle dparticle;
      if (ctx.ssdef.drawFunc(ctx, pinst, dparticle)) {
        dparticle.color = dparticle.color.blend(ps.color);
        out.push_back(std::move(dparticle));
      }
    }
}

bool FXManager::valid(ParticleSystemId id) const {
  return id >= 0 && id < (int)systems.size() && systems[id].spawnTime == id.getSpawnTime();
}

bool FXManager::dead(ParticleSystemId id) const {
  return !valid(id) || systems[id].isDead;
}
bool FXManager::alive(ParticleSystemId id) const {
  return valid(id) && !systems[id].isDead;
}

void FXManager::kill(ParticleSystemId id, bool immediate) {
  if (!dead(id))
    systems[id].kill(immediate);
}

ParticleSystem& FXManager::get(ParticleSystemId id) {
  DASSERT(valid(id));
  return systems[id];
}

const ParticleSystem& FXManager::get(ParticleSystemId id) const {
  DASSERT(valid(id));
  return systems[id];
}

ParticleSystem FXManager::makeSystem(FXName name, uint spawnTime, InitConfig config) {
  if (config.snapshotKey)
    if (auto* ssGroup = findSnapshotGroup(name, *config.snapshotKey)) {
      int index = randomGen->get(ssGroup->snapshots.size());
      auto& key = ssGroup->key;
      INFO << "FX: using snapshot: " << ENUM_STRING(name) << " (" << key.scalar[0] << ", " << key.scalar[1] << ")";
      return ParticleSystem(name, config, spawnTime, ssGroup->snapshots[index]);
    }

  auto& def = (*this)[name];
  ParticleSystem out(name, config, spawnTime, vector<ParticleSystem::SubSystem>((int)def.subSystems.size()));
  if (def.randomOffset)
    out.animTime = Random.getFloat(0, *def.animLength);
  for (int ssid = 0; ssid < (int)out.subSystems.size(); ssid++) {
    auto& ss = out.subSystems[ssid];
    ss.randomSeed = randomGen->get(INT_MAX);
    ss.emissionFract = def.subSystems[ssid].emitter.initialSpawnCount;
  }

  // TODO: initial particles
  return out;
}

ParticleSystemId FXManager::addSystem(FXName name, InitConfig config) {
  for (int n = 0; n < (int)systems.size(); n++)
    if (systems[n].isDead) {
      if (systems[n].spawnTime == spawnClock) {
        spawnClock++;
        if (spawnClock == INT_MAX)
          spawnClock = 1;
      }

      systems[n] = makeSystem(name, spawnClock, config);
      return ParticleSystemId(n, spawnClock);
    }

  systems.emplace_back(makeSystem(name, spawnClock, config));
  return ParticleSystemId(systems.size() - 1, spawnClock);
}

void FXManager::addDef(FXName name, ParticleSystemDef def) {
  systemDefs[name] = std::move(def);
}
}
