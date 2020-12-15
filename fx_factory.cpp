#include "fx_manager.h"

#include "fx_color.h"
#include "fx_defs.h"
#include "fx_particle_system.h"
#include "fx_rect.h"
#include "renderer.h"

namespace fx {

// TODO: normalize it ?
static const FVec2 dirVecs[8] = {
  {0.0, -1.0}, {0.0, 1.0},   {1.0, 0.0}, {-1.0, 0.0},
  {1.0, -1.0}, {-1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}};
FVec2 dirToVec(Dir dir) {
  return dirVecs[(int)dir];
}

template <class T, size_t N> const T& choose(const array<T, N>& values, uint randomSeed) {
  return values[randomSeed % N];
}

static void addTestSimpleEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength = 30.0f;
  edef.frequency = {{10.0f, 55.0f, 0.0f, 0.0}, InterpType::cosine};

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 32.0f;
  pdef.alpha = {{0.0f, 0.1, 0.8f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = {{{1.0f, 1.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {0.2f, 0.5f, 1.0f}}, InterpType::linear};
  pdef.textureName = TextureName::CIRCULAR;

  ParticleSystemDef psdef;
  psdef.subSystems.emplace_back(pdef, edef, 0.0f, 5.0f);
  mgr.addDef(FXName::TEST_SIMPLE, psdef);
}

static void addTestMultiEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength = 30.0f;
  edef.frequency = {{10.0f, 55.0f, 0.0f, 0.0}, InterpType::cosine};

  FVec3 colors[5] = {{0.7f, 0.2, 0.2f}, {0.2f, 0.7f, 0.2f}, {0.2f, 0.2f, 0.7f}, {0.9f, 0.2, 0.9f}, {0.3f, 0.9f, 0.4f}};

  ParticleDef pdefs[5];
  for (int n = 0; n < 5; n++) {
    pdefs[n].life = 1.0f;
    pdefs[n].size = 32.0f;
    pdefs[n].alpha = {{0.0f, 0.1, 0.8f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::linear};
    pdefs[n].color = {colors[n]};
    pdefs[n].textureName = TextureName::CIRCULAR;
  }

  ParticleSystemDef psdef;
  for (int n = 0; n < 5; n++)
    psdef.subSystems.emplace_back(pdefs[n], edef, float(n) * 0.5f, float(n) * 1.5f + 2.0f);
  mgr.addDef(FXName::TEST_MULTI, psdef);
}

static void addWoodSplinters(FXManager &mgr) {
  EmitterDef edef;
  edef.setStrengthSpread(40.0f, 20.0f);
  edef.rotSpeed = 0.5f;
  edef.frequency = 999.0f;

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 4.0f;
  pdef.slowdown = {{0.0f, 0.1f}, {5.0f, 1000.0f}};
  pdef.alpha = {{0.0f, 0.3f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

  auto animateFunc = [](AnimationContext &ctx, Particle &pinst) {
    defaultAnimateParticle(ctx, pinst);
    float shadow_min = 5.0f, shadow_max = 10.0f;
    if (pinst.pos.y < shadow_min) {
      float dist = shadow_min - pinst.pos.y;
      pinst.movement += FVec2(0.0f, dist);
    }
    pinst.pos.y = min(pinst.pos.y, shadow_max);
  };

  FColor brown(Color(120, 87, 46));
  // Kiedy cząsteczki opadną pod drzewo, robią się w zasięgu cienia
  // TODO: lepiej rysować je po prostu pod cieniem
  pdef.color = {{0.0f, 0.15f, 0.17}, {brown.rgb(), brown.rgb(), brown.rgb() * 0.6f}};
  pdef.textureName = TextureName::FLAKES_BORDERS;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.maxTotalParticles = 7;
  ssdef.animateFunc = animateFunc;
  ssdef.layer = Layer::back;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  mgr.addDef(FXName::WOOD_SPLINTERS, psdef);
}

static void addRockSplinters(FXManager &mgr) {
  // Opcja: spawnowanie splinterów na tym samym kaflu co minion:
  // - Problem: Te particle powinny się spawnować na tym samym tile-u co imp
  //   i spadać mu pod nogi, tak jak drewnianie drzazgi; Tylko jak to
  //   zrobić w przypadku kafli po diagonalach ?
  // - Problem: czy te particle wyświetlają się nad czy pod impem?
  //
  // Chyba prościej jest po prostu wyświetlać te particle na kaflu z rozwalanym
  // murem; Zresztą jest to bardziej spójne z particlami dla drzew
  EmitterDef edef;
  edef.setStrengthSpread(40.0f, 20.0f);
  edef.rotSpeed = 0.5f;
  edef.frequency = 999.0f;

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 4.0f;
  pdef.slowdown = {{0.0f, 0.1f}, {5.0f, 1000.0f}};
  pdef.alpha = {{0.0f, 0.4f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

  pdef.color = FVec3(0.4, 0.4, 0.4);
  pdef.textureName = TextureName::FLAKES_BORDERS;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.maxTotalParticles = 5;
  ssdef.layer = Layer::back;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  // TODO: cząsteczki mogą mieć różny czas życia
  mgr.addDef(FXName::ROCK_SPLINTERS, psdef);
}

static void addRockCloud(FXManager &mgr) {
  EmitterDef edef;
  edef.source = FRect(-5.0f, -5.0f, 5.0f, 5.0f);
  edef.setStrengthSpread(6.5f, 1.5f);
  edef.frequency = 60.0f;

  ParticleDef pdef;
  pdef.life = 3.5f;
  pdef.size = {{0.0f, 0.1f, 1.0f}, {15.0f, 30.0f, 38.0f}};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.3f, 0.4f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};

  FVec3 start_color(0.6), end_color(0.4);
  pdef.color = {{start_color, end_color}};
  pdef.textureName = TextureName::CLOUDS_SOFT;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.maxTotalParticles = 5;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  mgr.addDef(FXName::ROCK_CLOUD, psdef);
}

static FVec3 rgb(Color color) {
  return FColor(color).rgb();
}

static void addPoisonCloud(FXManager& mgr) {
  EmitterDef edef;
  edef.source = FRect(-5.0f, -5.0f, 5.0f, 5.0f);
  edef.setStrengthSpread(10.0f, 2.0f);
  edef.frequency = 8.0f;

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = {{0.0f, 0.4f, 1.0f}, {15.0f, 30.0f, 38.0f}};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.3f, 0.4f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};
  pdef.color = rgb(Color(100, 200, 50));
  pdef.textureName = TextureName::CLOUDS_SOFT;

  SubSystemDef ssdef1(pdef, edef, 0.0f, 1.0f);
  auto ssdef2 = ssdef1;

  // Smoke cloud
  ssdef1.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
    if (!defaultDrawParticle(ctx, pinst, out))
      return false;
    float mod = 0.0f + ctx.ps.params.scalar[0] * 0.3f;
    out.color.a = (out.color.a) * mod;
    return true;
  };

  // Glowing cloud
  ssdef2.particle.textureName = TextureName::CLOUDS_ADD;
  ssdef2.emitter.frequency = 4.0f;
  ssdef2.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
    if (!defaultDrawParticle(ctx, pinst, out))
      return false;
    float mod = max(0.0f, ctx.ps.params.scalar[0] * 0.3f - 0.1f);
    out.color = Color(FColor(out.color) * mod);
    return true;
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef1, ssdef2};
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::POISON_CLOUD, psdef);
  mgr.genSnapshots(FXName::POISON_CLOUD, {1.0f, 1.2f, 1.4f}, {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f}, 2);
}

static void addDestroyFurnitureEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.setStrengthSpread(70.0f, 20.0f);
  edef.rotSpeed = 0.7f;
  edef.initialSpawnCount = 30;
  edef.source = FRect(-3, -3, 3, 3);

  ParticleDef pdef;
  pdef.life = 1.5f;
  pdef.size = 6.0f;
  pdef.slowdown = 20000.0f;
  pdef.alpha = {{0.0f, 0.4f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};
  pdef.textureName = TextureName::FLAKES_BORDERS;
  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.layer = Layer::back; // will be drawn underneath creatures

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  // TODO: cząsteczki mogą mieć różny czas życia
  mgr.addDef(FXName::DESTROY_FURNITURE, psdef);
}

static void addFireballSplashEffect(FXManager& mgr) {
  // TODO: tutaj trzeba zrobić tak, żeby cząsteczki które spawnują się później
  // zaczynały z innym kolorem
  EmitterDef edef;
  edef.strength = 15.0f;
  edef.frequency = 40.0f;

  ParticleDef pdef;
  pdef.life = 0.3f;
  pdef.size = {{12.0f, 15.0f}};

  // TODO: something's wrong with color interpolation here
  // (try setting color2 to (200, 20, 20) and use only keys: 0.7 & 1.0)
  Color color1(137, 76, 25), color2(200, 0, 0);
  pdef.color = {{0.0f, 0.7f, 1.0f}, {rgb(color1), rgb(color1), rgb(color2)}};
  pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
  pdef.textureName = TextureName::FLAMES;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.35f);
  ssdef.maxTotalParticles = 20;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  mgr.addDef(FXName::FIREBALL_SPLASH, psdef);
}

static void addRippleEffect(FXManager &mgr) {
  EmitterDef edef;

  // Ta animacja nie ma sprecyzowanej długości;
  // Zamiast tego może być włączona / wyłączona albo może się zwięszyć/zmniejszyć jej siła
  // krzywe które zależą od czasu animacji tracą sens;
  // animacja może być po prostu zapętlona
  edef.frequency = 1.5f;
  edef.initialSpawnCount = 1.0f;

  // First scalar parameter controls how fast the ripples move
  auto prepFunc = [](AnimationContext &ctx, EmissionState &em) {
    float freq = defaultPrepareEmission(ctx, em);
    float mod = 1.0f + ctx.ps.params.scalar[0];
    return freq * mod;
  };

  auto animateFunc = [](AnimationContext &ctx, Particle &pinst) {
    float ptime = pinst.particleTime();
    float mod = 1.0f + ctx.ps.params.scalar[0];
    float timeDelta = ctx.timeDelta * mod;
    pinst.pos += pinst.movement * timeDelta;
    pinst.rot += pinst.rotSpeed * timeDelta;
    pinst.life += timeDelta;
  };

  ParticleDef pdef;
  pdef.life = 1.5f;
  pdef.size = {{10.0f, 50.0f}};
  pdef.alpha = {{0.0f, 0.3f, 0.6f, 1.0f}, {0.0f, 0.3f, 0.5f, 0.0f}};

  pdef.color = FVec3(1.0f, 1.0f, 1.0f);
  pdef.textureName = TextureName::TORUS;

  // TODO: możliwośc nie podawania czasu emisji (etedy emituje przez całą długośc animacji)?
  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.maxActiveParticles = 10;
  ssdef.prepareFunc = prepFunc;
  ssdef.animateFunc = animateFunc;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.animLength = 1.0f;
  psdef.isLooped = true;

  mgr.addDef(FXName::RIPPLE, psdef);
}

static void addCircularSpell(FXManager& mgr) {
  EmitterDef edef;
  edef.frequency = 50.0f;
  edef.initialSpawnCount = 10.0;

  auto prepFunc = [](AnimationContext &ctx, EmissionState &em) {
    defaultPrepareEmission(ctx, em);
    return 0.0f;
  };

  auto emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &new_inst) {
    new_inst.life = min(em.maxLife, float(ctx.ss.totalParticles) * 0.01f);
    new_inst.maxLife = em.maxLife;
  };

