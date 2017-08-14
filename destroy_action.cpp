#include "stdafx.h"
#include "destroy_action.h"
#include "sound.h"
#include "creature.h"
#include "creature_attributes.h"

SERIALIZE_DEF(DestroyAction, type)
SERIALIZATION_CONSTRUCTOR_IMPL(DestroyAction)

DestroyAction::DestroyAction(Type t) : type(t) {
}

const char* DestroyAction::getVerbSecondPerson() const {
  switch (type) {
    case Type::BOULDER: return "destroy";
    case Type::BASH: return "bash";
    case Type::CUT: return "cut";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "dig into";
  }
}

const char* DestroyAction::getVerbThirdPerson() const {
  switch (type) {
    case Type::BASH: return "bashes";
    case Type::BOULDER: return "destroys";
    case Type::CUT: return "cuts";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "digs into";
  }
}

const char*DestroyAction::getIsDestroyed() const {
  switch (type) {
    case Type::BASH:
    case Type::BOULDER: return "is destroyed";
    case Type::CUT: return "falls";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "is dug out";
  }
}

const char* DestroyAction::getSoundText() const {
  switch (type) {
    case Type::BASH: return "BANG!";
    case Type::BOULDER:
    case Type::CUT: return "CRASH!";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "";
  }
}

Sound DestroyAction::getSound() const {
  switch (type) {
    case Type::BASH:
    case Type::BOULDER: return SoundId::BANG_DOOR;
    case Type::CUT: return SoundId::TREE_CUTTING;
    case Type::HOSTILE_DIG:
    case Type::DIG: return SoundId::DIGGING;
   }
}

DestroyAction::Type DestroyAction::getType() const {
  return type;
}

bool DestroyAction::canDestroyFriendly() const {
  switch (type) {
    case Type::BASH:
      return false;
    default:
      return true;
  }
}

bool DestroyAction::canNavigate(WConstCreature c) const {
  switch (type) {
    case Type::HOSTILE_DIG:
      return c->getAttributes().getSkills().hasDiscrete(SkillId::DIGGING);
    case Type::BASH:
      return true;
    default:
      return false;
  }
}
