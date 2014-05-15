/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _LOCATION_H
#define _LOCATION_H

#include "util.h"

class Level;

class Location {
  public:
  Location(const string& name, const string& description, bool surprise = false);
  Location(bool surprise = false);
  Location(Level* l, Rectangle bounds);
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
  const Level* SERIAL(level);
  Rectangle SERIAL(bounds);
  bool SERIAL2(surprise, false);
};

#endif
