#include "fx_manager.h"

#include "fx_color.h"
#include "fx_emitter_def.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"
#include "fx_rect.h"

namespace fx {

static const FVec2 dirVecs[8] = {
  {0.0, -1.0}, {0.0, 1.0},   {1.0, 0.0}, {-1.0, 0.0},
  {1.0, -1.0}, {-1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}};
FVec2 dirToVec(Dir dir) {
  return dirVecs[(int)dir];
}

template <class T, size_t N> const T& choose(const array<T, N>& values, uint randomSeed) {
  return values[randomSeed % N];
}

using SubSystemDef = ParticleSystemDef::SubSystem;

static void addTestSimpleEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength = 30.0f;
  edef.frequency = {{10.0f, 55.0f, 0.0f, 0.0}, InterpType::cosine};

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 32.0f;
  pdef.alpha = {{0.0f, 0.1, 0.8f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = {{{1.0f, 1.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {0.2f, 0.5f, 1.0f}}, InterpType::linear};
  pdef.texture = "circular.png";

  ParticleSystemDef psdef;
  psdef.subSystems.emplace_back(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 5.0f);
  psdef.name = "test_simple";
  mgr.addDef(psdef);
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
    pdefs[n].texture = "circular.png";
  }

  ParticleSystemDef psdef;
  psdef.name = "test_multi";
  for (int n = 0; n < 5; n++)
    psdef.subSystems.emplace_back(mgr.addDef(pdefs[n]), mgr.addDef(edef), float(n) * 0.5f, float(n) * 1.5f + 2.0f);
  mgr.addDef(psdef);
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

  FColor brown(IColor(120, 87, 46));
  // Kiedy cząsteczki opadną pod drzewo, robią się w zasięgu cienia
  // TODO: lepiej rysować je po prostu pod cieniem
  pdef.color = {{0.0f, 0.15f, 0.17}, {brown.rgb(), brown.rgb(), brown.rgb() * 0.6f}};
  pdef.texture = {"flakes_4x4_borders.png", 4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.maxTotalParticles = 7;
  ssdef.animateFunc = animateFunc;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "wood_splinters";
  mgr.addDef(psdef);
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
  pdef.texture = {"flakes_4x4_borders.png", 4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.maxTotalParticles = 5;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  // TODO: cząsteczki mogą mieć różny czas życia
  psdef.name = "rock_splinters";
  mgr.addDef(psdef);
}

static void addRockCloud(FXManager &mgr) {
  // Spawnujemy kilka chmurek w ramach kafla;
  // mogą być większe lub mniejsze
  //
  // czy zostawiają po sobie jakieś ślady?
  // może niech zostają ślady po splinterach, ale po chmurach nie?
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
  pdef.texture = {"clouds_soft_4x4.png", 4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);

  // TODO: różna liczba początkowych cząsteczek
  ssdef.maxTotalParticles = 5;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "rock_clouds";
  mgr.addDef(psdef);
}

static void addExplosionEffect(FXManager &mgr) {
  // TODO: tutaj trzeba zrobić tak, żeby cząsteczki które spawnują się później
  // zaczynały z innym kolorem
  EmitterDef edef;
  edef.strength = 5.0f;
  edef.frequency = 30.0f;

  ParticleDef pdef;
  pdef.life = 0.8f;
  pdef.size = {{5.0f, 30.0f}};

  IColor color(137, 106, 40);
  pdef.color = {{0.0f, 0.1f, 1.0f}, {FVec3(0.0f), color.rgb(), FVec3(0.0f)}};
  pdef.texture = {"clouds_grayscale_4x4.png", 4, 4};
  pdef.blendMode = BlendMode::additive;

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.3f);
  ssdef.maxTotalParticles = 20;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "explosion";
  mgr.addDef(psdef);
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
  pdef.texture = "torus.png";

  // TODO: możliwośc nie podawania czasu emisji (etedy emituje przez całą długośc animacji)?
  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.maxActiveParticles = 10;
  ssdef.prepareFunc = prepFunc;
  ssdef.animateFunc = animateFunc;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.animLength = 1.0f;
  psdef.isLooped = true;
  psdef.name = "ripple";

  mgr.addDef(psdef);
}

static void addCircularBlast(FXManager &mgr) {
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
  pdef.size = {{10.0f, 80.0f}, InterpType::cosine};
  pdef.alpha = {{0.0f, 0.03f, 0.2f, 1.0f}, {0.0f, 0.15f, 0.15f, 0.0f}, InterpType::cosine};

  pdef.color = {{0.5f, 0.8f}, {{1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 1.0f}}};
  pdef.texture = "torus.png";

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.maxActiveParticles = 20;
  ssdef.prepareFunc = prepFunc;
  ssdef.animateFunc = animateFunc;
  ssdef.emitFunc = emitFunc;

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "circular_blast";

  mgr.addDef(psdef);
}

static void addFeetDustEffect(FXManager &mgr) {
  // Ten efekt musi zaznaczać fakt, że postać się porusza w jakimś kierunku
  // To musi byc kontrolowalne za pomocą parametru
  // FX jest odpalany w momencie gdy postać wychodzi z kafla ?
  //
  // TODO: Parametrem efektu powinien być enum: kierunek ruchu
  // zależnie od tego mamy różne kierunki generacji i inny punkt startowy
  // drugim parametrem jest kolor (choć chyba będzie uzywany tylko na piasku?)

  EmitterDef edef;
  edef.source = FRect(-3, 3, 3, 4);
  edef.setStrengthSpread(17.5f, 2.5f);
  edef.frequency = 60.0f;

  ParticleDef pdef;
  pdef.life = 1.25f;
  pdef.size = {{0.0f, 0.1f, 1.0f}, {5.0f, 14.0f, 20.0f}, InterpType::quadratic};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.2f, 0.3f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};

  FVec3 start_color(0.9), end_color(0.7);
  pdef.color = {{start_color, end_color}};
  pdef.texture = {"clouds_soft_4x4.png", 4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.2f);
  // TODO: różna liczba początkowych cząsteczek
  ssdef.maxTotalParticles = 3;

  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    auto dvec = dirToVec(ctx.ps.params.dir[0]);
    defaultEmitParticle(ctx, em, pinst);
    pinst.pos -= dvec * 4.0f;
    pinst.rot = 0.0f;
    pinst.size = FVec2(1.2f, 0.6f);
  };
  ssdef.prepareFunc = [](AnimationContext &ctx, EmissionState &em) {
    auto ret = defaultPrepareEmission(ctx, em);
    auto vec = normalize(dirToVec(ctx.ps.params.dir[0]));
    em.direction = vectorToAngle(vec);
    em.directionSpread = 0.0f;
    return ret;
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "feet_dust";
  mgr.addDef(psdef);
}

static const array<FColor, 6> magicMissileColors {{
  {0.3f, 0.5f, 0.9f}, {0.9f, 0.7f, 0.2f}, {0.2f, 0.8f, 0.9f},
  {0.2f, 0.4, 0.3f}, {0.6f, 0.2f, 1.0f}, {0.2f, 0.1f, 0.5f}
}};

static void addMagicMissileEffect(FXManager& mgr) {
  // Każda cząsteczka z czasem z grubsza liniowo przechodzi od źródła do celu
  // dodatkowo może być delikatnie przesunięta z głównego toru

  constexpr float flightTime = 0.45f;
  constexpr float maxBaseAlpha = 0.4f;
  constexpr float maxFrequency = 200.0f;

  ParticleSystemDef psdef;
  { // Base system, also a source for other particles
    EmitterDef edef;
    edef.strength = 200.0f;
    edef.initialSpawnCount = 1.0f;
    edef.rotSpeed = 0.5f;

    ParticleDef pdef;
    pdef.life = flightTime;
    pdef.slowdown = 1.0f;
    pdef.alpha = {{0.0f, 0.2f}, {0.0f, maxBaseAlpha}};
    pdef.size = {{0.0f, 0.2f, 0.8f, 1.0f}, {5.0f, 20.0f, 30.0f, 20.0f}};
    pdef.texture = {"magic_missile_4x4.png", 4, 4};
    pdef.blendMode = BlendMode::additive;

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.5f);
    ssdef.maxTotalParticles = 1;

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.texTile = {0, 0};
    };

    ssdef.animateFunc = [](AnimationContext& ctx, Particle& pinst) {
      defaultAnimateParticle(ctx, pinst);
      double attract_value = max(0.0001, std::pow(min(1.0, 1.0 - pinst.particleTime()), 2.0));
      double mul = std::pow(0.001 * attract_value, ctx.timeDelta);
      pinst.movement *= mul;
      pinst.pos *= mul;
    };

    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      auto temp = pinst;
      temp.pos += temp.particleTime() * ctx.ps.targetOff;
      defaultDrawParticle(ctx, temp, out);
      out.color = (IColor)(FColor(out.color) * FColor(ctx.ps.params.color[1]));
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  { // Trace particles
    EmitterDef edef;
    edef.strength = 20.0f;
    edef.frequency = maxFrequency;
    edef.directionSpread = fconstant::pi;
    edef.rotSpeed = .1f;

    ParticleDef pdef;
    pdef.life = 0.5f;
    pdef.size = {{6.0f, 8.0f}};
    pdef.alpha = {{0.0f, 0.1f, 0.7f, 1.0f}, {0.0, 1.0f, 1.0f, 0.0}};
    pdef.slowdown = 1.0f;
    pdef.texture = {"magic_missile_4x4.png", 4, 4};
    pdef.blendMode = BlendMode::additive;

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.5f);

    ssdef.prepareFunc = [](AnimationContext &ctx, EmissionState &em) {
      auto ret = defaultPrepareEmission(ctx, em);
      auto &parts = ctx.ps.subSystems[0].particles;
      if (parts.empty())
        return 0.0f;
      em.strength *= 1.2f - parts.front().particleTime();
      return ret;
    };

    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      auto &parts = ctx.ps.subSystems[0].particles;
      if (!parts.empty()) {
        auto &gpart = parts.front();
        pinst.pos += gpart.pos + gpart.particleTime() * ctx.ps.targetOff;
      }
      if (pinst.texTile.y < 2)
        pinst.texTile.y += 2;
    };
    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      defaultDrawParticle(ctx, pinst, out);
      out.color = IColor(FColor(out.color) * choose(magicMissileColors, pinst.randomSeed));
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  { // Final explosion
    EmitterDef edef;
    edef.strength = 40.0f;
    edef.frequency = maxFrequency * 0.6f;
    edef.directionSpread = fconstant::pi;

    ParticleDef pdef;
    pdef.life = .4f;
    pdef.size = {{8.0f, 16.0f}};
    pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0, 1.0f, 1.0f, 0.0}};
    pdef.slowdown = 1.0f;
    pdef.texture = {"magic_missile_4x4.png", 4, 4};
    pdef.blendMode = BlendMode::additive;

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), flightTime, flightTime + 0.25f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      if (pinst.texTile.y < 2)
        pinst.texTile.y += 2;
      pinst.pos += ctx.ps.targetOff;
    };
    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      defaultDrawParticle(ctx, pinst, out);
      out.color = IColor(FColor(out.color) * choose(magicMissileColors, pinst.randomSeed));
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  { // Final explosion background
    EmitterDef edef;
    edef.strength = 0.0f;
    edef.initialSpawnCount = 1.0f;

    ParticleDef pdef;
    pdef.life = .4f;
    pdef.size = {{20.0f, 30.0f}};
    pdef.alpha = {{0.5f, 1.0f}, {maxBaseAlpha, 0.0}};
    pdef.texture = {"magic_missile_4x4.png", 4, 4};
    pdef.blendMode = BlendMode::additive;

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), flightTime, flightTime + 0.25f);
    ssdef.emitFunc = [](AnimationContext& ctx, EmissionState& em, Particle& pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.pos += ctx.ps.targetOff;
      pinst.texTile = {0, 0};
    };
    ssdef.drawFunc = [](DrawContext& ctx, const Particle& pinst, DrawParticle& out) {
      defaultDrawParticle(ctx, pinst, out);
      out.color = (IColor)(FColor(out.color) * FColor(ctx.ps.params.color[1]));
    };

    ssdef.maxTotalParticles = 1;

    psdef.subSystems.emplace_back(ssdef);
  }

  psdef.name = "magic_missile";
  mgr.addDef(psdef);
}