  auto animateFunc = [](AnimationContext &ctx, Particle &pinst) {
    float ptime = pinst.particleTime();
    float mod = 1.0f + ctx.ps.params.scalar[0];
    float timeDelta = ctx.timeDelta * mod;
    pinst.life += pinst.movement.x;
    pinst.movement.x = 0;
    pinst.life += timeDelta;
  };

  ParticleDef pdef;
  pdef.life = 0.5f;
  pdef.size = {{10.0f, 60.0f}, InterpType::cosine};
  pdef.alpha = {{0.0f, 0.03f, 0.2f, 1.0f}, {0.0f, 0.15f, 0.15f, 0.0f}, InterpType::cosine};

  pdef.color = {{0.5f, 0.8f}, {{1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.5f}}};
  pdef.textureName = TextureName::TORUS;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.maxActiveParticles = 20;
  ssdef.prepareFunc = prepFunc;
  ssdef.animateFunc = animateFunc;
  ssdef.emitFunc = emitFunc;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};

  mgr.addDef(FXName::CIRCULAR_SPELL, psdef);
}

static void addCircularBlast(FXManager& mgr) {
  ParticleSystemDef psdef;

  {
    EmitterDef edef;
    edef.frequency = {{150.0f, 0.0f}};

    ParticleDef pdef;
    pdef.life = 0.2f;
    pdef.size = {{20.0f, 30.0f}};
    pdef.alpha = {{0.0f, 0.2f, 1.0f}, {0.0f, 0.3f, 0.0f}, InterpType::cosine};
    pdef.slowdown = 1.0f;
    pdef.textureName = TextureName::BLAST;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.3f);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) -> float {
      auto freq = defaultPrepareEmission(ctx, em);
      em.direction = ctx.ps.targetDirAngle;
      em.directionSpread = 0.0f;
      return freq;
    };
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float& pos = em.animationVars[0];
      pinst.pos += ctx.ps.targetDir * (12.0f + 24.0f * pos);
      pos += 0.06f;
      pinst.rot = ctx.ps.targetDirAngle;
      pinst.size *= (0.8f + pos * 0.4f);
      if (pos > 1.0f)
        pinst.life = pinst.maxLife;
    };
    ssdef.multiDrawFunc = [](DrawContext& ctx, const Particle& pinst, vector<DrawParticle>& out, Color color) {
      for (int n = 0; n < 12; n++) {
        float angle = float(n) * (fconstant::pi * 2.0f / 12.0f);
        DrawParticle dpart;
        Particle temp(pinst);
        temp.pos = rotateVector(temp.pos, angle);
        temp.rot = angle;
        if (defaultDrawParticle(ctx, temp, dpart)) {
          dpart.color = dpart.color.blend(color);
          out.push_back(std::move(dpart));
        }
      }
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::CIRCULAR_BLAST, psdef);
}

