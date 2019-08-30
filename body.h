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

RICH_ENUM(BodyMaterial,
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
  ADA
);

RICH_ENUM(BodySize,
  SMALL,
  MEDIUM,
  LARGE,
  HUGE
);


class Body {
  public:

  using Material = BodyMaterial;
  using Size = BodySize;

  static Body humanoid(Material, Size);
  static Body humanoid(Size);
  static Body nonHumanoid(Material, Size);
  static Body nonHumanoid(Size);
  static Body humanoidSpirit(Size);
  static Body nonHumanoidSpirit(Size);
  Body(bool humanoid, Material, Size);
  // To add once the creature has been constructed, use CreatureAttributes::add().
  void addWithoutUpdatingPermanentEffects(BodyPart, int cnt);
  void setWeight(double);
  void setSize(BodySize);
  void setBodyParts(const EnumMap<BodyPart, int>&);
  void setHumanoidBodyParts(int intrinsicDamage);
  void setHorseBodyParts(int intrinsicDamage);
  void setBirdBodyParts(int intrinsicDamage);
  void setMinionFood();
  void setDeathSound(optional<SoundId>);
  void setIntrinsicAttack(BodyPart, IntrinsicAttack);
  void setMinPushSize(Size);
  void setHumanoid(bool);
  void affectPosition(Position);

  enum DamageResult {
    NOT_HURT,
    HURT,
    KILLED
  };
  DamageResult takeDamage(const Attack&, Creature*, double damage);

  bool tick(const Creature*);
  bool heal(Creature*, double amount);
  bool affectByFire(Creature*, double amount);
  bool isIntrinsicallyAffected(LastingEffect) const;
  bool affectByPoisonGas(Creature*, double amount);
  bool affectBySilver(Creature*);
  bool affectByAcid(Creature*);
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
  bool canPush(const Body& other);
  bool canPerformRituals() const;
  bool canBeCaptured() const;
  vector<PItem> getCorpseItems(const string& name, UniqueEntity<Creature>::Id, bool instantlyRotten,
      const ContentFactory* factory) const;

  vector<AttackLevel> getAttackLevels() const;
  int getAttrBonus(AttrType) const;

  bool isCollapsed(const Creature*) const;
  int numGood(BodyPart) const;
  int numLost(BodyPart) const;
  int numBodyParts(BodyPart) const;
  void getBadAdjectives(vector<AdjectiveInfo>&) const;
  optional<Sound> getDeathSound() const;
  optional<AnimationId> getDeathAnimation() const;
  bool injureBodyPart(Creature*, BodyPart, bool drop);

  void healBodyParts(Creature*, bool regrow);
  bool fallsApartDueToLostBodyParts() const;
  bool canHeal(HealthType) const;
  bool isImmuneTo(LastingEffect effect) const;
  bool hasHealth(HealthType) const;
  bool hasAnyHealth() const;

  void consumeBodyParts(Creature*, Body& other, vector<string>& adjectives);

  bool isHumanoid() const;
  string getDescription() const;
  void updateViewObject(ViewObject&) const;
  int getCarryLimit() const;
  void bleed(Creature*, double amount);
  Item* chooseRandomWeapon(Item* realWeapon) const;
  Item* chooseFirstWeapon() const;
  const EnumMap<BodyPart, optional<IntrinsicAttack>>& getIntrinsicAttacks() const;
  EnumMap<BodyPart, optional<IntrinsicAttack>>& getIntrinsicAttacks();

  bool isUndead() const;
  double getBoulderDamage() const;

  SERIALIZATION_DECL(Body)
  template <class Archive>
  void serializeImpl(Archive& ar, const unsigned int);

  private:
  friend class Test;
  optional<BodyPart> getBodyPart(AttackLevel attack, bool flying, bool collapsed) const;
  BodyPart armOrWing() const;
  int numInjured(BodyPart) const;
  void clearInjured(BodyPart);
  void clearLost(BodyPart);
  bool looseBodyPart(BodyPart);
  bool injureBodyPart(BodyPart);
  void decreaseHealth(double amount);
  bool isPartDamaged(BodyPart, double damage) const;
  bool isCritical(BodyPart) const;
  PItem getBodyPartItem(const string& creatureName, BodyPart, const ContentFactory*);
  string getMaterialAndSizeAdjectives() const;
  bool SERIAL(xhumanoid) = false;
  Size SERIAL(size) = Size::LARGE;
  double SERIAL(weight) = 90;
  EnumMap<BodyPart, int> SERIAL(bodyParts) = {{BodyPart::TORSO, 1}, {BodyPart::BACK, 1}};
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
  Material SERIAL(material) = Material::FLESH;
  double SERIAL(health) = 1;
  bool SERIAL(minionFood) = false;
  optional<SoundId> SERIAL(deathSound);
  EnumMap<BodyPart, optional<IntrinsicAttack>> SERIAL(intrinsicAttacks);
  Size SERIAL(minPushSize);
  bool SERIAL(noHealth) = false;
  bool SERIAL(fallsApart) = true;
  optional<BodyPart> getAnyGoodBodyPart() const;
  double getBodyPartHealth() const;
  void dropUnsupportedEquipment(const Creature*) const;
  vector<pair<optional<ItemType>, double>> SERIAL(drops);
  optional<bool> SERIAL(canCapture);
};

