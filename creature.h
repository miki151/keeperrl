#ifndef _CREATURE_H
#define _CREATURE_H

#include "creature_attributes.h"
#include "view_object.h"
#include "equipment.h"
#include "enums.h"
#include "attack.h"
#include "shortest_path.h"
#include "tribe.h"
#include "skill.h"
#include "map_memory.h"
#include "creature_view.h"
#include "controller.h"
#include "unique_entity.h"
#include "event.h"

class Level;
class Tribe;

class Creature : public CreatureAttributes, public CreatureView, public UniqueEntity, public EventListener {
  public:
  typedef CreatureAttributes CreatureAttributes;
  Creature(Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  Creature(ViewObject, Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  virtual ~Creature();

  static Creature* getDefault();
  static void noExperienceLevels();
  static void initialize();

  const ViewObject& getViewObject() const;
  virtual ViewIndex getViewIndex(Vec2 pos) const override;
  void makeMove();
  double getTime() const;
  void setTime(double t);
  void setLevel(Level* l);
  virtual const Level* getLevel() const;
  Level* getLevel();
  const Square* getConstSquare() const;
  const Square* getConstSquare(Vec2 direction) const;
  Square* getSquare();
  Square* getSquare(Vec2 direction);
  void setPosition(Vec2 pos);
  virtual Vec2 getPosition() const override;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void heal(double amount = 1, bool replaceLimbs = false);
  double getHealth() const;
  double getWeight() const;
  bool canSleep() const;
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<Item*> items);
  virtual bool canSee(const Creature*) const override;
  virtual bool canSee(Vec2 pos) const override;
  virtual bool isEnemy(const Creature*) const override;
  void tick(double realTime);

  string getTheName() const;
  string getAName() const;
  string getName() const;
  string getSpeciesName() const;
  string getNameAndTitle() const;
  Optional<string> getFirstName() const;
  int getAttr(AttrType) const;
  int getPoints() const;
  vector<string> getMainAdjectives() const;
  vector<string> getAdjectives() const;
  VisionInfo getVisionInfo() const;
  void onKillEvent(const Creature* victim, const Creature* killer) override;

  virtual Tribe* getTribe() const override;
  bool isFriend(const Creature*) const;
  void addEnemyCheck(EnemyCheck*);
  void removeEnemyCheck(EnemyCheck*);
  int getDebt(const Creature* debtor) const;
  vector<Item*> getGold(int num) const;

  void takeItems(vector<PItem> items, const Creature* from);

  void youHit(BodyPart part, AttackType type) const;

  virtual void dropCorpse();
  
  void globalMessage(const string& playerCanSee, const string& cant = "") const;

  bool isDead() const;
  const Creature* getLastAttacker() const;
  vector<const Creature*> getKills() const;
  bool isHumanoid() const;
  bool isAnimal() const;
  bool isStationary() const;
  void setStationary();
  bool isInvincible() const;
  bool isUndead() const;
  bool hasBrain() const;
  bool isNotLiving() const;
  bool canSwim() const;
  bool canFly() const;
  bool canWalk() const;
  int numArms() const;
  int numLegs() const;
  int numWings() const;
  int numHeads() const;
  bool lostLimbs() const;
  int numLostOrInjuredBodyParts() const;
  int numGoodArms() const;
  int numGoodLegs() const;
  int numGoodHeads() const;
  double getCourage() const;
  Gender getGender() const;

  void increaseExpLevel(double);
  int getExpLevel() const;
  int getDifficultyPoints() const;

  string getDescription() const;
  bool isSpecialMonster() const;

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  bool hasSkillToUseWeapon(const Item*) const;
  vector<Skill*> getSkills() const;

  bool canMove(Vec2 direction) const;
  void move(Vec2 direction);
  bool canSwapPosition(Vec2 direction) const;
  void swapPosition(Vec2 direction);
  void wait();
  vector<Item*> getPickUpOptions() const;
  bool canPickUp(const vector<Item*>& item) const;
  void pickUp(const vector<Item*>& item, bool spendTime = true);
  void drop(const vector<Item*>& item);
  void drop(vector<PItem> item);
  bool canAttack(const Creature*) const;
  void attack(const Creature*, bool spendTime = true);
  bool canBumpInto(Vec2 direction) const;
  void bumpInto(Vec2 direction);
  void applyItem(Item* item);
  bool canApplyItem(const Item* item) const;
  void startEquipChain();
  void finishEquipChain();
  void equip(Item* item);
  void unequip(Item* item);
  bool canEquip(const Item* item, string* reason) const;
  bool canEquipIfEmptySlot(const Item* item, string* reason) const;
  bool canUnequip(const Item* item, string* reason) const;
  bool canThrowItem(Item*);
  void throwItem(Item*, Vec2 direction);
  bool canHeal(Vec2 direction) const;
  void heal(Vec2 direction);
  void applySquare();
  bool canHide() const;
  void hide();
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  bool isBlind() const;
  bool canFlyAway() const;
  void flyAway();
  void torture(Creature*);
  bool canChatTo(Vec2 direction) const;
  void chatTo(Vec2 direction);
  void stealFrom(Vec2 direction, const vector<Item*>&);
  void give(const Creature* whom, vector<Item*> items);
  bool canFire(Vec2 direction) const;
  void fire(Vec2 direction);
  void squash(Vec2 direction);
  void construct(Vec2 direction, SquareType);
  bool canConstruct(Vec2 direction, SquareType) const;
  bool canConstruct(SquareType) const;
  void eat(Item*);
  bool canDestroy(Vec2 direction) const;
  void destroy(Vec2 direction);
  
  virtual void onChat(Creature*);

  void learnLocation(const Location*);

  Item* getWeapon() const;

  Optional<Vec2> getMoveTowards(Vec2 pos, bool avoidEnemies = false);
  Optional<Vec2> getMoveAway(Vec2 pos, bool pathfinding = true);
  Optional<Vec2> continueMoving();
  bool atTarget() const;
  void die(const Creature* attacker = nullptr, bool dropInventory = true, bool dropCorpse = true);
  void bleed(double severity);
  void setOnFire(double amount);
  void poisonWithGas(double amount);
  void shineLight();
  void setHeld(const Creature* holding);
  bool isHeld() const;

  virtual vector<const Creature*> getUnknownAttacker() const override;
  virtual void refreshGameInfo(View::GameInfo&) const override;
  
  void you(MsgType type, const string& param) const;
  void you(const string& param) const;
  void privateMessage(const string& message) const;
  bool isPlayer() const;
  const MapMemory& getMemory() const override;
  void grantIdentify(int numItems);

  Controller* getController();
  void pushController(Controller*);
  void popController();
  bool canPopController() const;
  void setSpeed(double);
  double getSpeed() const;
  CreatureSize getSize() const;

  class Vision {
    public:
    virtual bool canSee(const Creature*, const Creature*) = 0;
    virtual ~Vision() {}

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };

  void addVision(Vision*);
  void removeVision(Vision*);

  void addSpell(SpellId);
  const vector<SpellInfo>& getSpells() const;
  bool canCastSpell(int index) const;
  void castSpell(int index);
  static SpellInfo getSpell(SpellId);

  SERIALIZATION_DECL(Creature);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & defaultCreature;
  }

