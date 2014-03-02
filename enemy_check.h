#ifndef _ENEMY_CHECK
#define _ENEMY_CHECK

#include "creature.h"

class EnemyCheck {
  public:
  EnemyCheck(double weight);
  double getWeight() const;
  virtual bool hasStanding(const Creature*) const = 0;
  virtual double getStanding(const Creature*) const = 0;

  static EnemyCheck* allEnemies(double weight);
  static EnemyCheck* friendlyAnimals(double weight);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  SERIALIZATION_DECL(EnemyCheck);

  private:
  double weight;
};

#endif