static void addAirBlast(FXManager& mgr) {
  ParticleSystemDef psdef;

  {
    EmitterDef edef;
    edef.frequency = {{200.0f, 0.0f}};

    ParticleDef pdef;
    pdef.life = 0.2f;
    pdef.size = {{20.0f, 30.0f}};
    pdef.alpha = {{0.0f, 0.2f, 1.0f}, {0.0f, 0.5f, 0.0f}, InterpType::cosine};
    pdef.slowdown = 1.0f;
    pdef.textureName = TextureName::BLAST;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.3f);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) -> float {
      auto freq = defaultPrepareEmission(ctx, em);
      em.direction = ctx.ps.targetDirAngle;
      em.directionSpread = 0.0f;
      return freq;
    };
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float posMax = min(1.0f, ctx.animTime * 10.0f);
      float posMin = clamp(ctx.animTime * 10.0f - 0.5f, 0.0f, posMax * 0.5f);
      float& pos = em.animationVars[0];
      pinst.pos += ctx.ps.targetDir * 12.0f + ctx.ps.targetOffset * pos;
      pos += 0.04f;
      pinst.rot = ctx.ps.targetDirAngle;
      pinst.size *= (0.8f + pos * 0.4f);
      if (pos > 1.0f)
        pinst.life = pinst.maxLife;
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::AIR_BLAST, psdef);
}

static void addSandDustEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.source = FRect(-6, -6, 6, 6);
  edef.setStrengthSpread(17.5f, 2.5f);
  edef.setDirectionSpread(0.0f, 0.0f);
  edef.frequency = 30.0f;

  ParticleDef pdef;
  pdef.life = 0.4f;
  pdef.size = {{0.0f, 0.1f, 1.0f}, {5.0f, 10.0f, 14.0f}, InterpType::quadratic};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.2f, 0.3f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};
  pdef.textureName = TextureName::CLOUDS_SOFT;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.2f);
  ssdef.maxTotalParticles = 6;

  ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      auto temp = pinst;
      temp.pos = rotateVector(temp.pos, ctx.ps.targetDirAngle) + FVec2(0, 3.5f);
      temp.size = FVec2(1.2f, 0.6f);
      temp.rot = 0.0f;
      return defaultDrawParticle(ctx, temp, out);
    };
  ssdef.layer = Layer::back;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};

  mgr.addDef(FXName::SAND_DUST, psdef);
}

static void addWaterSplashEffect(FXManager &mgr) {
  // TODO: this effect shouldn't cover the creature too much; it should mostly be visible around it
  EmitterDef edef;
  edef.setStrengthSpread(10.0f, 5.0f);
  edef.frequency = 200.0f;
  edef.source = FRect(-6, 4, 6, 12);

  ParticleDef pdef;
  pdef.life = 0.4f;
  pdef.size = {{0.0f, 0.5f, 1.0f}, {3.0f, 4.0f, 5.0f}, InterpType::quadratic};
  pdef.alpha = {{0.0f, 0.25f, 0.75f, 1.0f}, {0.0f, 0.2f, 0.2f, 0.0f}};
  pdef.textureName = TextureName::WATER_DROPS;

  SubSystemDef ssdef(pdef, edef, 0.07f, 0.27f);
  ssdef.maxTotalParticles = 30;

  ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
    defaultAnimateParticle(ctx, pinst);
    pinst.pos += FVec2(0.0f, -cos(pinst.life * 6.0f) * 30.0f * ctx.timeDelta);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  mgr.addDef(FXName::WATER_SPLASH, psdef);
}


static const array<FColor, 3> magicMissileColors {{
  {0.3f, 0.5f, 0.7f}, {0.3f, 0.5f, 0.6f}, {0.3f, 0.5f, 0.7f},
}};

static void addOldMagicMissileEffect(FXManager& mgr) {
  // Każda cząsteczka z czasem z grubsza liniowo przechodzi od źródła do celu
  // dodatkowo może być delikatnie przesunięta z głównego toru

  constexpr float flightTime = 0.3f;

  // Te efekty są zbyt łagodne; nie sprawjają wrażenia jakby robiły jakikolwiek damage

  ParticleSystemDef psdef;
  { // Base system, only a source for other particles
    EmitterDef edef;
    edef.setStrengthSpread(15.0f, 7.0f);
    edef.initialSpawnCount = 1.0f;
    Curve<float> bendFlight = {{0.0f, 0.1f, 0.3f, 1.0f}, {0.0f, 0.1f, 0.15f, 0.0f}, InterpType::cosine};
    edef.scalarCurves.emplace_back(bendFlight);

    ParticleDef pdef;
    pdef.life = flightTime;
    pdef.slowdown = 1.0f;
    pdef.alpha = {{0.0f, 0.5f, 0.0f}};
    pdef.size = {{0.0f, 0.2f, 0.8f, 1.0f}, {5.0f, 20.0f, 30.0f, 20.0f}};
    pdef.color = {FVec3(0.2, 0.1, 0.9)};
    pdef.textureName = TextureName::MISSILE_CORE;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.5f);
    ssdef.maxTotalParticles = 1;

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      em.animationVars[1] = (em.strength + ctx.uniformSpread(em.strengthSpread)) * (Random.get(2) * 2 - 1);
    };

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);

      auto ptime = min(pinst.particleTime(), 1.0f);
      auto newPos = lerp(FVec2(), ctx.ps.targetOffset, ptime);
      float bendStrength = ctx.ssdef.emitter.scalarCurves[0].sample(ptime) * ctx.ps.targetTileDist;
      auto* vars = ctx.ss.animationVars;
      newPos += rotateVector(ctx.ps.targetDir, fconstant::pi * 0.5f) * vars[1] * bendStrength;
      auto moveVec = newPos - pinst.pos;
      float moveLen = length(moveVec);
      pinst.rot = vectorToAngle(moveVec / moveLen);
      pinst.size = FVec2(moveLen * 0.3f, 1.0f);
      pinst.pos = newPos;
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  { // Trace particles
    EmitterDef edef;
    edef.strength = 20.0f;
    edef.frequency = 30.0f;
    edef.directionSpread = fconstant::pi;
    edef.rotSpeed = .1f;

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = {{8.0f, 10.0f}};
    pdef.alpha = {{0.0f, 0.1f, 0.7f, 1.0f}, {0.0, 0.7f, 0.7f, 0.0}};
    pdef.slowdown = 1.0f;
    pdef.textureName = TextureName::SPARKS;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.5f);

    ssdef.prepareFunc = [](AnimationContext &ctx, EmissionState &em) {
      auto ret = defaultPrepareEmission(ctx, em);
      auto &parts = ctx.ps.subSystems[0].particles;
      if (parts.empty())
        return 0.0f;
      em.strength *= 1.2f - parts.front().particleTime();
      float mod = 1.0f + ctx.ps.params.scalar[0];
      return ret * ctx.ps.targetTileDist * mod;
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      auto& parts = ctx.ps.subSystems[0].particles;
      if (!parts.empty())
        pinst.pos += parts.front().pos;
    };
    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      if (!defaultDrawParticle(ctx, pinst, out))
        return false;
      out.color = Color(FColor(out.color) * choose(magicMissileColors, pinst.randomSeed));
      return true;
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::MAGIC_MISSILE_OLD, psdef);
}

static FVec2 missileBasePos(AnimationContext& ctx, float offset = 0.3f) {
  auto& baseParts = ctx.ps.subSystems[0].particles;
  if (!baseParts.empty()) {
    auto& pinst = baseParts[0];
    auto& pdef = ctx.psdef.subSystems[0].particle;
    float pos = clamp(pinst.particleTime(), 0.0f, 1.0f);
    float size = pdef.size.sample(pos) * pinst.size.y;
    FVec2 vec = angleToVector(pinst.rot + fconstant::pi * 0.5f);
    return pinst.pos + vec * size * offset;
  }
  return FVec2();
};

