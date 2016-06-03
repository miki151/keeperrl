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

#include "util.h"
#include <SDL2/SDL_mixer.h>

class Options;

enum class MusicType { INTRO, MAIN, PEACEFUL, BATTLE, NIGHT, ADV_PEACEFUL, ADV_BATTLE };

class Jukebox {
  public:
  Jukebox(Options*, vector<pair<MusicType, string>> tracks, int maxVolume, map<MusicType, int> maxVolumes);

  void setType(MusicType, bool now);
  void toggle(bool on);

  private:
  void setCurrent(MusicType);
  void continueCurrent();
  bool turnedOff();
  void refresh();
  MusicType getCurrentType();

  typedef std::unique_lock<std::recursive_mutex> MusicLock;
  std::recursive_mutex musicMutex;

  vector<Mix_Music*> music;
  map<MusicType, vector<int>> byType;
  int current = 0;
  int currentPlaying = 0;
  bool on = false;
  int numTracks = 0;
  int getMaxVolume(int track);
  int maxVolume;
  map<MusicType, int> maxVolumes;
  optional<MusicType> nextType;
  optional<AsyncLoop> refreshLoop;
};

#endif
