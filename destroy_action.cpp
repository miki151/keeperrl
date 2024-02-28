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
    case Type::FILL: return "fill";
  }
}

const char* DestroyAction::getVerbThirdPerson() const {
  switch (type) {
    case Type::BASH: return "bashes";
    case Type::BOULDER: return "destroys";
    case Type::CUT: return "cuts";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "digs into";
    case Type::FILL: return "fills";
  }
}

const char*DestroyAction::getIsDestroyed() const {
  switch (type) {
    case Type::BASH:
    case Type::BOULDER: return "is destroyed";
    case Type::CUT: return "falls";
    case Type::HOSTILE_DIG:
    case Type::DIG: return "is dug out";
    case Type::FILL: return "is filled";
  }
}

const char* DestroyAction::getSoundText() const {
  switch (type) {
    case Type::BASH: return "BANG!";
    case Type::BOULDER:
    case Type::CUT: return "CRASH!";
    case Type::HOSTILE_DIG:
    case Type::FILL:
    case Type::DIG: return "";
  }
}

optional<Sound> DestroyAction::getSound() const {
  switch (type) {
    case Type::FILL: return none;
    case Type::BASH:
    case Type::BOULDER: return Sound(SoundId("BANG_DOOR"));
    case Type::CUT: return Sound(SoundId("TREE_CUTTING"));
    case Type::HOSTILE_DIG:
    case Type::DIG: return Sound(SoundId("DIGGING"));
   }
}

DestroyAction::Type DestroyAction::getType() const {
  return type;
}

bool DestroyAction::canDestroyFriendly() const {
  switch (type) {
    case Type::BASH:
    case Type::HOSTILE_DIG:
      return false;
    default:
      return true;
  }
}

bool DestroyAction::destroyAnimation() const {
  switch (type) {
    case Type::FILL: return false;
    default: return true;
  }
}

bool DestroyAction::canNavigate(const Creature* c) const {
  switch (type) {
    case Type::HOSTILE_DIG:
      return !isOneOf(c->getTribeId(), TribeId::getDarkKeeper(), TribeId::getWhiteKeeper(),
          TribeId::getRetiredKeeper());
    case Type::BASH:
      return true;
    default:
      return false;
  }
}

MinionActivity DestroyAction::getMinionActivity() const {
  switch (type) {
    case Type::DIG:
      return MinionActivity::DIGGING;
    default:
      return MinionActivity::WORKING;
  }
}

double DestroyAction::getDamage(Creature* c) const {
  PROFILE;
  switch (type) {
    case Type::BASH:
    case Type::BOULDER:
    case Type::CUT:
      return max(c->getAttr(AttrType("DAMAGE")), 15);
    case Type::HOSTILE_DIG:
      return max(c->getAttr(AttrType("DIGGING")), 15) / 5.0;
    case Type::DIG:
    case Type::FILL:
      return c->getAttr(AttrType("DIGGING")) / 5.0;
   }
}