static void addMagicMissileEffect(FXManager& mgr) {
  constexpr float flightTime = 0.25f;
  // TODO: spell power should affect missile size
  //
  FColor color = Color(184, 234, 50);

  ParticleSystemDef psdef;
  { // Blurred trail
    EmitterDef edef;
    edef.initialSpawnCount = 1.0f;

    ParticleDef pdef;
    pdef.life = flightTime;
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 0.5f, 0.5f, 0.0f}};
    pdef.size = 20.0f;
    pdef.color = color.rgb() * FVec3(0.5f, 1.0f, 3.0f);
    pdef.textureName = TextureName::MISSILE_CORE;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      em.direction = ctx.ps.targetDirAngle;
      em.directionSpread = 0.0f;
      em.strength = ctx.ps.targetTileDist * 24.0f / flightTime;
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = ctx.ps.targetDirAngle - fconstant::pi * 0.5f;
      pinst.size = FVec2(1.0f, 0.0f);
    };

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);
      float ptime = pinst.particleTime();
      float max_width = ctx.ps.targetTileDist;
      float shear = ptime < 0.25f ? ptime * 4.0f : ptime > 0.75f ? (1.0f - ptime) * 4.0f : 1.0f;
      pinst.size = FVec2(1.0f, max_width * shear);
    };
    psdef.subSystems.emplace_back(ssdef);
  }
  { // Missile core
    EmitterDef edef;
    edef.initialSpawnCount = 3.0f;

    ParticleDef pdef;
    pdef.life = flightTime;
    pdef.alpha = {{0.0f, 0.2f, 0.9f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.size = {{0.0f, 0.7f, 1.0f}, {30.0f, 30.0f, 50.0f}};
    pdef.color = color.rgb();
    pdef.textureName = TextureName::FLASH1;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      em.direction = ctx.ps.targetDirAngle;
      em.directionSpread = 0.0f;
      defaultEmitParticle(ctx, em, pinst);
      pinst.rotSpeed = ctx.uniform(5.0f, 10.0f);
      pinst.pos = missileBasePos(ctx);
    };

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);
      pinst.pos = missileBasePos(ctx);
    };

    ssdef.multiDrawFunc = [](DrawContext& ctx, const Particle& pinst, vector<DrawParticle>& out, Color color) {
      DrawParticle dpart;
      if (!defaultDrawParticle(ctx, pinst, dpart))
        return;
      dpart.color = dpart.color.blend(color);
      out.push_back(dpart);

      dpart.texName = TextureName::FLASH1_GLOW;
      dpart.color = (Color)FColor(FColor(dpart.color).rgb() + FVec3(0.3f));
      dpart.color = dpart.color.blend(color);
      out.push_back(std::move(dpart));
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::MAGIC_MISSILE, psdef);
}

static void addMagicMissileSplashEffect(FXManager& mgr) {
  // TODO: spell power should affect size
  FColor color = Color(184, 224, 80);

  ParticleSystemDef psdef;
  {
    EmitterDef edef;
    edef.initialSpawnCount = 3.0f;

    ParticleDef pdef;
    pdef.life = 0.3f;
    pdef.alpha = {{0.0f, 0.2f, 0.6f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.size = 35.0f;
    pdef.color = color.rgb();
    pdef.textureName = TextureName::FLASH1;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);

    ssdef.multiDrawFunc = [](DrawContext& ctx, const Particle& pinst, vector<DrawParticle>& out, Color color) {
      DrawParticle dpart;
      if (!defaultDrawParticle(ctx, pinst, dpart))
        return;
      dpart.color = dpart.color.blend(color);
      out.push_back(dpart);

      dpart.texName = TextureName::FLASH1_GLOW;
      dpart.color = (Color)FColor(FColor(dpart.color).rgb() + FVec3(0.3f));
      dpart.color = dpart.color.blend(color);
      out.push_back(std::move(dpart));
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::MAGIC_MISSILE_SPLASH, psdef);
}

static void addFireEffect(FXManager& mgr) {
  ParticleSystemDef psdef;

  { // Glow
    EmitterDef edef;
    edef.frequency = 2.0f;
    edef.initialSpawnCount = 1.0f;
    edef.source = FVec2(0, 2);

    ParticleDef pdef;
    pdef.life = 1.0f;
    pdef.size = 32.0f;

    pdef.color = {{rgb(Color(185, 155, 100)), rgb(Color(195, 135, 90))}};
    pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0f, 0.5f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR_STRONG;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.size = FVec2(1.0f + 0.5 * ctx.ps.params.scalar[0]);
      pinst.rot = 0.0f;
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Fire
    EmitterDef edef;
    edef.strength = 20.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
    edef.frequency = 20.0f;
    edef.source = FRect(-4, 6, 4, 12);
    edef.rotSpeed = 0.05f;

    ParticleDef pdef;
    pdef.life = 0.7f;
    pdef.size = 8.0f;

    pdef.color = {{rgb(Color(155, 85, 30)), rgb(Color(155, 85, 30))}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.textureName = TextureName::FLAMES;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
      float freq = defaultPrepareEmission(ctx, em);
      float mod = ctx.ps.params.scalar[0];
      return freq * (1.0f + mod * 2.0f);
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float mod = 0;//ctx.ps.params.scalar[0];
      pinst.pos.x *= (1.0f + mod);
      pinst.movement *= (1.0f + mod);
      pinst.size *= (1.0f + mod * 0.25f);
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  { // Smoke
    EmitterDef edef;
    edef.strength = 20.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
    edef.frequency = 20.0f;
    edef.source = FRect(-4, -10, 4, -4);
    edef.rotSpeed = 0.05f;

    ParticleDef pdef;
    pdef.life = 0.7f;
    pdef.size = 12.0f;
    pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 0.15, 0.0}};

    pdef.color = {{0.0f, 0.5f, 1.0f}, {FVec3(0.0f), FVec3(0.3f), FVec3(0.0f)}};
    pdef.textureName = TextureName::CLOUDS_SOFT_BORDERS;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
      float freq = defaultPrepareEmission(ctx, em);
      float mod = ctx.ps.params.scalar[0];
      return freq * (1.0f + mod * 2.0f);
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float mod = ctx.ps.params.scalar[0];
      pinst.pos.x *= (1.0f + mod);
      pinst.pos.y -= mod * 6.0f;
      pinst.movement *= (1.0f + mod);
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::FIRE, psdef);
  mgr.genSnapshots(FXName::FIRE, {1.0f, 1.2f, 1.4f}, {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f}, 2);
}

static void addEngineSteamEffect(FXManager& mgr) {
  ParticleSystemDef psdef;
  EmitterDef edef;
  edef.strength = 20.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
  edef.frequency = 15.0f;
  edef.source = FRect(0, -13, 2, -11);
  edef.rotSpeed = 0.05f;

  ParticleDef pdef;
  pdef.life = 0.5f;
  pdef.size = 12.0f;
  pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 0.6, 0.05}};

  pdef.color = {{0.0f, 0.5f, 1.0f}, {FVec3(1.0f), FVec3(0.7f), FVec3(1.0f)}};
  pdef.textureName = TextureName::CLOUDS_SOFT_BORDERS;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
    float freq = defaultPrepareEmission(ctx, em);
    float mod = ctx.ps.params.scalar[0];
    return freq * (1.0f + mod * 2.0f);
  };

  ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
    defaultEmitParticle(ctx, em, pinst);
    float mod = ctx.ps.params.scalar[0];
    pinst.pos.x *= (1.0f + mod);
    pinst.pos.y -= mod * 6.0f;
    pinst.movement *= (1.0f + mod);
  };
  psdef.subSystems.emplace_back(ssdef);
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::ENGINE_STEAM, psdef);
  mgr.genSnapshots(FXName::ENGINE_STEAM, {1.0f, 1.2f, 1.4f}, {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f}, 2);
}

