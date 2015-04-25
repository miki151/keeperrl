/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */
#ifndef _COLLECTIVE_CONFIG_INFO_H
#define _COLLECTIVE_CONFIG_INFO_H

#include "enum_variant.h"
#include "square_type.h"
#include "util.h"
#include "minion_task.h"
#include "model.h"

enum class ItemClass;

enum class AttractionId {
  SQUARE,
  ITEM_INDEX,
};

typedef EnumVariant<AttractionId, TYPES(SquareType, ItemIndex),
    ASSIGN(SquareType, AttractionId::SQUARE),
    ASSIGN(ItemIndex, AttractionId::ITEM_INDEX)> MinionAttraction;

namespace std {
  template <> struct hash<MinionAttraction> {
    size_t operator()(const MinionAttraction& t) const {
      return (size_t)t.getId();
    }
  };
}

struct AttractionInfo {
  AttractionInfo(MinionAttraction, double amountClaimed, double minAmount, bool mandatory = false);

  SERIALIZATION_DECL(AttractionInfo);

  MinionAttraction SERIAL(attraction);
  double SERIAL(amountClaimed);
  double SERIAL(minAmount);
  bool SERIAL(mandatory);
};

struct ImmigrantInfo {
  CreatureId SERIAL(id);
  double SERIAL(frequency);
  vector<AttractionInfo> SERIAL(attractions);
  EnumSet<MinionTrait> SERIAL(traits);
  bool SERIAL(spawnAtDorm);
  int SERIAL(salary);
  optional<TechId> SERIAL(techId);
  optional<Model::SunlightInfo::State> SERIAL(limit);
  optional<Range> SERIAL(groupSize);
  bool SERIAL(autoTeam);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

class CollectiveConfig {
  public:
  static CollectiveConfig keeper(double immigrantFrequency, int payoutTime, double payoutMultiplier,
      vector<ImmigrantInfo>);
  static CollectiveConfig withImmigrants(double immigrantFrequency, int maxPopulation, vector<ImmigrantInfo>);
  static CollectiveConfig noImmigrants();

  bool getManageEquipment() const;
  bool getWorkerFollowLeader() const;
  double getImmigrantFrequency() const;
  int getPayoutTime() const;
  double getPayoutMultiplier() const;
  bool getStripSpawns() const;
  bool getFetchItems() const;
  bool getEnemyPositions() const;
  bool getWarnings() const;
  bool getConstructions() const;
  int getMaxPopulation() const;
  bool sleepOnlyAtNight() const;
  const vector<ImmigrantInfo>& getImmigrantInfo() const;

  SERIALIZATION_DECL(CollectiveConfig);

  private:
  enum CollectiveType { KEEPER, VILLAGE };
  CollectiveConfig(double immigrantFrequency, int payoutTime, double payoutMultiplier, int maxPopulation,
      vector<ImmigrantInfo>, CollectiveType);

  double SERIAL(immigrantFrequency);
  int SERIAL(payoutTime);
  double SERIAL(payoutMultiplier);
  int SERIAL(maxPopulation);
  vector<ImmigrantInfo> SERIAL(immigrantInfo);
  CollectiveType SERIAL(type);
};


#endif

