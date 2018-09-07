#include "fx_particle_system.h"

#include "fx_defs.h"
#include "fx_rect.h"
#include "renderer.h"

namespace fx {

float SnapshotKey::distanceSq(const SnapshotKey& rhs) const {
  float sum = 0.0f;
  for (int n = 0; n < maxScalars; n++)
    sum += (scalar[n] - rhs.scalar[n]) * (scalar[n] - rhs.scalar[n]);
  return sum;
}

SnapshotKey::SnapshotKey(const SystemParams& params) {
  for (int n = 0; n < maxScalars; n++)
    scalar[n] = params.scalar[n];
}

void SnapshotKey::apply(SystemParams& out) const {
  for (int n = 0; n < maxScalars; n++)
    out.scalar[n] = scalar[n];
}

bool SnapshotKey::operator==(const SnapshotKey& rhs) const {
  for (int n = 0; n < maxScalars; n++)
    if (scalar[n] != rhs.scalar[n])
      return false;
  return true;
}

ParticleSystem::ParticleSystem(FXName defId, const InitConfig& config, uint spawnTime, vector<SubSystem> snapshot)
    : subSystems(std::move(snapshot)), pos(config.pos), targetOffset(config.targetOffset), defId(defId),
      spawnTime(spawnTime) {
  float dist = length(targetOffset);
  targetDir = dist < 0.00001f ? FVec2(1, 0) : targetOffset / dist;
  targetTileDist = dist / float(Renderer::nominalSize);
  targetDirAngle = vectorToAngle(targetDir);
  if (config.snapshotKey)
    config.snapshotKey->apply(params);
}

void ParticleSystem::randomize(RandomGen& random) {
  for (auto& ss : subSystems)
    ss.randomSeed = random.get(INT_MAX);
}

void ParticleSystem::kill(bool immediate) {
  if (immediate) {
    isDead = true;
    subSystems.clear();
  } else {
    isDying = true;
  }
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

  em.direction = edef.direction.sample(em.time);
  em.directionSpread = edef.directionSpread.sample(em.time);

  em.strength = edef.strength.sample(em.time);
  em.strengthSpread = edef.strengthSpread.sample(em.time);

  em.rotSpeed = edef.rotSpeed.sample(em.time);
  em.rotSpeedSpread = edef.rotSpeedSpread.sample(em.time);

  return edef.frequency.sample(em.time) * ctx.timeDelta;
}

float AnimationContext::uniformSpread(float spread) { return rand.getDouble(-spread, spread); }
float AnimationContext::uniform(float min, float max) { return rand.getDouble(min, max); }
uint AnimationContext::randomSeed() {
  return rand.get(INT_MAX);
}

SVec2 AnimationContext::randomTexTile() {
  if (!(tdef.tiles == IVec2(1, 1))) {
    IVec2 texTile(rand.get(tdef.tiles.x), rand.get(tdef.tiles.y));
    return SVec2(texTile);
  }

  return SVec2(0, 0);
}

void defaultEmitParticle(AnimationContext &ctx, EmissionState &em, Particle &newInst) {
  newInst.pos = ctx.edef.source.sample(ctx.rand);
  float pangle;
  if (em.directionSpread < fconstant::pi)
    pangle = em.direction + ctx.uniformSpread(em.directionSpread);
  else
    pangle = ctx.uniform(0.0f, fconstant::pi * 2.0f);

  FVec2 pdir = angleToVector(pangle);
  float strength = em.strength;
  float rotSpeed = em.rotSpeed;

  if (em.strengthSpread > 0.0f)
    strength += ctx.uniformSpread(em.strengthSpread);
  if (em.rotSpeedSpread > 0.0f)
    rotSpeed += ctx.uniformSpread(em.rotSpeedSpread);
  if (rotSpeed > 0.0f && ctx.rand.chance(0.5))
    rotSpeed = -rotSpeed;

  newInst.movement = pdir * strength;
  newInst.rot = ctx.uniform(0.0f, fconstant::pi * 2.0f);
  newInst.rotSpeed = rotSpeed * strength; // TODO: is this necessary ?
  newInst.life = 0.0f;
  newInst.maxLife = em.maxLife;
  newInst.texTile = ctx.randomTexTile();
  newInst.randomSeed = ctx.randomSeed();
}

array<FVec2, 4> DrawContext::quadCorners(FVec2 pos, FVec2 size, float rotation) const {
  auto corners = FRect(pos - size * 0.5f, pos + size * 0.5f).corners();
  for (auto &corner : corners)
    corner = rotateVector(corner - pos, rotation) + pos;
  return corners;
}

array<FVec2, 4> DrawContext::texQuadCorners(SVec2 texTile) const {
  FRect tex_rect(FVec2(1));
  if (!(tdef.tiles == IVec2(1, 1)))
    tex_rect = (tex_rect + FVec2(texTile)) * invTexTile;
  return tex_rect.corners();
}

void defaultDrawParticle(DrawContext &ctx, const Particle &pinst, DrawParticle &out) {
  float ptime = pinst.particleTime();
  const auto &pdef = ctx.pdef;

  FVec2 pos = pinst.pos + ctx.ps.pos;
  FVec2 size(pdef.size.sample(ptime) * pinst.size);
  float alpha = pdef.alpha.sample(ptime);
  FVec3 colorMul = ctx.ps.params.color[0];
  if (ctx.tdef.blendMode == BlendMode::additive)
    colorMul *= alpha;

  FColor color(pdef.color.sample(ptime) * colorMul, alpha);
  out.positions = ctx.quadCorners(pos, size, pinst.rot);
  out.texCoords = ctx.texQuadCorners(pinst.texTile);
  out.color = IColor(color);
}

SubSystemContext::SubSystemContext(const ParticleSystem& ps, const ParticleSystemDef& psdef, const ParticleDef& pdef,
                                   const EmitterDef& edef, const TextureDef& tdef, int ssid)
    : ps(ps), ss(ps.subSystems[ssid]), psdef(psdef), ssdef(psdef[ssid]), pdef(pdef), edef(edef), tdef(tdef),
      ssid(ssid) {}

AnimationContext::AnimationContext(const SubSystemContext &ssctx, float animTime, float timeDelta)
    : SubSystemContext(ssctx), animTime(animTime), timeDelta(timeDelta), invTimeDelta(1.0f / timeDelta) {}

DrawContext::DrawContext(const SubSystemContext &ssctx, FVec2 invTexTile)
    : SubSystemContext(ssctx), invTexTile(invTexTile) {}
}