static void addMagmaFireEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.setStrengthSpread(10.0f, 5.0f);
  edef.frequency = 20.0f;
  edef.source = FRect(-6, 4, 6, 12);

  ParticleDef pdef;
  pdef.life = 0.8f;
  pdef.size = {{0.0f, 0.5f, 1.0f}, {3.0f, 4.0f, 5.0f}, InterpType::quadratic};
  //pdef.alpha = {{0.0f, 0.25f, 0.75f, 1.0f}, {0.0f, 0.8f, 0.8f, 0.0f}};
  pdef.alpha = {{1.0f, 0.0f}};
  pdef.color = {{rgb(Color::ORANGE), rgb(Color::ORANGE)}};
  pdef.textureName = TextureName::WATER_DROPS;

  SubSystemDef ssdef(pdef, edef, 0.07f, 0.27f);

  ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
    defaultAnimateParticle(ctx, pinst);
    pinst.pos += 0.5f * FVec2(0.0f, -cos(pinst.life * 1.0f) * 30.0f * ctx.timeDelta);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.animLength = 5.0f;
  psdef.isLooped = true;
  psdef.randomOffset = true;
  mgr.addDef(FXName::MAGMA_FIRE, psdef);
}

static void addFireSphereEffect(FXManager& mgr) {
  ParticleSystemDef psdef;

  { // Fire
    EmitterDef edef;
    edef.strength = 20.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
    edef.frequency = 40.0f;
    edef.source = FRect(-4, 6, 4, 12);
    edef.rotSpeed = 0.05f;

    ParticleDef pdef;
    pdef.life = 0.7f;
    pdef.size = 8.0f;

    pdef.color = {{rgb(Color(185, 85, 30)), rgb(Color(95, 35, 60))}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.textureName = TextureName::FLAMES;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Glow
    EmitterDef edef;
    edef.frequency = 10.0f;
    edef.source = FVec2(0, 2);

    ParticleDef pdef;
    pdef.life = 0.7f;
    pdef.size = 26.0f;

    pdef.color = {{rgb(Color(185, 85, 30)), rgb(Color(95, 35, 60))}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 0.3f, 0.3f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR_STRONG;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.size = FVec2(1.0f, 1.3f);
      pinst.rot = 0.0f;
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::FIRE_SPHERE, psdef);
  mgr.genSnapshots(FXName::FIRE_SPHERE, {1.0f, 1.2f, 1.4f}, {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f}, 2);
}

static void addFireballEffect(FXManager& mgr) {
  ParticleSystemDef psdef;

  // TODO: this should depend on vector length ?
  static constexpr float flightTime = 0.3f;
  Curve<float> movementCurve = {{0.0f, 0.0f, 0.2f, 0.6f, 1.0f}, InterpType::cubic};

  // This function is only responsible for proper positioning
  auto drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
    auto temp = pinst;
    float flightPos = min(ctx.ps.animTime / flightTime, 1.0f);
    auto& movementCurve = ctx.psdef.subSystems[0].particle.scalarCurves[0];
    temp.pos += movementCurve.sample(flightPos) * ctx.ps.targetOffset;
    return defaultDrawParticle(ctx, temp, out);
  };

  { // Flying ball of fire
    EmitterDef edef;
    edef.strength = 15.0f;
    edef.frequency = 80.0f;
    edef.source = FVec2(0.0f);
    edef.rotSpeed = {{0.3f, 0.1f}};
    edef.initialSpawnCount = 5;

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = 12.0f;
    pdef.color = {{rgb(Color(155, 85, 30)), rgb(Color(45, 35, 30))}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.textureName = TextureName::FLAMES_BLURRED;
    pdef.scalarCurves.emplace_back(movementCurve);

    SubSystemDef ssdef(pdef, edef, 0.0f, flightTime);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
      float freq = defaultPrepareEmission(ctx, em);
      float mod = ctx.ps.params.scalar[0];
      return freq * (1.0f + mod * 2.0f);
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float mod = ctx.ps.params.scalar[0];
      pinst.pos.x *= (1.0f + mod);
      pinst.movement *= (1.0f + mod);
      pinst.size *= (1.0f + mod * 0.25f);
    };

    ssdef.drawFunc = drawFunc;
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Glow
    EmitterDef edef;
    edef.initialSpawnCount = 1.0f;
    edef.source = FVec2(0, 2);

    ParticleDef pdef;
    pdef.life = flightTime + 0.3f;
    pdef.size = 70.0f;

    pdef.color = {{rgb(Color(185, 155, 100)), rgb(Color(195, 135, 90))}};
    pdef.alpha = {{0.0f, 0.2f, 0.7f, 1.0f}, {0.0f, 0.8f, 0.8f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.size = FVec2(1.0f + 0.5 * ctx.ps.params.scalar[0]);
    };

    ssdef.drawFunc = drawFunc;
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Smoke
    EmitterDef edef;
    edef.strength = 10.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
    edef.frequency = 30.0f;
    edef.source = FRect(-3, 2, 3, 6);
    edef.rotSpeed = 0.05f;

    ParticleDef pdef;
    pdef.life = 0.7f;
    pdef.size = {{15.0f, 30.0f}};
    pdef.alpha = {{0.0f, 0.1f, 1.0f}, {0.0f, 0.3f, 0.0f}, InterpType::cosine};

    pdef.color = {{0.0f, 0.5f, 1.0f}, {FVec3(0.0f), FVec3(0.3f), FVec3(0.0f)}};
    pdef.textureName = TextureName::CLOUDS_SOFT_BORDERS;
    pdef.scalarCurves.emplace_back(movementCurve);

    SubSystemDef ssdef(pdef, edef, 0.0f, flightTime);
    ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
      float freq = defaultPrepareEmission(ctx, em);
      float mod = ctx.ps.params.scalar[0];
      return freq * (1.0f + mod * 2.0f);
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      float flightPos = min(ctx.ps.animTime / flightTime, 1.0f);
      float mod = ctx.ps.params.scalar[0];
      pinst.pos += ctx.pdef.scalarCurves[0].sample(flightPos) * ctx.ps.targetOffset;
      pinst.movement *= (1.0f + mod);
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::FIREBALL, psdef);
}

static void addFlamethrowerEffect(FXManager& mgr, FXName fxName, Color flameColor1, Color flameColor2, Color glowColor) {
  ParticleSystemDef psdef;

  auto emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
    defaultEmitParticle(ctx, em, pinst);
    float mod = ctx.ps.params.scalar[0];
    pinst.size *= (1.0f + mod * 0.25f);
  };

  auto prepFunc = [](AnimationContext& ctx, EmissionState& em) {
    float freq = defaultPrepareEmission(ctx, em);
    em.direction = ctx.ps.targetDirAngle;
    em.directionSpread /= ctx.ps.targetTileDist;
    em.strength *= ctx.ps.targetTileDist;
    freq *= ctx.ps.targetTileDist;
    return freq;
  };

  { // Flames
    EmitterDef edef;
    edef.strength = 50.0f;
    edef.setDirectionSpread(0.0f, 0.3f);
    edef.frequency = 40.0f;
    edef.source = FVec2(0.0f);

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = 12.0f;
    pdef.color = {{rgb(flameColor1), rgb(flameColor2)}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}};
    pdef.textureName = TextureName::FLAMES_BLURRED;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.7f);
    ssdef.prepareFunc = prepFunc;
    ssdef.emitFunc = emitFunc;
    psdef.subSystems.emplace_back(ssdef);
  }
  { // Glow
    EmitterDef edef;
    edef.strength = 50.0f;
    edef.setDirectionSpread(0.0f, 0.3f);
    edef.frequency = 8.0f;
    edef.source = FVec2(0.0f);

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = 55.0f;

    pdef.color = rgb(glowColor);
    pdef.alpha = {{0.0f, 0.2f, 0.7f, 1.0f}, {0.0f, 0.5f, 0.5f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.65f);
    ssdef.prepareFunc = prepFunc;
    ssdef.emitFunc = emitFunc;
    ssdef.layer = Layer::back;
    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(fxName, psdef);
}

static void addSleepEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.strength = 10.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
  edef.frequency = 1.2f;
  edef.source = FRect(-2, -12, 2, -9);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.2f, 1.0f}, {0.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = FVec3(1.0f);
  pdef.textureName = TextureName::SPECIAL;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {0, 0};
    pinst.rot = Random.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::SLEEP, psdef);
  mgr.genSnapshots(FXName::SLEEP, {2.0f, 2.2f, 2.4f, 2.6f, 2.8f});
}

