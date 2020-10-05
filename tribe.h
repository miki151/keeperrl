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

#pragma once

#include <map>
#include <set>

#include "enums.h"
#include "singleton.h"
#include "entity_map.h"
#include "hashing.h"

class Creature;

class TribeId {
  public:
  static TribeId getMonster();
  static TribeId getPest();
  static TribeId getWildlife();
  static TribeId getHuman();
  static TribeId getElf();
  static TribeId getDarkElf();
  static TribeId getDwarf();
  static TribeId getGnome();
  static TribeId getAdventurer();
  static TribeId getBandit();
  static TribeId getHostile();
  static TribeId getShelob();
  static TribeId getPeaceful();
  static TribeId getDarkKeeper();
  static TribeId getRetiredKeeper();
  static TribeId getLizard();
  static TribeId getGreenskin();
  static TribeId getAnt();

  bool operator == (const TribeId&) const;
  bool operator != (const TribeId&) const;

  enum class KeyType : std::uint8_t;
  KeyType getKey() const;

  int getHash() const;

  // This is a ridiculous, but effective hack to switch one tribe for another in entire model before retiring a game.
  static void switchForSerialization(TribeId from, TribeId to);
  static void clearSwitch();

  SERIALIZATION_DECL(TribeId)

  TribeId(KeyType key);

  private:
  friend class TribeSet;
  KeyType SERIAL(key);
};

class TribeSet {
  public:
  static TribeSet getFull();
  void clear();
  TribeSet& insert(TribeId);
  TribeSet& erase(TribeId);
  bool contains(TribeId) const;
  int getHash() const;

  bool operator==(const TribeSet&) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  bitset<32> SERIAL(elems);
};

class Tribe {
  public:
  Tribe(const Tribe&) = delete;
  Tribe& operator = (Tribe&&) = default;
  Tribe(Tribe&&) = default;
  bool isEnemy(const Creature*) const;
  bool isEnemy(const Tribe*) const;
  void addEnemy(Tribe*);
  const TribeSet& getFriendlyTribes() const;

  void onMemberKilled(Creature* member, Creature* killer);
  void onItemsStolen(Creature* thief);

  SERIALIZATION_DECL(Tribe)

  typedef unordered_map<TribeId, PTribe, CustomHash<TribeId>> Map;

  static Map generateTribes();

  private:
  Tribe(TribeId, bool diplomatic);
  static void init(Tribe::Map&, TribeId, bool diplomatic);
  double getStanding(const Creature*) const;

  bool SERIAL(diplomatic);

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  EntityMap<Creature, double> SERIAL(standing);
  TribeSet SERIAL(friendlyTribes);
  TribeId SERIAL(id);
};

RICH_ENUM2(std::uint8_t,
    TribeId::KeyType,
    MONSTER,
    PEST,
    WILDLIFE,
    HUMAN,
    ELF,
    DARK_ELF,
    DWARF,
    GNOME,
    ADVENTURER,
    BANDIT,
    HOSTILE,
    PEACEFUL,
    DARK_KEEPER,
    RETIRED_KEEPER,
    LIZARD,
    GREENSKIN,
    ANT,
    SHELOB
);
