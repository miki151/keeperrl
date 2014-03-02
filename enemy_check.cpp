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

