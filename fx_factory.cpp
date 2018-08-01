#include "fx_manager.h"

#include "fx_color.h"
#include "fx_emitter_def.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"
#include "fx_rect.h"

namespace fx {

static const FVec2 dir_vecs[8] = {{0.0, -1.0}, {0.0, 1.0},   {1.0, 0.0}, {-1.0, 0.0},
                                  {1.0, -1.0}, {-1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}};
FVec2 dirToVec(Dir dir) { return dir_vecs[(int)dir]; }

using SubSystemDef = ParticleSystemDef::SubSystem;

static void addTestSimpleEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength_min = edef.strength_max = 30.0f;
  edef.frequency = {{10.0f, 55.0f, 0.0f, 0.0}, InterpType::cosine};

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 32.0f;
  pdef.alpha = {{0.0f, 0.1, 0.8f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::linear};

  pdef.color = {{{1.0f, 1.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {0.2f, 0.5f, 1.0f}}, InterpType::linear};
  pdef.texture_name = "circular.png";

  ParticleSystemDef psdef;
  psdef.subsystems.emplace_back(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 5.0f);
  psdef.name = "test_simple";
  mgr.addDef(psdef);
}

static void addTestMultiEffect(FXManager &mgr) {
  EmitterDef edef;
  edef.strength_min = edef.strength_max = 30.0f;
  edef.frequency = {{10.0f, 55.0f, 0.0f, 0.0}, InterpType::cosine};

  FVec3 colors[5] = {{0.7f, 0.2, 0.2f}, {0.2f, 0.7f, 0.2f}, {0.2f, 0.2f, 0.7f}, {0.9f, 0.2, 0.9f}, {0.3f, 0.9f, 0.4f}};

  ParticleDef pdefs[5];
  for(int n = 0; n < 5; n++) {
    pdefs[n].life = 1.0f;
    pdefs[n].size = 32.0f;
    pdefs[n].alpha = {{0.0f, 0.1, 0.8f, 1.0f}, {0.0, 1.0, 1.0, 0.0}, InterpType::linear};
    pdefs[n].color = {colors[n]};
    pdefs[n].texture_name = "circular.png";
  }

  ParticleSystemDef psdef;
  psdef.name = "test_multi";
  for(int n = 0; n < 5; n++)
    psdef.subsystems.emplace_back(mgr.addDef(pdefs[n]), mgr.addDef(edef), float(n) * 0.5f, float(n) * 1.5f + 2.0f);
  mgr.addDef(psdef);
}

static void addWoodSplinters(FXManager &mgr) {
  EmitterDef edef;
  edef.strength_min = 20.0f;
  edef.strength_max = 60.0f;
  edef.rotation_speed_min = -0.5f;
  edef.rotation_speed_max = 0.5f;
  edef.frequency = 999.0f;

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 4.0f;
  pdef.slowdown = {{0.0f, 0.1f}, {5.0f, 1000.0f}};
  pdef.alpha = {{0.0f, 0.3f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

  auto animate_func = [](AnimationContext &ctx, Particle &pinst) {
    defaultAnimateParticle(ctx, pinst);
    float shadow_min = 5.0f, shadow_max = 10.0f;
    if(pinst.pos.y < shadow_min) {
      float dist = shadow_min - pinst.pos.y;
      pinst.movement += FVec2(0.0f, dist);
    }
    pinst.pos.y = min(pinst.pos.y, shadow_max);
  };

  FColor brown(IColor(120, 87, 46));
  // Kiedy cząsteczki opadną pod drzewo, robią się w zasięgu cienia
  // TODO: lepiej rysować je po prostu pod cieniem
  pdef.color = {{0.0f, 0.15f, 0.17}, {brown.rgb(), brown.rgb(), brown.rgb() * 0.6f}};
  pdef.texture_name = "flakes_4x4_borders.png";
  pdef.texture_tiles = {4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.max_total_particles = 7;
  ssdef.animate_func = animate_func;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
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
  edef.strength_min = 20.0f;
  edef.strength_max = 60.0f;
  edef.rotation_speed_min = -0.5f;
  edef.rotation_speed_max = 0.5f;
  edef.frequency = 999.0f;

  ParticleDef pdef;
  pdef.life = 1.0f;
  pdef.size = 4.0f;
  pdef.slowdown = {{0.0f, 0.1f}, {5.0f, 1000.0f}};
  pdef.alpha = {{0.0f, 0.4f, 1.0f}, {1.0, 1.0, 0.0}, InterpType::cosine};

  pdef.color = FVec3(0.4, 0.4, 0.4);
  pdef.texture_name = "flakes_4x4_borders.png";
  pdef.texture_tiles = {4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.max_total_particles = 5;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
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
  edef.strength_min = 5.0f;
  edef.strength_max = 8.0f;
  edef.frequency = 60.0f;

  ParticleDef pdef;
  pdef.life = 3.5f;
  pdef.size = {{0.0f, 0.1f, 1.0f}, {15.0f, 30.0f, 38.0f}};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.3f, 0.4f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};

  FVec3 start_color(0.6), end_color(0.4);
  pdef.color = {{start_color, end_color}};
  pdef.texture_name = "clouds_soft_4x4.png";
  pdef.texture_tiles = {4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);

  // TODO: różna liczba początkowych cząsteczek
  ssdef.max_total_particles = 5;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
  psdef.name = "rock_clouds";
  mgr.addDef(psdef);
}

static void addExplosionEffect(FXManager &mgr) {
  // TODO: tutaj trzeba zrobić tak, żeby cząsteczki które spawnują się później
  // zaczynały z innym kolorem
  EmitterDef edef;
  edef.strength_min = edef.strength_max = 15.0f;
  edef.frequency = 60.0f;

  ParticleDef pdef;
  pdef.life = 0.5f;
  pdef.size = {{5.0f, 30.0f}};
  pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.3, 0.4, 0.0}};

  IColor start_color(255, 244, 88), end_color(225, 92, 19);
  pdef.color = {{FColor(start_color).rgb(), FColor(end_color).rgb()}};
  pdef.texture_name = "clouds_soft_borders_4x4.png";
  pdef.texture_tiles = {4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.5f);
  ssdef.max_total_particles = 20;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
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
  edef.initial_spawn_count = 1.0f;

  // First scalar parameter controls how fast the ripples move
  auto prep_func = [](AnimationContext &ctx, EmissionState &em) {
    float freq = defaultPrepareEmission(ctx, em);
    float mod = 1.0f + ctx.ps.params.scalar[0];
    return freq * mod;
  };

  auto animate_func = [](AnimationContext &ctx, Particle &pinst) {
    float ptime = pinst.particleTime();
    float mod = 1.0f + ctx.ps.params.scalar[0];
    float time_delta = ctx.time_delta * mod;
    pinst.pos += pinst.movement * time_delta;
    pinst.rot += pinst.rot_speed * time_delta;
    pinst.life += time_delta;
  };

  ParticleDef pdef;
  pdef.life = 1.5f;
  pdef.size = {{10.0f, 50.0f}};
  pdef.alpha = {{0.0f, 0.3f, 0.6f, 1.0f}, {0.0f, 0.3f, 0.5f, 0.0f}};

  pdef.color = FVec3(1.0f, 1.0f, 1.0f);
  pdef.texture_name = "torus.png";

  // TODO: możliwośc nie podawania czasu emisji (etedy emituje przez całą długośc animacji)?
  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.max_active_particles = 10;
  ssdef.prepare_func = prep_func;
  ssdef.animate_func = animate_func;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
  psdef.anim_length = 1.0f;
  psdef.is_looped = true;
  psdef.name = "ripple";

  mgr.addDef(psdef);
}

static void addCircularBlast(FXManager &mgr) {
  EmitterDef edef;
  edef.frequency = 50.0f;
  edef.initial_spawn_count = 10.0;

  auto prep_func = [](AnimationContext &ctx, EmissionState &em) {
    defaultPrepareEmission(ctx, em);
    return 0.0f;
  };

  auto emit_func = [](AnimationContext &ctx, EmissionState &em, Particle &new_inst) {
    new_inst.life = min(em.max_life, float(ctx.ss.total_particles) * 0.01f);
    new_inst.max_life = em.max_life;
  };

  auto animate_func = [](AnimationContext &ctx, Particle &pinst) {
    float ptime = pinst.particleTime();
    float mod = 1.0f + ctx.ps.params.scalar[0];
    float time_delta = ctx.time_delta * mod;
    pinst.life += pinst.movement.x;
    pinst.movement.x = 0;
    pinst.life += time_delta;
  };

  ParticleDef pdef;
  pdef.life = 0.5f;
  pdef.size = {{10.0f, 80.0f}, InterpType::cosine};
  pdef.alpha = {{0.0f, 0.03f, 0.2f, 1.0f}, {0.0f, 0.15f, 0.15f, 0.0f}, InterpType::cosine};

  pdef.color = {{0.5f, 0.8f}, {{1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 1.0f}}};
  pdef.texture_name = "torus.png";

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.1f);
  ssdef.max_active_particles = 20;
  ssdef.prepare_func = prep_func;
  ssdef.animate_func = animate_func;
  ssdef.emit_func = emit_func;

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
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
  edef.strength_min = 15.0f;
  edef.strength_max = 20.0f;
  edef.frequency = 60.0f;

  ParticleDef pdef;
  pdef.life = 1.25f;
  pdef.size = {{0.0f, 0.1f, 1.0f}, {5.0f, 14.0f, 20.0f}, InterpType::quadratic};
  pdef.alpha = {{0.0f, 0.05f, 0.2f, 1.0f}, {0.0f, 0.2f, 0.3f, 0.0f}};
  pdef.slowdown = {{0.0f, 0.2f}, {0.0f, 10.0f}};

  FVec3 start_color(0.9), end_color(0.7);
  pdef.color = {{start_color, end_color}};
  pdef.texture_name = "clouds_soft_4x4.png";
  pdef.texture_tiles = {4, 4};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.2f);
  // TODO: różna liczba początkowych cząsteczek
  ssdef.max_total_particles = 3;

  ssdef.emit_func = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
    auto dvec = dirToVec(ctx.ps.params.dir[0]);
    defaultEmitParticle(ctx, em, pinst);
    pinst.pos -= dvec * 4.0f;
    pinst.rot = 0.0f;
    pinst.size = FVec2(1.2f, 0.6f);
  };
  ssdef.prepare_func = [](AnimationContext &ctx, EmissionState &em) {
    auto ret = defaultPrepareEmission(ctx, em);
    auto vec = normalize(dirToVec(ctx.ps.params.dir[0]));
    em.angle = vectorToAngle(vec);
    em.angle_spread = 0.0f;
    return ret;
  };

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
  psdef.name = "feet_dust";
  mgr.addDef(psdef);
}

