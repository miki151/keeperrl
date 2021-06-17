#include "stdafx.h"
#include "movement_type.h"
#include "tribe.h"

SERIALIZE_DEF(MovementType, traits, tribeSet, sunlightVulnerable, fireResistant, forced, destroyActions, buildBridge)

MovementType::MovementType(EnumSet<MovementTrait> t) : traits(t) {
}

MovementType::MovementType(MovementTrait t) : traits({t}) {
}

MovementType::MovementType(TribeSet tr, EnumSet<MovementTrait> t) : traits(t), tribeSet(tr) {
}

bool MovementType::hasTrait(MovementTrait t) const {
  return traits.contains(t);
}

bool MovementType::operator == (const MovementType& o) const {
  return traits == o.traits && tribeSet == o.tribeSet && sunlightVulnerable == o.sunlightVulnerable &&
      fireResistant == o.fireResistant && forced == o.forced && destroyActions == o.destroyActions &&
      buildBridge == o.buildBridge && prisoner == o.prisoner;
}

const EnumSet<MovementTrait>& MovementType::getTraits() const {
  return traits;
}

int MovementType::getHash() const {
  return destroyActions.getHash() + traits.getHash() + sunlightVulnerable + 2 * fireResistant + 4 * forced +
      8 * buildBridge + prisoner * 16;
}

bool MovementType::isCompatible(TribeId id) const {
  return !tribeSet || tribeSet->contains(id);
}

MovementType& MovementType::removeTrait(MovementTrait trait) {
  traits.erase(trait);
  return *this;
}

MovementType& MovementType::addTrait(MovementTrait trait) {
  traits.insert(trait);
  return *this;
}

MovementType& MovementType::setSunlightVulnerable(bool state) {
  sunlightVulnerable = state;
  return *this;
}

MovementType& MovementType::setFireResistant(bool state) {
  fireResistant = state;
  return *this;
}

MovementType& MovementType::setForced(bool state) {
  forced = state;
  return *this;
}

MovementType& MovementType::setDestroyActions(EnumSet<DestroyAction::Type> d) {
  destroyActions = d;
  return *this;
}

MovementType& MovementType::setCanBuildBridge(bool can) {
  buildBridge = can;
  return *this;
}

MovementType& MovementType::setPrisoner(bool p) {
  prisoner = p;
  return *this;
}

const EnumSet<DestroyAction::Type>& MovementType::getDestroyActions() const {
  return destroyActions;
}

bool MovementType::isSunlightVulnerable() const {
  return sunlightVulnerable;
}

bool MovementType::isFireResistant() const {
  return fireResistant;
}

bool MovementType::isForced() const {
  return forced;
}

bool MovementType::canBuildBridge() const {
  return buildBridge;
}

bool MovementType::isPrisoner() const {
  return prisoner;
}

