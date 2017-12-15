#pragma once

#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "sound.h"
#include "item_type.h"
#include "body_part.h"
#include "intrinsic_attack.h"

#undef HUGE


class Attack;
struct AdjectiveInfo;

class Body {
  public:

  enum class Material {
    FLESH,
    SPIRIT,
    FIRE,
    WATER,
    UNDEAD_FLESH,
    BONE,
    ROCK,
    CLAY,
    WOOD,
    IRON,
    LAVA,
  };

  enum class Size {
    SMALL,
    MEDIUM,
    LARGE,
    HUGE
  };

  static Body humanoid(Material, Size);
  static Body humanoid(Size);
  static Body nonHumanoid(Material, Size);
  static Body nonHumanoid(Size);
  static Body humanoidSpirit(Size);
  static Body nonHumanoidSpirit(Size);
  Body(bool humanoid, Material, Size);
  void addWings();
  void setWeight(double);
  void setBodyParts(const EnumMap<BodyPart, int>&);
  void setHumanoidBodyParts(int intrinsicDamage);
  void setHorseBodyParts(int intrinsicDamage);
  void setBirdBodyParts(int intrinsicDamage);
  void setMinionFood();
  void setDeathSound(optional<SoundId>);
  void setNoCarryLimit();
  void setDoesntEat();
  void setIntrinsicAttack(BodyPart, IntrinsicAttack);

  void affectPosition(Position);

  bool takeDamage(const Attack&, WCreature, double damage);

  bool tick(WConstCreature);
  bool heal(WCreature, double amount);
  bool affectByFire(WCreature, double amount);
  bool isIntrinsicallyAffected(LastingEffect) const;
  bool affectByPoisonGas(WCreature, double amount);
  void affectByTorture(WCreature);
  bool affectBySilver(WCreature);
  bool affectByAcid(WCreature);
  bool isKilledByBoulder() const;
  bool canWade() const;
  bool isMinionFood() const;
  bool canCopulateWith() const;
  bool canConsume() const;
  bool isWounded() const;
  bool isSeriouslyWounded() const;
  double getHealth() const;
  bool hasBrain() const;
  bool needsToEat() const;
  bool needsToSleep() const;
  vector<PItem> getCorpseItems(const string& name, UniqueEntity<Creature>::Id, bool instantlyRotten) const;

  vector<AttackLevel> getAttackLevels() const;
  double modifyAttr(AttrType, double) const;

  bool isCollapsed(WConstCreature) const;
  int numGood(BodyPart) const;
  int numLost(BodyPart) const;
  int numBodyParts(BodyPart) const;
  void getBadAdjectives(vector<AdjectiveInfo>&) const;
  optional<Sound> getDeathSound() const;
  void injureBodyPart(WCreature, BodyPart, bool drop);

  void healBodyParts(WCreature, bool regrow);
  int lostOrInjuredBodyParts() const;
  bool canHeal() const;
  bool isImmuneTo(LastingEffect effect) const;
  bool hasHealth() const;

  void consumeBodyParts(WCreature, Body& other, vector<string>& adjectives);

  bool isHumanoid() const;
  string getDescription() const;
  void updateViewObject(ViewObject&) const;
  const optional<double>& getCarryLimit() const;
  void bleed(WCreature, double amount);
  WItem chooseWeapon(WItem realWeapon) const;
  const EnumMap<BodyPart, optional<IntrinsicAttack>>& getIntrinsicAttacks() const;
  EnumMap<BodyPart, optional<IntrinsicAttack>>& getIntrinsicAttacks();

  bool isUndead() const;
  double getBoulderDamage() const;

  SERIALIZATION_DECL(Body);

  private:
  friend class Test;
  BodyPart getBodyPart(AttackLevel attack, bool flying, bool collapsed) const;
  BodyPart armOrWing() const;
  int numInjured(BodyPart) const;
  void clearInjured(BodyPart);
  void clearLost(BodyPart);
  void looseBodyPart(BodyPart);
  void injureBodyPart(BodyPart);
  void decreaseHealth(double amount);
  bool isPartDamaged(BodyPart, double damage) const;
  bool isCritical(BodyPart) const;
  PItem getBodyPartItem(const string& creatureName, BodyPart);
  string getMaterialAndSizeAdjectives() const;
  bool fallsApartFromDamage() const;
  bool SERIAL(xhumanoid);
  Size SERIAL(size);
  double SERIAL(weight);
  EnumMap<BodyPart, int> SERIAL(bodyParts) = {{BodyPart::TORSO, 1}, {BodyPart::BACK, 1}};
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
  Material SERIAL(material);
  double SERIAL(health) = 1;
  bool SERIAL(minionFood) = false;
  optional<SoundId> SERIAL(deathSound);
  optional<double> SERIAL(carryLimit);
  bool SERIAL(doesntEat) = false;
  EnumMap<BodyPart, optional<IntrinsicAttack>> SERIAL(intrinsicAttacks);
};

