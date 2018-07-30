#include "fx_particle_system.h"

#include "fx_emitter_def.h"
#include "fx_particle_def.h"
#include "fx_rect.h"

namespace fx {

ParticleSystem::ParticleSystem(FVec2 pos, ParticleSystemDefId def_id, int spawn_time, int num_subsystems)
    : subsystems(num_subsystems), pos(pos), def_id(def_id), spawn_time(spawn_time) {}

void ParticleSystem::kill() {
  subsystems.clear();
  is_dead = true;
}

int ParticleSystem::numActiveParticles() const {
  int out = 0;
  for(auto &ss : subsystems)
    out += (int)ss.particles.size();
  return out;
}

int ParticleSystem::numTotalParticles() const {
  int out = 0;
  for(auto &ss : subsystems)
    out += (int)ss.total_particles;
  return out;
}

void defaultAnimateParticle(AnimationContext &ctx, Particle &pinst) {
  const auto &pdef = ctx.pdef;

  float ptime = pinst.particleTime();
  float slowdown = 1.0f / (1.0f + pdef.slowdown.sample(ptime));

  pinst.pos += pinst.movement * ctx.time_delta;
  pinst.rot += pinst.rot_speed * ctx.time_delta;
  if(slowdown < 1.0f) {
    float factor = pow(slowdown, ctx.time_delta);
    pinst.movement *= factor;
    pinst.rot_speed *= factor;
  }
  pinst.life += ctx.time_delta;
}

float defaultPrepareEmission(AnimationContext &ctx, EmissionState &em) {
  auto &pdef = ctx.pdef;
  auto &edef = ctx.edef;

  em.max_life = pdef.life.sample(em.time);

  em.angle = edef.angle.sample(em.time);
  em.angle_spread = edef.angle_spread.sample(em.time);

  em.strength_min = edef.strength_min.sample(em.time);
  em.strength_max = edef.strength_max.sample(em.time);

  em.rot_speed_min = edef.rotation_speed_min.sample(em.time);
  em.rot_speed_max = edef.rotation_speed_max.sample(em.time);

  return edef.frequency.sample(em.time) * ctx.time_delta;
}

float AnimationContext::uniformSpread(float spread) { return rand.getDouble(-spread, spread); }
float AnimationContext::uniform(float min, float max) { return rand.getDouble(min, max); }
int AnimationContext::randomSeed() { return rand.getLL() % 1973257861; }

SVec2 AnimationContext::randomTexTile() {
  if(!(pdef.texture_tiles == IVec2(1, 1))) {
    IVec2 tex_tile(rand.get(pdef.texture_tiles.x - 1), rand.get(pdef.texture_tiles.y - 1));
    return SVec2(tex_tile);
  }

  return SVec2(0, 0);
}

void defaultEmitParticle(AnimationContext &ctx, EmissionState &em, Particle &new_inst) {
  new_inst.pos = ctx.edef.source.sample(ctx.rand);
  float pangle;
  if(em.angle_spread < fconstant::pi)
    pangle = em.angle + ctx.uniformSpread(em.angle_spread);
  else
    pangle = ctx.uniform(0.0f, fconstant::pi * 2.0f);

  FVec2 pdir = angleToVector(pangle);
  float strength = ctx.uniform(em.strength_min, em.strength_max);
  float rot_speed = ctx.uniform(em.rot_speed_min, em.rot_speed_max);
  new_inst.movement = pdir * strength;
  new_inst.rot = ctx.uniform(0.0f, fconstant::pi * 2.0f);
  new_inst.rot_speed = rot_speed * strength;
  new_inst.life = 0.0f;
  new_inst.max_life = em.max_life;
  new_inst.tex_tile = ctx.randomTexTile();
}

array<FVec2, 4> DrawContext::quadCorners(FVec2 pos, FVec2 size, float rotation) const {
  auto corners = FRect(pos - size * 0.5f, pos + size * 0.5f).corners();
  for(auto &corner : corners)
    corner = rotateVector(corner - pos, rotation) + pos;
  return corners;
}

array<FVec2, 4> DrawContext::texQuadCorners(SVec2 tex_tile) const {
  FRect tex_rect(FVec2(1));
  if(!(pdef.texture_tiles == IVec2(1, 1)))
    tex_rect = (tex_rect + FVec2(tex_tile)) * inv_tex_tile;
  return tex_rect.corners();
}

void defaultDrawParticle(DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
  float ptime = pinst.particleTime();
  const auto &pdef = ctx.pdef;

  FVec2 pos = pinst.pos + ctx.ps.pos;
  FVec2 size(pdef.size.sample(ptime) * pinst.size);
  float alpha = pdef.alpha.sample(ptime);

  FColor color(pdef.color.sample(ptime) * ctx.ps.params.color[0],
               alpha); // TODO: by default params dont apply ?
  out.positions = ctx.quadCorners(pos, size, pinst.rot);
  out.tex_coords = ctx.texQuadCorners(pinst.tex_tile);
  out.color = IColor(color);
}

SubSystemContext::SubSystemContext(const ParticleSystem &ps, const ParticleSystemDef &psdef, const ParticleDef &pdef,
                                   const EmitterDef &edef, int ssid)
    : ps(ps), ss(ps.subsystems[ssid]), psdef(psdef), ssdef(psdef[ssid]), pdef(pdef), edef(edef), ssid(ssid) {}

AnimationContext::AnimationContext(const SubSystemContext &ssctx, float anim_time, float time_delta)
    : SubSystemContext(ssctx), anim_time(anim_time), time_delta(time_delta), inv_time_delta(1.0f / time_delta) {}

DrawContext::DrawContext(const SubSystemContext &ssctx, FVec2 inv_tex_tile)
    : SubSystemContext(ssctx), inv_tex_tile(inv_tex_tile) {}
}
