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

  SERIAL_CHECKER;

  private:
  Optional<string> SERIAL(name);
  Optional<string> SERIAL(description);
  Rectangle SERIAL(bounds);
  const Level* SERIAL(level);
  bool SERIAL2(surprise, false);
};

#endif
