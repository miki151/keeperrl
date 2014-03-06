#ifndef _PLAYER_H
#define _PLAYER_H

#include "creature.h"

class View;
class Model;

class Player : public Controller, public EventListener {
  public:
  Player(Creature*, View*, Model*, bool displayGreeting, map<const Level*, MapMemory>* levelMemory);
  virtual ~Player();
  virtual void grantIdentify(int numItems) override;

  virtual bool isPlayer() const override;

  virtual void you(MsgType type, const string& param) const override;
  virtual void you(const string& param) const override;
  virtual void privateMessage(const string& message) const override;

  virtual void onKilled(const Creature* attacker) override;
  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) override;

  virtual const MapMemory& getMemory(const Level* l = nullptr) const override;
  virtual void learnLocation(const Location*) override;

  virtual void makeMove() override;
  virtual void sleeping() override;

  virtual void onBump(Creature*);

  static ControllerFactory getFactory(View*, Model*, map<const Level*, MapMemory>* levelMemory);
  
  virtual const Level* getListenerLevel() const override;
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) override;
  virtual void onExplosionEvent(const Level* level, Vec2 pos) override;

  SERIALIZATION_DECL(Player);

  private:
  void remember(Vec2 pos, const ViewObject& object);
  void pickUpAction(bool extended);
  void itemsMessage();
  void dropAction(bool extended);
  void equipmentAction();
  void applyAction();
  void applyItem(vector<Item*> item);
  void throwAction(Optional<Vec2> dir = Nothing());
  void throwItem(vector<Item*> item, Optional<Vec2> dir = Nothing());
  void takeOffAction();
  void hideAction();
  void displayInventory();
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payDebtAction();
  void chatAction(Optional<Vec2> dir = Nothing());
  void spellAction();
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, ItemPredicate, Optional<ActionId> exitAction = Nothing());
  void getItemNames(vector<Item*> it, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
      ItemPredicate = alwaysTrue<const Item*>());
  string getPluralName(Item* item, int num);
  Creature* creature;
  bool travelling = false;
  Vec2 travelDir;
  Optional<Vec2> target;
  const Location* lastLocation = nullptr;
  vector<const Creature*> specialCreatures;
  bool displayGreeting;
  map<const Level*, MapMemory>* levelMemory;
  Model* model;
  bool displayTravelInfo = true;
};

#endif
