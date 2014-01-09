#ifndef _CREATURE_H
#define _CREATURE_H

#include "creature_attributes.h"
#include "view_object.h"
#include "equipment.h"
#include "enums.h"
#include "attack.h"
#include "shortest_path.h"
#include "tribe.h"
#include "timer_var.h"
#include "skill.h"
#include "map_memory.h"
#include "creature_view.h"
#include "controller.h"

class Level;
class Tribe;

class Creature : private CreatureAttributes, public CreatureView {
  public:
  typedef CreatureAttributes CreatureAttributes;
  Creature(ViewObject o, Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  virtual ~Creature() {}

  static Creature* getDefault();

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
  bool dodgeAttack(Attack);
  bool takeDamage(Attack);
  void heal(double amount = 1, bool replaceLimbs = false);
  double getHealth() const;
  double getWeight() const;
  void sleep(int time);
  bool isSleeping() const;
  void wakeUp();
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  vector<PItem> steal(const vector<Item*> items);
  int getUniqueId() const;
  virtual bool canSee(const Creature*) const override;
  virtual bool canSee(Vec2 pos) const override;
  void slowDown(double duration);
  void speedUp(double duration);

  void tick(double realTime);

  string getTheName() const;
  string getAName() const;
  string getName() const;
  Optional<string> getFirstName() const;
  int getAttr(AttrType) const;

  Tribe* getTribe() const;
  bool isFriend(const Creature*) const;
  bool isEnemy(const Creature*) const;
  void addEnemyCheck(EnemyCheck*);
  void removeEnemyCheck(EnemyCheck*);
  int getDebt(const Creature* debtor) const;
  vector<Item*> getGold(int num) const;

  bool wantsItems(const Creature* from, vector<Item*> items) const;
  void takeItems(const Creature* from, vector<PItem> items);

  void youHit(BodyPart part, AttackType type) const;

  virtual void dropCorpse();
  
  void globalMessage(const string& playerCanSee, const string& cant = "") const;
  const vector<const Creature*>& getVisibleEnemies() const;
  vector<const Creature*> getVisibleCreatures() const;

  bool isDead() const;
  bool isHumanoid() const;
  bool isAnimal() const;
  bool isStationary() const;
  void setStationary();
  bool isInvincible() const;
  bool isUndead() const;
  bool canSwim() const;
  bool canFly() const;
  bool canWalk() const;
  int numArms() const;
  int numLegs() const;
  int numWings() const;
  int numHeads() const;
  bool lostLimbs() const;
  int numGoodArms() const;
  int numGoodLegs() const;
  int numGoodHeads() const;
  double getCourage() const;

  void increaseExpLevel();
  int getExpLevel() const;

  string getDescription() const;
  bool isSpecialMonster() const;

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  bool hasSkillToUseWeapon(const Item*) const;

  bool canMove(Vec2 direction) const;
  void move(Vec2 direction);
  bool canSwapPosition(Vec2 direction) const;
  void swapPosition(Vec2 direction);
  void wait();
  vector<Item*> getPickUpOptions() const;
  bool canPickUp(const vector<Item*>& item) const;
  void pickUp(const vector<Item*>& item);
  void drop(const vector<Item*>& item);
  void drop(vector<PItem> item);
  bool canAttack(const Creature*) const;
  void attack(const Creature*, bool spendTime = true);
  bool canBumpInto(Vec2 direction) const;
  void bumpInto(Vec2 direction);
  void applyItem(Item* item);
  bool canApplyItem(Item* item) const;
  void startEquipChain();
  void finishEquipChain();
  void equip(Item* item);
  void unequip(Item* item);
  bool canEquip(const Item* item) const;
  bool canUnequip(const Item* item) const;
  bool canThrowItem(Item*);
  void throwItem(Item*, Vec2 direction);
  bool canHeal(Vec2 direction) const;
  void heal(Vec2 direction);
  void applySquare();
  bool canHide() const;
  void hide();
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  void panic(double time);
  void rage(double time);
  void hallucinate(double time);
  bool isHallucinating() const;
  void blind(double time);
  bool isBlind() const;
  void makeInvisible(double time);
  bool isInvisible() const;
  void giveStrBonus(double time);
  void giveDexBonus(double time);
  bool canFlyAway() const;
  void flyAway();
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

  bool isPanicking() const;
  Item* getWeapon() const;

  Optional<Vec2> getMoveTowards(Vec2 pos, bool avoidEnemies = false);
  Optional<Vec2> getMoveAway(Vec2 pos, bool pathfinding = true);
  bool atTarget() const;
  void die(const Creature* attacker = nullptr, bool dropInventory = true);
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
  void onItemsAppeared(vector<Item*> items);
  const MapMemory& getMemory(const Level* l = nullptr) const;
  void grantIdentify(int numItems);

  Controller* getController();
  void pushController(Controller*);
  void popController();
  bool canPopController();
  void setSpeed(double);
  double getSpeed() const;

  typedef function<bool(const Creature*)> EnemyVision;

  void addEnemyVision(EnemyVision);

  protected:
  void setViewObject(const ViewObject&);

  private:
  Optional<Vec2> getMoveTowards(Vec2 pos, bool away, bool avoidEnemies);
  double getInventoryWeight() const;
  Item* getAmmo() const;
  void updateViewObject();
  int getStrengthAttackBonus() const;
  int getAttrVal(AttrType type) const;
  int getToHit() const;
  BodyPart getBodyPart(AttackLevel attack) const;
  void injureLeg(bool drop);
  void injureArm(bool drop);
  void injureWing(bool drop);
  void injureHead(bool drop);
  AttackLevel getRandomAttackLevel() const;
  AttackType getAttackType() const;
  void spendTime(double time);
  BodyPart armOrWing() const;
  void updateVisibleEnemies();
  pair<double, double> getStanding(const Creature* c) const;

  ViewObject viewObject;
  Level* level = nullptr;
  Vec2 position;
  double time;
  Equipment equipment;
  int uniqueId;
  Optional<ShortestPath> shortestPath;
  unordered_set<const Creature*> knownHiding;
  Tribe* tribe;
  unordered_map<const Tribe*, double> standingOverride;
  vector<EnemyCheck*> enemyChecks;
  double health = 1;
  bool dead;
  double lastTick;
  bool collapsed = false;
  int injuredArms = 0;
  int injuredLegs = 0;
  int injuredWings = 0;
  int injuredHeads = 0;
  int lostArms = 0;
  int lostLegs = 0;
  int lostWings = 0;
  int lostHeads = 0;
  bool hidden = false;
  bool inEquipChain = false;
  int numEquipActions = 0;
  Optional<Vec2> homePos;
  const Creature* leader = nullptr;
  const Creature* lastAttacker = nullptr;
  int swapPositionCooldown = 0;
  TimerVar sleeping;
  TimerVar panicking;
  TimerVar enraged;
  TimerVar slowed;
  TimerVar speeding;
  TimerVar strBonus;
  TimerVar dexBonus;
  TimerVar hallucinating;
  TimerVar blinded;
  TimerVar invisible;
  int expLevel = 1;
  vector<const Creature*> unknownAttacker;
  vector<const Creature*> visibleEnemies;
  vector<const Creature*> privateEnemies;
  const Creature* holding = nullptr;
  PController controller;
  stack<PController> controllerStack;
  vector<EnemyVision> enemyVision;
};


#endif