  enum LastingEffect {
    SLEEP, PANIC, RAGE, SLOWED, SPEED, STR_BONUS, DEX_BONUS, HALLU, BLIND, INVISIBLE, POISON, ENTANGLED, STUNNED };

  void addEffect(LastingEffect, double time, bool msg = true);
  void removeEffect(LastingEffect, bool msg = true);
  bool isAffected(LastingEffect) const;

  private:
  bool affects(LastingEffect effect) const;
  void onAffected(LastingEffect effect, bool msg);
  void onRemoved(Creature::LastingEffect effect, bool msg);
  void onTimedOut(Creature::LastingEffect effect, bool msg);
  static PCreature defaultCreature;
  Optional<Vec2> getMoveTowards(Vec2 pos, bool away, bool avoidEnemies);
  double getInventoryWeight() const;
  Item* getAmmo() const;
  void updateViewObject();
  int getStrengthAttackBonus() const;
  int getAttrVal(AttrType type) const;
  int getToHit() const;
  BodyPart getBodyPart(AttackLevel attack) const;
  bool isFireResistant() const;
  void injureLeg(bool drop);
  void injureArm(bool drop);
  void injureWing(bool drop);
  void injureHead(bool drop);
  AttackLevel getRandomAttackLevel() const;
  AttackType getAttackType() const;
  void spendTime(double time);
  BodyPart armOrWing() const;
  pair<double, double> getStanding(const Creature* c) const;

  ViewObject SERIAL(viewObject);
  Level* SERIAL2(level, nullptr);
  Vec2 SERIAL(position);
  double SERIAL2(time, 0);
  Equipment SERIAL(equipment);
  Optional<ShortestPath> SERIAL(shortestPath);
  unordered_set<const Creature*> SERIAL(knownHiding);
  Tribe* SERIAL(tribe);
  vector<EnemyCheck*> SERIAL(enemyChecks);
  double SERIAL2(health, 1);
  bool SERIAL2(dead, false);
  double SERIAL2(lastTick, 0);
  bool SERIAL2(collapsed, false);
  int SERIAL2(injuredArms, 0);
  int SERIAL2(injuredLegs, 0);
  int SERIAL2(injuredWings, 0);
  int SERIAL2(injuredHeads, 0);
  int SERIAL2(lostArms, 0);
  int SERIAL2(lostLegs, 0);
  int SERIAL2(lostWings, 0);
  int SERIAL2(lostHeads, 0);
  bool SERIAL2(hidden, false);
  bool inEquipChain = false;
  int numEquipActions = 0;
  const Creature* SERIAL2(lastAttacker, nullptr);
  int SERIAL2(swapPositionCooldown, 0);
  map<LastingEffect, double> SERIAL(lastingEffects);
  double SERIAL2(expLevel, 1);
  vector<const Creature*> SERIAL(unknownAttacker);
  vector<const Creature*> SERIAL(privateEnemies);
  const Creature* SERIAL2(holding, nullptr);
  PController SERIAL(controller);
  vector<PController> SERIAL(controllerStack);
  vector<Vision*> SERIAL(visions);
  mutable vector<const Creature*> SERIAL(kills);
  mutable double SERIAL2(difficultyPoints, 0);
  int SERIAL2(points, 0);
};

struct SpellInfo {
  SpellId id;
  string name;
  EffectType type;
  double ready;
  int difficulty;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
};



#endif