static void addFireEffect(FXManager& mgr) {
  ParticleSystemDef psdef;

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

    pdef.color = {{0.0f, 0.2f, 0.8f, 1.0f},
                  {FVec3(0.0f), IColor(155, 85, 30).rgb(), IColor(45, 35, 60).rgb(), FVec3(0.0f)}};
    pdef.texture = {"flames_4x4.png", 4, 4};
    pdef.blendMode = BlendMode::additive;

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
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
    pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 0.15, 0.0}, InterpType::cosine};

    pdef.color = {{0.0f, 0.5f, 1.0f}, {FVec3(0.0f), FVec3(0.3f), FVec3(0.0f)}};
    pdef.texture = {"clouds_soft_borders_4x4.png", 4, 4};

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
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

  psdef.name = "fire";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static void addSleepEffect(FXManager& mgr) {
  EmitterDef edef;
  edef.strength = 20.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.2f);
  edef.frequency = 3.0f;
  edef.source = FRect(-2, -8, 2, -5);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 1.0, 0.0}, InterpType::cosine};

  pdef.color = FVec3(1.0f);
  pdef.texture = {"special_4x2.png", 4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {0, 0};
    pinst.rot = ctx.rand.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "sleep";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static const array<IColor, 6> insanityColors = {{
  {244, 255, 190}, {255, 189, 230}, {190, 214, 220},
  {242, 124, 161}, {230, 240, 240}, {220, 200, 220},
}};

static void addInsanityEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.frequency = 4.0f;
  edef.source = FRect(-10, -12, 10, -4);

  ParticleDef pdef;
  pdef.life = 0.7f;
  pdef.size = {{5.0f, 8.0f, 12.0f}, InterpType::quadratic};
  pdef.alpha = {{0.0f, 0.2, 0.8f, 1.0f}, {0.0, 0.8, 0.8, 0.0}, InterpType::cosine};

  // TODO: zestaw losowych kolorów ?
  pdef.color = FVec3(1.0);
  pdef.texture = {"special_4x2.png", 4, 2};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {0, 1};
    pinst.rot = ctx.rand.getDouble(-0.2f, 0.2f);
  };

  ssdef.drawFunc = [](DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
    Particle temp(pinst);
    temp.size += FVec2(std::cos(pinst.life * 80.0f)) * 0.15;
    defaultDrawParticle(ctx, temp, out);
    out.color.setRGB(choose(insanityColors, pinst.randomSeed));
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "insanity";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
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

  pdef.color = IColor(253, 247, 172).rgb();
  pdef.texture = {"special_4x2.png", 4, 2};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {1, 0};
    pinst.rot = ctx.rand.getDouble(-0.2f, 0.2f);

    auto distFunc = [&](FVec2 pos) {
      float dist = fconstant::inf;
      for (auto &p : ctx.ss.particles)
        dist = min(dist, distanceSq(p.pos, pos));
      return dist;
    };

    float bestDist = distFunc(pinst.pos);

    // We're trying to make sure that particles are far away from each other
    for (int n = 0; n < 10; n++) {
      FVec2 newPos = ctx.edef.source.sample(ctx.rand);
      float dist = distFunc(newPos);
      if (dist > bestDist) {
        bestDist = dist;
        pinst.pos = newPos;
      }
    }
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "blind";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static void addPeacefulnessEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength = 8.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.3f);
  edef.frequency = 1.6f;
  edef.source = FRect(-8.0f, 0.0f, 8.0f, 8.0f);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = {{7.0f, 14.0f}};
  pdef.alpha = {{0.0f, 0.2f, 0.8f, 1.0f}, {0.0, 0.3f, 0.2, 0.0}, InterpType::cosine};

  pdef.color = IColor(255, 110, 209).rgb();
  pdef.texture = {"special_4x2.png", 4, 2};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.texTile = {3, 0};
    pinst.life = min(em.maxLife, float(ctx.ss.particles.size()) * 0.1f);
    pinst.rot = ctx.rand.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "peacefulness";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static void addSlowEffect(FXManager &mgr) {
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

  pdef.color = FVec3(0.7f, 1.0f, 0.6f);
  pdef.texture = "circular.png";

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.rot = 0.0f;
  };
  ssdef.drawFunc = [](DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
    Particle temp(pinst);
    FVec2 circlePos = angleToVector(12.0f - pinst.life * 3.0f);
    temp.pos += circlePos * FVec2(10.0f, 4.0f);
    defaultDrawParticle(ctx, temp, out);
    float alphaMul = dot(circlePos, FVec2(0.0f, 1.0f));
    float alpha = float(out.color.a) * clamp(alphaMul + 0.3f, 0.0f, 1.0f);
    out.color.a = (unsigned char)(alpha);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "slow";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static void addSpeedEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.frequency = 10.0f;
  edef.initialSpawnCount = 1.0f;
  edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.0f);
  edef.strength = 5.0f;
  edef.source = FVec2(0.0f, 10.0f);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.1f, 0.8f, 1.0f}, {0.0, 0.7f, 0.7f, 0.0}};

  pdef.color = FVec3(1.0f);
  pdef.texture = "circular.png";

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    defaultEmitParticle(ctx, em, pinst);
    pinst.rot = 0.0f;
  };
  ssdef.drawFunc = [](DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
    Particle temp(pinst);
    FVec2 circlePos = angleToVector(pinst.life * 6.0f);
    temp.pos += circlePos * FVec2(10.0f, 4.0f);
    defaultDrawParticle(ctx, temp, out);
    float alphaMul = dot(circlePos, FVec2(0.0f, 1.0f));
    float alpha = float(out.color.a) * clamp(alphaMul + 0.3f, 0.0f, 1.0f);
    out.color.a = (unsigned char)(alpha);
  };

  ParticleSystemDef psdef;
  psdef.subSystems = {ssdef};
  psdef.name = "speed";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

