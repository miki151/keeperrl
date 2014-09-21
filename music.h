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

#ifndef _JUKEBOX_H
#define _JUKEBOX_H

#include <SFML/Audio/Music.hpp>

class Jukebox {
  public:
  enum Type { INTRO, PEACEFUL, BATTLE };

  Jukebox(const string& introTrack);

  void addTrack(Type, const string&);
  void updateCurrent(Type);

  void toggle();
  void update();

  private:
  void setCurrent(Type);
  bool turnedOff();
  Type getCurrentType();
  unique_ptr<sf::Music[]> music;
  map<Type, vector<int>> byType;
  int current = 0;
  int currentPlaying;
  bool on = true;
  int numTracks = 1;
};

#endif
