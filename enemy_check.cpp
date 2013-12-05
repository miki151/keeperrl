#include "stdafx.h"


EnemyCheck::EnemyCheck(double w) : weight(w){}

double EnemyCheck::getWeight() const {
  return weight;
}

class AllEnemies : public EnemyCheck {
  public:
  AllEnemies(double weight) : EnemyCheck(weight) {}

  virtual bool hasStanding(const Creature*) const override {
    return true;
  }

  virtual double getStanding(const Creature*) const override {
    return -1;
  }
};

EnemyCheck* EnemyCheck::allEnemies(double weight) {
  return new AllEnemies(weight);
}

class FriendlyAnimals : public EnemyCheck {
  public:
  FriendlyAnimals(double weight) : EnemyCheck(weight) {}

  virtual bool hasStanding(const Creature* c) const override {
    return c->isAnimal();
  }

  virtual double getStanding(const Creature* c) const override {
    CHECK(c->isAnimal());
    return 1;
  }
};

EnemyCheck* EnemyCheck::friendlyAnimals(double weight) {
  return new FriendlyAnimals(weight);
}