static void addMagicMissleEffect(FXManager &mgr) {
  // Każda cząsteczka z czasem z grubsza liniowo przechodzi od źródła do celu
  // dodatkowo może być delikatnie przesunięta z głównego toru

  ParticleSystemDef psdef;
  { // Base system, not visible, only a source for other particles
    EmitterDef edef;
    edef.strength_min = edef.strength_max = 100.0f;
    edef.frequency = 60.0f;
    edef.angle_spread = fconstant::pi;

    ParticleDef pdef;
    pdef.life = .45f;
    pdef.size = {{15.0f, 20.0f}};
    pdef.alpha = 0.0f; // TODO: don't draw invisible particles
    pdef.slowdown = 1.0f;

    SubSystemDef ssdef_base(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.5f);
    ssdef_base.max_total_particles = 1;

    ssdef_base.animate_func = [](AnimationContext &ctx, Particle &pinst) {
      defaultAnimateParticle(ctx, pinst);
      float attract_value = max(0.0001f, std::pow(min(1.0f, 1.0f - pinst.particleTime()), 5.0f));
      float mul = std::pow(0.001f * attract_value, ctx.time_delta);
      pinst.pos *= mul;
    };

    ssdef_base.draw_func = [](DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
      Particle temp(pinst);
      temp.pos += temp.particleTime() * ctx.ps.target_off;
      defaultDrawParticle(ctx, temp, out);
    };
    psdef.subsystems.emplace_back(ssdef_base);
  }

  { // Secondary system
    EmitterDef edef;
    edef.strength_min = edef.strength_max = 40.0f;
    edef.frequency = 50.0f;
    edef.angle_spread = fconstant::pi;

    ParticleDef pdef;
    pdef.life = .3f;
    pdef.size = {{20.0f, 25.0f}};
    pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 1.0, 0.0}};
    pdef.slowdown = 1.0f;
    IColor color(155, 244, 228);
    pdef.color = {{FColor(color).rgb(), FColor(0.2f, 0.2f, 0.8f).rgb()}};
    pdef.texture_name = "circular.png";

    SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 0.5f);

    ssdef.prepare_func = [](AnimationContext &ctx, EmissionState &em) {
      auto ret = defaultPrepareEmission(ctx, em);
      auto &parts = ctx.ps.subsystems[0].particles;
      if(parts.empty())
        return 0.0f;
      em.strength_min = em.strength_max = em.strength_max * (1.2f - parts.front().particleTime());
      return ret;
    };

    ssdef.emit_func = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
      defaultEmitParticle(ctx, em, pinst);
      auto &parts = ctx.ps.subsystems[0].particles;
      if(!parts.empty()) {
        auto &gpart = parts.front();
        pinst.pos += gpart.pos + gpart.particleTime() * ctx.ps.target_off;
      }
    };
    psdef.subsystems.emplace_back(ssdef);
  }

  psdef.name = "magic_missile";
  mgr.addDef(psdef);
}

