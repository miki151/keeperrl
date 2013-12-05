#ifndef _CONTROLLER_H
#define _CONTROLLER_H

class Creature;

class Controller {
  public:
  virtual void grantIdentify(int numItems) {};

  virtual bool isPlayer() const = 0;

  virtual void you(MsgType type, const string& param) const = 0;
  virtual void you(const string& param) const = 0;
  virtual void privateMessage(const string& message) const {}

  virtual void onKilled(const Creature* attacker) {}
  virtual void onItemsAppeared(vector<Item*> items) {}
  virtual const MapMemory& getMemory(const Level* l = nullptr) const = 0;

  virtual void makeMove() = 0;

  virtual bool wantsItems(const Creature* from, vector<Item*> items) const { return false; }
  virtual void takeItems(const Creature* from, vector<PItem> items) {}
  virtual int getDebt(const Creature* debtor) const { return 0; }

  virtual void onBump(Creature*) = 0;

  virtual ~Controller() {}
};

class ControllerFactory {
  public:
  ControllerFactory(function<Controller* (Creature*)>);
  PController get(Creature*);

  private:
  function<Controller* (Creature*)> fun;
};

#endif
