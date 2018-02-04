#include "stdafx.h"
#include "vision.h"
#include "creature_attributes.h"
#include "creature.h"

SERIALIZE_DEF(Vision, id, nightVision)

VisionId Vision::getId() const {
  return id;
}

constexpr int darkViewRadius = 5;

bool Vision::canSeeAt(double light, double distance) const {
  return nightVision || light > 0.8 || distance <= darkViewRadius;
}

void Vision::update(WConstCreature c) {
  PROFILE;
  nightVision = c->isAffected(LastingEffect::NIGHT_VISION);
  if (c->isAffected(LastingEffect::ELF_VISION) || c->isAffected(LastingEffect::FLYING))
    id = VisionId::ELF;
  else
    id = VisionId::NORMAL;
}
