#pragma once

#include "util.h"

class Sound;

class DestroyAction {
  public:
  enum class Type;
  DestroyAction(Type);
  const char* getVerbSecondPerson() const;
  const char* getVerbThirdPerson() const;
  const char* getIsDestroyed() const;
  const char* getSoundText() const;
  optional<Sound> getSound() const;
  Type getType() const;
  bool canDestroyFriendly() const;
  bool canNavigate(const Creature*) const;
  bool destroyAnimation() const;
  MinionActivity getMinionActivity() const;
  double getDamage(Creature*) const;

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
  FILL
);
