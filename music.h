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

#pragma once

#include "util.h"

class Options;
class AudioDevice;
class SoundStream;

enum class MusicType { INTRO, MAIN, PEACEFUL, BATTLE, NIGHT, ADV_PEACEFUL, ADV_BATTLE };

class Jukebox {
  public:
  Jukebox(Options*, AudioDevice&, vector<pair<MusicType, string>> tracks, float maxVolume,
      map<MusicType, float> maxVolumes);

  void setType(MusicType, bool now);
  void toggle(bool on);

  private:
  void play(int index);
  void setCurrent(MusicType);
  void continueCurrent();
  bool turnedOff();
  void refresh();
  MusicType getCurrentType();

  typedef std::unique_lock<std::recursive_mutex> MusicLock;
  std::recursive_mutex musicMutex;

  vector<string> music;
  unique_ptr<SoundStream> stream;
  map<MusicType, vector<int>> byType;
  int current = 0;
  int currentPlaying = 0;
  bool on = false;
  int numTracks = 0;
  float getMaxVolume(int track);
  float maxVolume;
  map<MusicType, float> maxVolumes;
  optional<MusicType> nextType;
  optional<AsyncLoop> refreshLoop;
  AudioDevice& audioDevice;
};
