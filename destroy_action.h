#pragma once

#include "util.h"

class Sound;
class TStringId;

class DestroyAction {
  public:
  enum class Type;
  DestroyAction(Type);
  TStringId getVerbSecondPerson() const;
  TStringId getVerbThirdPerson() const;
  TStringId getIsDestroyed() const;
  optional<TStringId> getSoundText() const;
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
