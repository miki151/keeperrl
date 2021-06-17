#pragma once

#include "util.h"
#include "tribe.h"
#include "destroy_action.h"

RICH_ENUM(MovementTrait,
  WALK,
  FLY,
  SWIM,
  WADE
);

class MovementType {
  public:
  MovementType(EnumSet<MovementTrait> = {});
  MovementType(MovementTrait);
  MovementType(TribeSet, EnumSet<MovementTrait>);
  bool hasTrait(MovementTrait) const;
  bool isCompatible(TribeId) const;
  MovementType& removeTrait(MovementTrait);
  MovementType& addTrait(MovementTrait);
  MovementType& setSunlightVulnerable(bool = true);
  MovementType& setFireResistant(bool = true);
  MovementType& setForced(bool = true);
  MovementType& setDestroyActions(EnumSet<DestroyAction::Type>);
  MovementType& setCanBuildBridge(bool = true);
  MovementType& setPrisoner(bool = true);
  const EnumSet<DestroyAction::Type>& getDestroyActions() const;

  bool isSunlightVulnerable() const;
  bool isFireResistant() const;
  bool isForced() const;
  bool canBuildBridge() const;
  bool isPrisoner() const;

  int getHash() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  bool operator == (const MovementType&) const;
  const EnumSet<MovementTrait>& getTraits() const;

  private:
  bool SERIAL(sunlightVulnerable) = false;
  bool SERIAL(fireResistant) = false;
  bool SERIAL(forced) = false;
  bool SERIAL(buildBridge) = false;
  bool SERIAL(prisoner) = false;
  EnumSet<MovementTrait> SERIAL(traits);
  EnumSet<DestroyAction::Type> SERIAL(destroyActions);
  optional<TribeSet> SERIAL(tribeSet);
};