static void addLoveEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.strength = 10.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
  edef.frequency = 1.2f;
  edef.source = FRect(-2, -12, 2, -9);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.2f, 1.0f}, {0.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = FVec3(1.0f);
  pdef.textureName = TextureName::SPECIAL;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.pos += ctx.ps.targetDir * Renderer::nominalSize / 2;
    pinst.texTile = {2, 1};
    pinst.rot = Random.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = false;
  psdef.animLength = 3.0f;

  mgr.addDef(FXName::LOVE, psdef);
}

static void addLoopedLoveEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.strength = 10.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
  edef.frequency = 1.2f;
  edef.source = FRect(-6, 2, 6, 6);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.2f, 1.0f}, {0.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = FVec3(1.0f);
  pdef.textureName = TextureName::SPECIAL;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {2, 1};
    pinst.rot = Random.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = true;
  psdef.animLength = 3.0f;

  mgr.addDef(FXName::LOVE_LOOPED, psdef);
}

static void addBlindEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength = 0.0f;
  edef.frequency = 2.0f;
  edef.initialSpawnCount = 1.0f;
  edef.source = FRect(-8, -12, 8, -6);

  // TODO: cząsteczki które nie giną ?
  // TODO: może dodać tutaj jeszcze poświatę pod spód ?
  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = {{0.0f, 0.3f, 0.8f, 1.0f}, {8.0f, 10.0f, 10.0f, 25.0f}};
  pdef.alpha = {{0.0f, 0.3f, 0.8f, 1.0f}, {0.0f, 1.0, 0.5, 0.0}, InterpType::cosine};

  pdef.color = rgb(Color(253, 247, 172));
  pdef.textureName = TextureName::SPECIAL;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {1, 0};
    pinst.rot = Random.getDouble(-0.2f, 0.2f);

    auto distFunc = [&](FVec2 pos) {
      float dist = fconstant::inf;
      for (auto &p : ctx.ss.particles)
        dist = min(dist, distanceSq(p.pos, pos));
      return dist;
    };

    float bestDist = distFunc(pinst.pos);

    // We're trying to make sure that particles are far away from each other
    for (int n = 0; n < 10; n++) {
      FVec2 newPos = ctx.edef.source.sample(Random);
      float dist = distFunc(newPos);
      if (dist > bestDist) {
        bestDist = dist;
        pinst.pos = newPos;
      }
    }
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::BLIND, psdef);
  mgr.genSnapshots(FXName::BLIND, {2.0f, 2.2f, 2.4f, 2.6f, 2.8f});
}

static void addLaboratoryEffect(FXManager& mgr) {
  ParticleSystemDef psdef;

  { // Glow
    EmitterDef edef;
    edef.initialSpawnCount = 1.0f;
    edef.source = FVec2(0, -10.0f);

    ParticleDef pdef;
    pdef.life = 2.0f;
    pdef.size = 40.0f;
    pdef.alpha = {{0.0f, 0.3f, 0.7f, 1.0f}, {0.0f, 0.3f, 0.3f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Smoke & bubbles
    EmitterDef edef;
    edef.strength = 10.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.5f);
    edef.frequency = 4.0f;
    edef.source = FRect(-3.0f, -9.0f, 3.0f, -7.0f);

    ParticleDef pdef;
    pdef.life = 1.5f;
    pdef.size = {{12.0f, 12.0f, 30.0f}};
    pdef.alpha = {{0.0f, 0.2f, 0.6f, 1.0f}, {0.0, 0.5f, 0.2, 0.0}, InterpType::cosine};
    pdef.color = FVec3(0.5);
    pdef.textureName = TextureName::FLAKES_BORDERS;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.6f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
    };

    psdef.subSystems.emplace_back(ssdef);

    ssdef.particle.alpha = {{0.0f, 0.2f, 0.6f, 1.0f}, {0.0, 1.0f, 0.5, 0.0}, InterpType::cosine};
    ssdef.particle.textureName = TextureName::BUBBLES;
    ssdef.particle.size = {{5.0f, 10.0f, 12.0f}};
    ssdef.particle.color = FVec3(1.0);
    ssdef.emitter.frequency = 12.0f;
    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::LABORATORY, psdef);
}

static void addForgeEffect(FXManager& mgr) {
  ParticleSystemDef psdef;
  { // Glow
    EmitterDef edef;
    edef.initialSpawnCount = 1.0f;
    edef.source = FVec2(0, -6.0f);

    ParticleDef pdef;
    pdef.life = 0.3f;
    pdef.size = 40.0f;
    pdef.alpha = {{0.0f, 1.0f}, {0.3f, 0.0f}};
    pdef.textureName = TextureName::CIRCULAR_STRONG;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    psdef.subSystems.emplace_back(ssdef);
  }
  { // Sparks
    EmitterDef edef;
    edef.setStrengthSpread(40.0f, 20.0f);
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 1.0f);
    edef.source = FRect(-4.0f, -4.0f, 4.0f, -2.0f);
    edef.initialSpawnCount = 12.0f;

    ParticleDef pdef;
    pdef.life = 0.6f;
    pdef.size = 1.5f;
    pdef.alpha = {{0.0f, 0.4f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

    pdef.textureName = TextureName::SPARKS_LIGHT;

    SubSystemDef ssdef(pdef, edef, 0.0f, 0.03f);

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);
      pinst.movement += FVec2(0.0f, 100.0f) * ctx.timeDelta;
    };
    psdef.subSystems.emplace_back(ssdef);
  }


  mgr.addDef(FXName::FORGE, psdef);
}

