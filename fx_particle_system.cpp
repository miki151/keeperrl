#include "fx_particle_system.h"

#include "fx_emitter_def.h"
#include "fx_particle_def.h"
#include "fx_rect.h"

namespace fx {

ParticleSystem::ParticleSystem(FVec2 pos, FVec2 targetOff, ParticleSystemDefId defId, int spawnTime, int numSubSystems)
    : subSystems(numSubSystems), pos(pos), targetOff(targetOff), defId(defId), spawnTime(spawnTime) {}

void ParticleSystem::kill() {
  subSystems.clear();
  isDead = true;
}

int ParticleSystem::numActiveParticles() const {
  int out = 0;
  for (auto &ss : subSystems)
    out += (int)ss.particles.size();
  return out;
}

int ParticleSystem::numTotalParticles() const {
  int out = 0;
  for (auto &ss : subSystems)
    out += (int)ss.totalParticles;
  return out;
}

void defaultAnimateParticle(AnimationContext &ctx, Particle &pinst) {
  const auto &pdef = ctx.pdef;

  float ptime = pinst.particleTime();
  float slowdown = 1.0f / (1.0f + pdef.slowdown.sample(ptime));

  pinst.pos += pinst.movement * ctx.timeDelta;
  pinst.rot += pinst.rotSpeed * ctx.timeDelta;
  if (slowdown < 1.0f) {
    float factor = pow(slowdown, ctx.timeDelta);
    pinst.movement *= factor;
    pinst.rotSpeed *= factor;
  }
  pinst.life += ctx.timeDelta;
}

float defaultPrepareEmission(AnimationContext &ctx, EmissionState &em) {
  auto &pdef = ctx.pdef;
  auto &edef = ctx.edef;

  em.maxLife = pdef.life.sample(em.time);

  em.angle = edef.angle.sample(em.time);
  em.angleSpread = edef.angleSpread.sample(em.time);

  em.strengthMin = edef.strengthMin.sample(em.time);
  em.strengthMax = edef.strengthMax.sample(em.time);

  em.rotSpeedMin = edef.rotationSpeedMin.sample(em.time);
  em.rotSpeedMax = edef.rotationSpeedMax.sample(em.time);

  return edef.frequency.sample(em.time) * ctx.timeDelta;
}

float AnimationContext::uniformSpread(float spread) { return rand.getDouble(-spread, spread); }
float AnimationContext::uniform(float min, float max) { return rand.getDouble(min, max); }
int AnimationContext::randomSeed() { return rand.getLL() % 1973257861; }

SVec2 AnimationContext::randomTexTile() {
  if (!(pdef.textureTiles == IVec2(1, 1))) {
    IVec2 texTile(rand.get(pdef.textureTiles.x), rand.get(pdef.textureTiles.y));
    return SVec2(texTile);
  }

  return SVec2(0, 0);
}

void defaultEmitParticle(AnimationContext &ctx, EmissionState &em, Particle &newInst) {
  newInst.pos = ctx.edef.source.sample(ctx.rand);
  float pangle;
  if (em.angleSpread < fconstant::pi)
    pangle = em.angle + ctx.uniformSpread(em.angleSpread);
  else
    pangle = ctx.uniform(0.0f, fconstant::pi * 2.0f);

  FVec2 pdir = angleToVector(pangle);
  float strength = ctx.uniform(em.strengthMin, em.strengthMax);
  float rotSpeed = ctx.uniform(em.rotSpeedMin, em.rotSpeedMax);
  newInst.movement = pdir * strength;
  newInst.rot = ctx.uniform(0.0f, fconstant::pi * 2.0f);
  newInst.rotSpeed = rotSpeed * strength;
  newInst.life = 0.0f;
  newInst.maxLife = em.maxLife;
  newInst.texTile = ctx.randomTexTile();
}

array<FVec2, 4> DrawContext::quadCorners(FVec2 pos, FVec2 size, float rotation) const {
  auto corners = FRect(pos - size * 0.5f, pos + size * 0.5f).corners();
  for (auto &corner : corners)
    corner = rotateVector(corner - pos, rotation) + pos;
  return corners;
}

array<FVec2, 4> DrawContext::texQuadCorners(SVec2 texTile) const {
  FRect tex_rect(FVec2(1));
  if (!(pdef.textureTiles == IVec2(1, 1)))
    tex_rect = (tex_rect + FVec2(texTile)) * invTexTile;
  return tex_rect.corners();
}

void defaultDrawParticle(DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
  float ptime = pinst.particleTime();
  const auto &pdef = ctx.pdef;

  FVec2 pos = pinst.pos + ctx.ps.pos;
  FVec2 size(pdef.size.sample(ptime) * pinst.size);
  float alpha = pdef.alpha.sample(ptime);

  // TODO: by default params dont apply ?
  FColor color(pdef.color.sample(ptime) * ctx.ps.params.color[0], alpha);
  out.positions = ctx.quadCorners(pos, size, pinst.rot);
  out.texCoords = ctx.texQuadCorners(pinst.texTile);
  out.color = IColor(color);
}

SubSystemContext::SubSystemContext(const ParticleSystem &ps, const ParticleSystemDef &psdef, const ParticleDef &pdef,
                                   const EmitterDef &edef, int ssid)
    : ps(ps), ss(ps.subSystems[ssid]), psdef(psdef), ssdef(psdef[ssid]), pdef(pdef), edef(edef), ssid(ssid) {}

AnimationContext::AnimationContext(const SubSystemContext &ssctx, float animTime, float timeDelta)
    : SubSystemContext(ssctx), animTime(animTime), timeDelta(timeDelta), invTimeDelta(1.0f / timeDelta) {}

DrawContext::DrawContext(const SubSystemContext &ssctx, FVec2 invTexTile)
    : SubSystemContext(ssctx), invTexTile(invTexTile) {}
}
