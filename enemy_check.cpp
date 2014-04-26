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

#include "stdafx.h"

#include "enemy_check.h"


template <class Archive> 
void EnemyCheck::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(weight);
}

SERIALIZABLE(EnemyCheck);

EnemyCheck::EnemyCheck(double w) : weight(w){}

double EnemyCheck::getWeight() const {
  return weight;
}

class AllEnemies : public EnemyCheck {
  public:
  AllEnemies() {}
  AllEnemies(double weight) : EnemyCheck(weight) {}

  virtual bool hasStanding(const Creature*) const override {
    return true;
  }

  virtual double getStanding(const Creature*) const override {
    return -1;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(EnemyCheck);
  }
};

EnemyCheck* EnemyCheck::allEnemies(double weight) {
  return new AllEnemies(weight);
}

class FriendlyAnimals : public EnemyCheck {
  public:
  FriendlyAnimals() {}
  FriendlyAnimals(double weight) : EnemyCheck(weight) {}

  virtual bool hasStanding(const Creature* c) const override {
    return c->isAnimal();
  }

  virtual double getStanding(const Creature* c) const override {
    CHECK(c->isAnimal());
    return 1;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(EnemyCheck);
  }
};

EnemyCheck* EnemyCheck::friendlyAnimals(double weight) {
  return new FriendlyAnimals(weight);
}

template <class Archive>
void EnemyCheck::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, AllEnemies);
  REGISTER_TYPE(ar, FriendlyAnimals);
}

REGISTER_TYPES(EnemyCheck);

