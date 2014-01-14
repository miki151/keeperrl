#ifndef _PLAYER_H
#define _PLAYER_H

#include "creature.h"

class View;

class Player : public Controller, public EventListener {
  public:
  Player(Creature*, View*, bool displayGreeting, map<const Level*, MapMemory>* levelMemory);
  virtual ~Player();
  virtual void grantIdentify(int numItems) override;

  virtual bool isPlayer() const override;

  virtual void you(MsgType type, const string& param) const override;
  virtual void you(const string& param) const override;
  virtual void privateMessage(const string& message) const override;

  virtual void onKilled(const Creature* attacker) override;
  virtual void onItemsAppeared(vector<Item*> items, const Creature* from);

  View* getView();

  const MapMemory& getMemory(const Level* l = nullptr) const;
  virtual void makeMove() override;

  virtual void onBump(Creature*);

  static ControllerFactory getFactory(View*, map<const Level*, MapMemory>* levelMemory);
  
  virtual const Level* getListenerLevel() const override;
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) override;


  private:
  void remember(Vec2 pos, const ViewObject& object);
  void pickUpAction(bool extended);
  void itemsMessage();
  void dropAction(bool extended);
  void equipmentAction();
  void applyAction();
  void throwAction(Optional<Vec2> dir = Nothing());
  void takeOffAction();
  void hideAction();
  void displayInventory();
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payDebtAction();
  void chatAction();
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, function<bool (Item*)> predicate, bool displayOnly = false,
      Optional<string> otherOption = Nothing());
  void getItemNames(vector<Item*> items, vector<string>& names, vector<vector<Item*> >& groups);
  string getPluralName(Item* item, int num);
  Creature* creature;
  View* view = nullptr;
  bool travelling = false;
  Vec2 travelDir;
  Optional<Vec2> target;
  const Location* lastLocation;
  vector<const Creature*> specialCreatures;
  bool displayGreeting;
  map<const Level*, MapMemory>* levelMemory;
};

#endif
