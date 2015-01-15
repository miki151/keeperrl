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

#include "util.h"

class Options;

enum class MusicType { MAIN, PEACEFUL, BATTLE, NIGHT };

class Jukebox {
  public:

  Jukebox(Options*);

  void addTrack(MusicType, const string&);
  void update(MusicType);

  void toggle();

  private:
  void setCurrent(MusicType);
  void continueCurrent();
  bool turnedOff();
  MusicType getCurrentType();
  unique_ptr<sf::Music[]> music;
  map<MusicType, vector<int>> byType;
  int current = 0;
  int currentPlaying = 0;
  bool on = true;
  int numTracks = 0;
};

#endif
