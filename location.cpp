#include "stdafx.h"

Location::Location(const string& _name, const string& desc) : name(_name), description(desc), bounds(-1, -1, 1, 1) {
}

Location::Location() : bounds(-1, -1, 1, 1) {}

Location::Location(bool s) : bounds(-1, -1, 1, 1), surprise(s) {}

bool Location::isMarkedAsSurprise() const {
  return surprise;
}

string Location::getName() const {
  return *name;
}

string Location::getDescription() const {
  return *description;
}

bool Location::hasName() const {
  return name;
}

Rectangle Location::getBounds() const {
  CHECK(bounds.getPX() > -1) << "Location bounds not initialized";
  return bounds;
}

void Location::setBounds(Rectangle b) {
  bounds = b;
}

void Location::setLevel(const Level* l) {
  level = l;
}

const Level* Location::getLevel() const {
  return level;
}

class TowerTopLocation : public Location {
  public:

/*  virtual void onCreature(Creature* cr) override {
    if (!cr->isPlayer())
      return;
    Player* c = dynamic_cast<Player*>(cr);
    if (!entered.count(c) && !c->isBlind()) {
      for (Vec2 v : c->getLevel()->getBounds())
        if ((v - c->getPosition()).lengthD() < 300 && !c->getLevel()->getSquare(v)->isCovered())
          c->remember(v, c->getLevel()->getSquare(v)->getViewObject());
      c->privateMessage("You stand at the top of a very tall stone tower.");
      c->privateMessage("You see distant land in all directions.");
      entered.insert(c);
    }
  }*/

  private:
  unordered_set<Creature*> entered;
};

Location* Location::towerTopLocation() {
  return new TowerTopLocation();
}

