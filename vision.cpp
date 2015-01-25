#include "stdafx.h"
#include "vision.h"

template <class Archive> 
void CreatureVision::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(CreatureVision);

Vision::Vision(Vision* inherited, bool night) : inheritedFov(inherited), nightVision(night) {}

Vision* Vision::getInheritedFov() const {
  return inheritedFov;
}

bool Vision::isNightVision() const {
  return nightVision;
}

void Vision::init() {
  Vision::set(VisionId::NORMAL, new Vision(nullptr, false));
  Vision::set(VisionId::ELF, new Vision(Vision::get(VisionId::NORMAL), false));
  Vision::set(VisionId::NIGHT, new Vision(Vision::get(VisionId::NORMAL), true));
}