static void addWorkshopEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.setStrengthSpread(50.0f, 20.0f);
  edef.rotSpeed = 0.5f;
  edef.frequency = 999.0f;
  edef.source = FRect(-2.0f, -6.0f, 2.0f, -4.0f);

  ParticleDef pdef;
  pdef.life = 0.7f;
  pdef.size = 5.0f;
  pdef.slowdown = {{0.0f, 0.1f}, {5.0f, 1000.0f}};
  pdef.alpha = {{0.0f, 0.6f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

  FColor brown(Color(120, 87, 46));
  // Kiedy cząsteczki opadną pod drzewo, robią się w zasięgu cienia
  // TODO: lepiej rysować je po prostu pod cieniem
  pdef.color = {{0.0f, 0.3f, 0.4}, {brown.rgb(), brown.rgb(), brown.rgb() * 0.8f}};
  pdef.textureName = TextureName::FLAKES_BORDERS;

  SubSystemDef ssdef(pdef, edef, 0.0f, 0.1f);
  ssdef.maxTotalParticles = 7;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  mgr.addDef(FXName::WORKSHOP, psdef);
}

static void addJewelerEffect(FXManager& mgr) {
  ParticleSystemDef psdef;
  { // Sparks
    EmitterDef edef;
    edef.strength = 0.0f;
    edef.frequency = 2.0f;
    edef.source = FRect(-8, -11, 8, -3);

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = 3.5f;
    pdef.alpha = {{0.0f, 0.3f, 0.8f, 1.0f}, {0.0f, 1.0, 0.7, 0.0}, InterpType::cosine};
    pdef.textureName = TextureName::SPARKS_LIGHT;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.5f);

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rotSpeed = 1.5f;
    };

    ssdef.multiDrawFunc = [](DrawContext& ctx, const Particle& pinst, vector<DrawParticle>& out, Color color) {
      DrawParticle dpart;
      if (!defaultDrawParticle(ctx, pinst, dpart))
        return;
      dpart.color = dpart.color.blend(color);
      out.push_back(dpart);

      // Glow particles:
      dpart.texName = TextureName::CIRCULAR;
      dpart.texCoords = ctx.texQuadCorners({}, FVec2(1));
      dpart.color.a /= 4;
      auto center = pinst.pos + ctx.ps.pos;
      for (auto& pos : dpart.positions)
        pos = (pos - center) * 12.0f + center;
      out.push_back(std::move(dpart));
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::JEWELLER, psdef);
}

static void addSpiralEffects(FXManager& mgr) {
  ParticleSystemDef psdef;
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  {
    EmitterDef edef;
    edef.frequency = 10.0f;
    edef.initialSpawnCount = 1.0f;
    edef.setDirectionSpread(fconstant::pi * 0.5f, 0.0f);
    edef.strength = 2.5f;
    edef.source = FVec2(0.0f, -4.0f);

    ParticleDef pdef;
    pdef.life = 4.0f;
    pdef.size = 10.0f;
    pdef.alpha = {{0.0f, 0.1f, 0.8f, 1.0f}, {0.0, 0.7f, 0.7f, 0.0}};

    pdef.textureName = TextureName::CIRCULAR;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
    };
    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      Particle temp(pinst);
      FVec2 circlePos = angleToVector(10.0f - pinst.life * 3.0f);
      temp.pos += circlePos * FVec2(10.0f, 4.0f);
      if (!defaultDrawParticle(ctx, temp, out))
        return false;
      float alphaMul = dot(circlePos, FVec2(0.0f, 1.0f));
      float alpha = float(out.color.a) * clamp(alphaMul + 0.3f, 0.0f, 1.0f);
      out.color.a = (unsigned char)(alpha);
      return out.color.a > 0;
    };

    psdef.subSystems = {ssdef};
    mgr.addDef(FXName::SPIRAL, psdef);

    ssdef.particle.textureName = TextureName::CIRCULAR_STRONG;
    ssdef.particle.size = 5.0f;
    ssdef.emitter.frequency = 5.0f;
    psdef.subSystems = {ssdef};
    mgr.addDef(FXName::SPIRAL2, psdef);
  }

  mgr.genSnapshots(FXName::SPIRAL, {4.0f, 4.2f, 4.4f, 4.6f, 4.8f});
  mgr.genSnapshots(FXName::SPIRAL2, {4.0f, 4.2f, 4.4f, 4.6f, 4.8f});
}

static void addBuffEffect(FXManager& mgr) {
  static constexpr int numParticles = 2;
  EmitterDef edef;
  edef.initialSpawnCount = numParticles;

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.1f, 0.8f, 1.0f}, {0.0, 0.7f, 0.7f, 0.0}};

  pdef.color = FVec3(1.0f);
  pdef.textureName = TextureName::CIRCULAR;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
    defaultAnimateParticle(ctx, pinst);

    int idx = &pinst - ctx.ss.particles.data();
    float pos = ctx.globalTime * 2.0f + double(idx) * fconstant::pi * 2.0f / numParticles;

    float width = 10.0f + 3.0f * ctx.ps.params.scalar[0];
    float hoffset = 2 * fconstant::pi * ctx.ps.params.scalar[1] / numParticles;
    float voffset = 0.25;
    /*float hoffset = 0.0f;
    if (voffset >= 0.5f) {
      voffset -= 0.5f;
      hoffset = 0.5f;
    }
    hoffset += voffset * 0.5f;*/

    auto vec = angleToVector(pos + hoffset);
    pinst.temp = vec.y;
    pinst.pos = vec * FVec2(width, 6.0f) - FVec2(0.0f, voffset * 16.0f) + FVec2(0.0f, 2.0f);
    if (!ctx.ps.isDying)
      pinst.life = min(pinst.life, 0.5f);
  };

  ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
    if (!defaultDrawParticle(ctx, pinst, out))
      return false;
    float alpha = float(out.color.a) * clamp(pinst.temp + 0.5f, 0.0f, 1.0f);
    out.color.a = (unsigned char)(alpha);
    return out.color.a > 0;
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::BUFF, psdef);
}

static void addFlyingEffect(FXManager& mgr) {
  ParticleSystemDef psdef;
  {
    EmitterDef edef;
    edef.frequency = 2.0f;
    edef.strength = 24.0f;
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.f);
    edef.source = FVec2(0.0f, 10.0f);

    ParticleDef pdef;
    pdef.life = 0.8f;
    pdef.size = {{0.0f, 1.0f}, {32.0f, 24.0f}, InterpType::cosine};
    pdef.alpha = {{0.0f, 0.25f, 0.7, 1.0f}, {0.0, 0.6, 0.2, 0.0}, InterpType::cosine};
    pdef.color = FVec3(1.0f);
    pdef.textureName = TextureName::TORUS_BOTTOM;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  {
    EmitterDef edef;
    edef.frequency = 1.0f;
    edef.source = FVec2(0.0f, 4.0f);
    edef.initialSpawnCount = 1.0f;

    ParticleDef pdef;
    pdef.life = 1.5f;
    pdef.size = 24.0f;
    pdef.alpha = {{0.0f, 0.33333f, 0.6666f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::cosine};
    pdef.color = FVec3(0.6f, 0.8f, 1.0f);
    pdef.textureName = TextureName::TORUS_BOTTOM_BLURRED;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
    };

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);
      if (!ctx.ps.isDying)
        pinst.life = min(pinst.life, 1.0f);
    };
    ssdef.maxTotalParticles = 1;
    psdef.subSystems.emplace_back(ssdef);
  }

  psdef.isLooped = true;
  psdef.animLength = 1.0f;

  mgr.addDef(FXName::FLYING, psdef);
  mgr.genSnapshots(FXName::FLYING, {1.0f, 1.4f, 1.8f}, {}, 1);
}

