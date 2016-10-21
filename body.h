#pragma once

#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "sound.h"

#undef HUGE

RICH_ENUM(BodyPart,
  HEAD,
  TORSO,
  ARM,
  WING,
  LEG,
  BACK
);

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

  Body& addWings();
  Body& setWeight(double);
  Body& setBodyParts(const EnumMap<BodyPart, int>&);
  Body& setHumanoidBodyParts();
  Body& setHorseBodyParts();
  Body& setBirdBodyParts();
  Body& setMinionFood();
  Body& setDeathSound(optional<SoundId>);

  void affectPosition(Position);

  bool takeDamage(const Attack&, Creature*, double damage);


  bool tick(const Creature*);
  bool heal(Creature*, double amount, bool replaceLimbs);
  void fireDamage(Creature*, double amount);
  bool isIntrinsicallyAffected(LastingEffect) const;
  bool affectByPoison(Creature*, double amount);
  bool affectByPoisonGas(Creature*, double amount);
  void affectByTorture(Creature*);
  bool affectBySilver(Creature*);
  bool affectByAcid(Creature*);
  bool isKilledByBoulder() const;
  bool canWade() const;
  bool isMinionFood() const;
  bool canCopulateWith() const;
  bool canConsume() const;
  bool isSunlightVulnerable() const;
  bool isWounded() const;
  bool isSeriouslyWounded() const;
  bool canEntangle() const;
  double getHealth() const;
  bool hasBrain() const;
  vector<PItem> getCorpseItem(const string& name, UniqueEntity<Creature>::Id);

  vector<AttackLevel> getAttackLevels() const;
  double modifyAttr(AttrType, double) const;

  bool isCollapsed(const Creature*) const;
  int numGood(BodyPart) const;
  int numLost(BodyPart) const;
  int numBodyParts(BodyPart) const;
  void getBadAdjectives(vector<AdjectiveInfo>&) const;
  optional<Sound> getDeathSound() const;

  void healLimbs(Creature*, bool regrow);
  int lostOrInjuredBodyParts() const;
  bool canHeal() const;
  bool hasHealth() const;

  static const char* getBodyPartName(BodyPart);

  void consumeBodyParts(Creature*, const Body& other, vector<string>& adjectives);

  bool isHumanoid() const;
  string getDescription() const;
  void updateViewObject(ViewObject&) const;

  bool isUndead() const;
  double getBoulderDamage() const;


  SERIALIZATION_DECL(Body);

  private:
  Size getSize() const;
  void injureBodyPart(Creature*, BodyPart, bool drop);
  BodyPart getBodyPart(AttackLevel attack, bool flying, bool collapsed) const;
  BodyPart armOrWing() const;
  int numInjured(BodyPart) const;
  void clearInjured(BodyPart);
  void clearLost(BodyPart);
  void looseBodyPart(BodyPart);
  void injureBodyPart(BodyPart);
  void decreaseHealth(double amount);
  double getMinDamage(BodyPart) const;
  bool isCritical(BodyPart) const;
  PItem getBodyPartItem(const string& creatureName, BodyPart);
  bool isBleeding() const;
  void bleed(Creature*, double amount);
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
};