static void addSleepEffect(FXManager &mgr) {
  // TODO: tutaj trzeba zrobić tak, żeby cząsteczki które spawnują się później
  // zaczynały z innym kolorem
  EmitterDef edef;
  edef.strength_min = edef.strength_max = 20.0f;
  edef.angle = -fconstant::pi * 0.5f;
  edef.angle_spread = 0.2f;
  edef.frequency = 3.0f;
  edef.source = FRect(-2, -8, 2, -5);

  ParticleDef pdef;
  pdef.life = 2.0f;
  pdef.size = 10.0f;
  pdef.alpha = {{0.0f, 0.5f, 1.0f}, {0.0, 1.0, 0.0}, InterpType::cosine};

  pdef.color = FVec3(1.0f);
  pdef.texture_name = "special_4x1.png";
  pdef.texture_tiles = {4, 1};

  SubSystemDef ssdef(mgr.addDef(pdef), mgr.addDef(edef), 0.0f, 1.0f);
  ssdef.emit_func = [](AnimationContext &ctx, EmissionState &em, Particle &pinst) {
	defaultEmitParticle(ctx, em, pinst);
	pinst.tex_tile = {0,0};
	pinst.rot = ctx.rand.getDouble(-0.2f, 0.2f);
  };

  ParticleSystemDef psdef;
  psdef.subsystems = {ssdef};
  psdef.name = "sleep";
  psdef.is_looped = true;
  psdef.anim_length = 1.0f;
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
  addMagicMissleEffect(*this);
  addSleepEffect(*this);
};
}