static void addFlyingEffect(FXManager &mgr) {
  ParticleSystemDef psdef;

  { // clouds
    EmitterDef edef;
    edef.frequency = 12.0f;
    edef.initialSpawnCount = 2.0f;
    edef.setStrengthSpread(2.0f, 1.0f);
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.1f);
    edef.source = FRect(-8.0f, 8.0f, 8.0f, 12.0f);

    ParticleDef pdef;
    pdef.life = 1.0f;
    pdef.size = 16.0f;
    pdef.alpha = {{0.0f, 0.5, 1.0f}, {0.0, 0.4, 0.0}, InterpType::cosine};

    pdef.color = FVec3(1.0f);
    pdef.texture = {"clouds_soft_4x4.png", 4, 4};

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.rot = 0.0f;
      pinst.size = FVec2(1.2f, 0.6f);
    };
    psdef.subSystems.emplace_back(ssdef);
  }

  { // sparkles
    EmitterDef edef;
    edef.frequency = 12.0f;
    edef.initialSpawnCount = 2.0f;
    edef.setStrengthSpread(17.5f, 2.5f);
    edef.setDirectionSpread(-fconstant::pi * 0.5f, 0.1f);
    edef.source = FRect(-8.0f, 8.0f, 8.0f, 12.0f);

    ParticleDef pdef;
    pdef.life = 1.0f;
    pdef.size = 6.0f;
    pdef.alpha = {{0.0f, 0.5, 1.0f}, {0.0, 0.5, 0.0}, InterpType::cosine};

    pdef.color = IColor(253, 247, 122).rgb();
    pdef.texture = {"special_4x2.png", 4, 2};

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
    ssdef.emitFunc = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
      defaultEmitParticle(ctx, em, pinst);
      pinst.texTile = {0, 1};
    };

    psdef.subSystems.emplace_back(ssdef);
  }

  psdef.name = "flying";
  psdef.isLooped = true;
  psdef.animLength = 1.0f;
  mgr.addDef(psdef);
}

void FXManager::addDefaultDefs() {
  addTestSimpleEffect(*this);
  addTestMultiEffect(*this);
  addWoodSplinters(*this);
  addRockSplinters(*this);
  addRockCloud(*this);
  addExplosionEffect(*this);
  addRippleEffect(*this);
  addCircularBlast(*this);
  addFeetDustEffect(*this);
  addMagicMissileEffect(*this);
  addFireEffect(*this);

  addSleepEffect(*this);
  addInsanityEffect(*this);
  addBlindEffect(*this);
  addPeacefulnessEffect(*this);

  addSlowEffect(*this);
  addSpeedEffect(*this);
  addFlyingEffect(*this);
};
}
