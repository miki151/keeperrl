#ifndef _LOCATION_H
#define _LOCATION_H

#include "util.h"

class Location {
  public:
  Location(const string& name, const string& description);
  Location();
  string getName() const;
  string getDescription() const;
  bool hasName() const;
  Rectangle getBounds() const;
  void setBounds(Rectangle);
  void setLevel(const Level*);
  const Level* getLevel() const;

  virtual void onCreature(Creature* c) {}

  static Location* towerTopLocation();

  private:
  Optional<string> name;
  Optional<string> description;
  Rectangle bounds;
  const Level* level;
};

#endif