static void addDebuffEffect(FXManager& mgr) {
  ParticleSystemDef psdef;
  {
    EmitterDef edef;
    edef.initialSpawnCount = 1;

    ParticleDef pdef;
    pdef.life = 1.2f;
    pdef.size = 28.0f;
    pdef.alpha = {{0.0f, 0.25f, 0.7, 1.0f}, {0.0, 0.6, 0.2, 0.0}, InterpType::cosine};
    pdef.textureName = TextureName::TORUS_BOTTOM;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
    };
    ssdef.maxActiveParticles = 1;

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);

      float offset = ctx.ps.params.scalar[1] * fconstant::pi * 2.0f;
      float pos = std::cos((ctx.globalTime) * 5.0f + offset);
      // Faster movement in the middle, slower on the edges:
      pos = (pos < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(pos), 0.5);
      // Constant offset:
      //pos +=  (ctx.ps.params.scalar[1] - 0.5f) * 3.0f;

      float width = 1.0f + 0.2f * ctx.ps.params.scalar[0];
      pinst.size = FVec2(width, 1.0f);
      pinst.pos = FVec2(0.0f, pos * 2.0f - 4.0f);
      if (!ctx.ps.isDying)
        pinst.life = min(pinst.life, 0.25f);
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::DEBUFF, psdef);
  mgr.genSnapshots(FXName::DEBUFF, {1.0f, 1.4f, 1.8f}, {}, 1);
}

static void addGlitteringEffect(FXManager& mgr, FXName name, double life, double animLength, double size) {
  EmitterDef edef;
  edef.source = FRect(-10, -10, 10, 10);

  ParticleDef pdef;
  pdef.life = life;
  pdef.size = size;
  pdef.alpha = {{0.0f, 0.3f, 0.8f, 1.0f}, {0.0f, 1.0, 0.7, 0.0}, InterpType::cosine};
  pdef.textureName = TextureName::SPARKS_LIGHT;

  SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
  ssdef.prepareFunc = [](AnimationContext& ctx, EmissionState& em) {
    defaultPrepareEmission(ctx, em);
    auto& var = em.animationVars[0];

    // Randomizing time until next particle
    if (ctx.animTime == 0.0f || var < 0.0)
      var = ctx.globalTime + ctx.uniform(0.5, 2.0);
    if (ctx.globalTime > var && var > 0.0) {
      var = -1.0;
      return 1.0f;
    }

    return 0.0f;
  };

  ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.rotSpeed = 1.5f;
  };

  ssdef.multiDrawFunc = [](DrawContext& ctx, const Particle& pinst, vector<DrawParticle>& out, Color color) {
    DrawParticle dpart;
    if (!defaultDrawParticle(ctx, pinst, dpart))
      return;
    dpart.color = dpart.color.blend(color);
    out.push_back(dpart);

    // Glow particles:
    dpart.texName = TextureName::CIRCULAR;
    dpart.texCoords = ctx.texQuadCorners({}, FVec2(1));
    dpart.color.a /= 4;
    auto center = pinst.pos + ctx.ps.pos;
    for (auto& pos : dpart.positions)
      pos = (pos - center) * 10.0f + center;
    out.push_back(std::move(dpart));
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.isLooped = true;
  psdef.animLength = animLength;

  mgr.addDef(name, psdef);
}

static void addTeleportEffects(FXManager& mgr) {
  ParticleSystemDef psdef;

  static constexpr float numParts = 40.0f;
  {
    EmitterDef edef;
    edef.initialSpawnCount = numParts;
    edef.source = FVec2(0, -24);

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = 40.0f;

    pdef.color = FVec3(0.5f, 0.5f, 0.75f);
    pdef.alpha = {{0.9f, 1.0f}, {1.0f, 0.0f}};
    pdef.textureName = TextureName::TELEPORT_BLEND;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      int index = em.animationVars[0]++;
      pinst.texTile = {0, short(index)};
      pinst.pos += float(index) * FVec2(0.0, 40.0f / numParts);
      pinst.size = FVec2(0.9f, 1.0f / numParts);
      pinst.rot = 0.0f;
      pinst.life += float(float(index) / numParts) * 0.3f;
    };

    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      if (!defaultDrawParticle(ctx, pinst, out))
        return false;
      out.texCoords = ctx.texQuadCorners(pinst.texTile, FVec2(1.0f, 1.0f / numParts));
      return true;
    };

    ssdef.maxActiveParticles = numParts;
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Glow
    EmitterDef edef;
    edef.initialSpawnCount = 2;
    edef.source = FVec2(0, -6);

    ParticleDef pdef;
    pdef.life = 0.4f;
    pdef.size = 62.0f;

    pdef.alpha = {{0.0f, 0.3f, 1.0f}, {0.8f, 0.6f, 0.0f}};
    pdef.textureName = TextureName::BEAMS;

    SubSystemDef ssdef(pdef, edef, 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  mgr.addDef(FXName::TELEPORT_OUT, psdef);

  psdef.subSystems[0].emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
    defaultEmitParticle(ctx, em, pinst);
    int index = em.animationVars[0]++;
    pinst.texTile = {0, short(index)};
    pinst.pos += float(index) * FVec2(0.0, 40.0f / numParts);
    pinst.size = FVec2(0.9f, 1.0f / numParts);
    pinst.rot = 0.0f;
    pinst.life += float(float(numParts - index) / numParts) * 0.3f;
  };

  mgr.addDef(FXName::TELEPORT_IN, psdef);

  psdef.subSystems[0].particle.color = FVec3(1.0);
  psdef.subSystems.pop_back();
  mgr.addDef(FXName::SPAWN, psdef);
}

void FXManager::initializeDefs() {
  addTestSimpleEffect(*this);
  addTestMultiEffect(*this);
  addWoodSplinters(*this);
  addRockSplinters(*this);
  addRockCloud(*this);
  addPoisonCloud(*this);
  addDestroyFurnitureEffect(*this);

  addRippleEffect(*this);
  addFireEffect(*this);
  addEngineSteamEffect(*this);
  addMagmaFireEffect(*this);
  addFireSphereEffect(*this);

  addSandDustEffect(*this);
  addWaterSplashEffect(*this);

  addCircularSpell(*this);
  addAirBlast(*this);
  addCircularBlast(*this);
  addOldMagicMissileEffect(*this);
  addMagicMissileEffect(*this);
  addMagicMissileSplashEffect(*this);
  addFireballEffect(*this);
  addFireballSplashEffect(*this);
  addFlamethrowerEffect(*this, FXName::FLAMETHROWER, Color(85, 85, 85), Color(35, 35, 35), Color(155, 155, 155));

  addSleepEffect(*this);
  addLoveEffect(*this);
  addLoopedLoveEffect(*this);
  addBlindEffect(*this);
  addGlitteringEffect(*this, FXName::GLITTERING, 0.5, 1.0, 3.5);
  addGlitteringEffect(*this, FXName::MAGIC_FIELD, 1.0, 1.0, 2.5);
  addTeleportEffects(*this);

  addLaboratoryEffect(*this);
  addForgeEffect(*this);
  addWorkshopEffect(*this);
  addJewelerEffect(*this);

  addSpiralEffects(*this);
  addFlyingEffect(*this);
  addDebuffEffect(*this);
  addBuffEffect(*this);
};
}
