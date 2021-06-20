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
  return nightVision || light >= getDarknessVisionThreshold() || distance <= darkViewRadius;
}

void Vision::update(const Creature* c, GlobalTime time) {
  PROFILE;
  nightVision = c->isAffected(LastingEffect::NIGHT_VISION, time);
  if (c->isAffected(LastingEffect::ARCHER_VISION, time))
    id = VisionId::ARCHER;
  else if (c->isAffected(LastingEffect::ELF_VISION, time) || c->isAffected(LastingEffect::FLYING, time))
    id = VisionId::ELF;
  else
    id = VisionId::NORMAL;
}

double Vision::getDarknessVisionThreshold() {
  return 0.8;
}
