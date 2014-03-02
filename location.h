#ifndef _LOCATION_H
#define _LOCATION_H

#include "util.h"

class Level;

class Location {
  public:
  Location(const string& name, const string& description);
  Location();
  Location(bool surprise);
  string getName() const;
  string getDescription() const;
  bool hasName() const;
  bool isMarkedAsSurprise() const;
  Rectangle getBounds() const;
  void setBounds(Rectangle);
  void setLevel(const Level*);
  const Level* getLevel() const;

  virtual void onCreature(Creature* c) {}

  static Location* towerTopLocation();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Optional<string> name;
  Optional<string> description;
  Rectangle bounds;
  const Level* level;
  bool surprise = false;
};

#endif
