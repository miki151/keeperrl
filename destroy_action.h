#pragma once

#include "util.h"

class Sound;
class Skill;

class DestroyAction {
  public:
  enum class Type;
  DestroyAction(Type);
  const char* getVerbSecondPerson() const;
  const char* getVerbThirdPerson() const;
  const char* getIsDestroyed() const;
  const char* getSoundText() const;
  Sound getSound() const;
  Type getType() const;
  bool canDestroyFriendly() const;
  bool canNavigate(WConstCreature) const;
  MinionActivity getMinionActivity() const;
  optional<SkillId> getDestroyingSkillMultiplier() const;

  SERIALIZATION_DECL(DestroyAction)

  private:
  Type SERIAL(type);
};

RICH_ENUM(DestroyAction::Type,
  BOULDER,
  BASH,
  CUT,
  DIG,
  HOSTILE_DIG,
  HOSTILE_DIG_NO_SKILL
);
