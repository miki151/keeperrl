#pragma once

#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "sound.h"
#include "item_type.h"
#include "body_part.h"
#include "intrinsic_attack.h"
#include "effect.h"
#include "furniture_type.h"
#include "body_material_id.h"
#include "lasting_or_buff.h"

#undef HUGE


class Attack;
struct AdjectiveInfo;

RICH_ENUM(BodySize,
  SMALL,
  MEDIUM,
  LARGE,
  HUGE
);

const char* getName(BodySize);

class Body {
  public:

  using Size = BodySize;

  static Body humanoid(BodyMaterialId, Size);
  static Body humanoid(Size);
  static Body nonHumanoid(BodyMaterialId, Size);
  static Body nonHumanoid(Size);
  static Body humanoidSpirit(Size);
  static Body nonHumanoidSpirit(Size);
  Body(bool humanoid, BodyMaterialId, Size);
  // To add once the creature has been constructed, use CreatureAttributes::add().
  void addWithoutUpdatingPermanentEffects(BodyPart, int cnt);
  void setWeight(double);
  void setSize(BodySize);
  void setBodyParts(const EnumMap<BodyPart, int>&);
  void setHumanoidBodyParts(int intrinsicDamage);
  void setHorseBodyParts(int intrinsicDamage);
  void setBirdBodyParts(int intrinsicDamage);
  void setDeathSound(optional<SoundId>);
  void addIntrinsicAttack(BodyPart, IntrinsicAttack);
  void initializeIntrinsicAttack(const ContentFactory*);
  void setMinPushSize(Size);
  void setHumanoid(bool);
  void affectPosition(Position);
  void addBodyPart(BodyPart, int count);
  void setCanBeCaptured(bool);

  enum DamageResult {
    NOT_HURT,
    HURT,
    KILLED
  };
  DamageResult takeDamage(const Attack&, Creature*, double damage);

  bool tick(const Creature*);
  bool heal(Creature*, double amount);
  bool isIntrinsicallyAffected(LastingOrBuff, const ContentFactory*) const;
  bool isKilledByBoulder(const ContentFactory*) const;
  bool canWade() const;
  bool isFarmAnimal() const;
  bool canCopulateWith(const ContentFactory*) const;
  bool canConsume() const;
  bool isWounded() const;
  bool isSeriouslyWounded() const;
  double getHealth() const;
  double getBodyPartHealth() const;
  bool hasBrain(const ContentFactory*) const;
  bool needsToEat() const;
  bool burnsIntrinsically(const ContentFactory*) const;
  bool canPush(const Body& other);
  bool canPerformRituals(const ContentFactory*) const;
  bool canBeCaptured(const ContentFactory*) const;
  vector<PItem> getCorpseItems(const string& name, UniqueEntity<Creature>::Id, bool instantlyRotten,
      const ContentFactory* factory, Game*) const;
  vector<AttackLevel> getAttackLevels() const;
  BodySize getSize() const;
  int numGood(BodyPart) const;
  int numLost(BodyPart) const;
  int numInjured(BodyPart) const;
  int numBodyParts(BodyPart) const;
  void getBadAdjectives(vector<AdjectiveInfo>&) const;
  optional<Sound> getDeathSound() const;
  optional<AnimationId> getDeathAnimation(const ContentFactory*) const;
  bool injureBodyPart(Creature*, BodyPart, bool drop);

  bool healBodyParts(Creature*, int max);
  bool fallsApartDueToLostBodyParts(const ContentFactory*) const;
  bool canHeal(HealthType, const ContentFactory*) const;
  bool isImmuneTo(LastingOrBuff, const ContentFactory*) const;
  bool hasHealth(HealthType, const ContentFactory*) const;
  bool hasAnyHealth(const ContentFactory*) const;
  FurnitureType getDiningFurniture() const;

  const char* getDeathDescription(const ContentFactory*) const;

  void consumeBodyParts(Creature*, Body& other, vector<string>& adjectives);

  BodyMaterialId getMaterial() const;
  bool isHumanoid() const;
  bool canPickUpItems() const;
  string getDescription(const ContentFactory*) const;
  void updateViewObject(ViewObject&, const ContentFactory*) const;
  int getCarryLimit() const;
  void bleed(Creature*, double amount);
  vector<pair<Item*, double>> chooseRandomWeapon(vector<Item*> weapons, vector<double> multipliers) const;
  Item* chooseFirstWeapon() const;
  const EnumMap<BodyPart, vector<IntrinsicAttack> >& getIntrinsicAttacks() const;
  EnumMap<BodyPart, vector<IntrinsicAttack> >& getIntrinsicAttacks();

  bool isUndead(const ContentFactory*) const;
  double getBoulderDamage() const;

  SERIALIZATION_DECL(Body)
  template <class Archive>
  void serializeImpl(Archive& ar, const unsigned int);

  private:
  friend class Test;
  optional<BodyPart> getBodyPart(AttackLevel attack, bool flying, bool collapsed) const;
  BodyPart armOrWing() const;
  void clearInjured(BodyPart);
  void clearLost(BodyPart);
  bool looseBodyPart(BodyPart, const ContentFactory*);
  bool injureBodyPart(BodyPart, const ContentFactory*);
  void decreaseHealth(double amount);
  bool isPartDamaged(BodyPart, double damage, const ContentFactory*) const;
  bool isCritical(BodyPart, const ContentFactory*) const;
  PItem getBodyPartItem(const string& creatureName, BodyPart, const ContentFactory*) const;
  string getMaterialAndSizeAdjectives(const ContentFactory*) const;
  bool SERIAL(xhumanoid) = false;
  bool SERIAL(xCanPickUpItems) = false;
  Size SERIAL(size) = Size::LARGE;
  double SERIAL(weight) = 90;
  EnumMap<BodyPart, int> SERIAL(bodyParts) = {{BodyPart::TORSO, 1}, {BodyPart::BACK, 1}};
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
  BodyMaterialId SERIAL(material) = BodyMaterialId("FLESH");
  double SERIAL(health) = 1;
  bool SERIAL(minionFood) = false;
  optional<SoundId> SERIAL(deathSound);
  EnumMap<BodyPart, vector<IntrinsicAttack>> SERIAL(intrinsicAttacks);
  Size SERIAL(minPushSize);
  bool SERIAL(noHealth) = false;
  bool SERIAL(fallsApart) = true;
  optional<BodyPart> getAnyGoodBodyPart() const;
  void dropUnsupportedEquipment(const Creature*) const;
  vector<pair<optional<ItemType>, double>> SERIAL(drops);
  optional<bool> SERIAL(canCapture);
  optional<Effect> SERIAL(droppedPartUpgrade);
  optional<string> SERIAL(corpseIngredientType);
  bool SERIAL(canBeRevived) = true;
  optional<FurnitureType> SERIAL(overrideDiningFurniture);
};