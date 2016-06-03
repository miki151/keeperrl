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

#include "stdafx.h"
#include "music.h"
#include "options.h"

Jukebox::Jukebox(Options* options, vector<pair<MusicType, string>> tracks, int maxVol, map<MusicType, int> maxV)
    : numTracks(tracks.size()), maxVolume(maxVol), maxVolumes(maxV) {
  music.resize(numTracks);
  for (int i : All(tracks)) {
    CHECK(music[i] = Mix_LoadMUS(tracks[i].second.c_str())) << Mix_GetError();
    byType[tracks[i].first].push_back(i);
  }
  options->addTrigger(OptionId::MUSIC, [this](bool turnOn) { toggle(turnOn); });
  refreshLoop.emplace([this] {
    refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
});
}

int Jukebox::getMaxVolume(int track) {
  auto type = getCurrentType();
  if (maxVolumes.count(type))
    return maxVolumes.at(type);
  else
    return maxVolume;
}

void Jukebox::toggle(bool state) {
  MusicLock lock(musicMutex);
  if (state == on)
    return;
  if (!numTracks)
    return;
  on = state;
  if (on) {
    current = Random.choose(byType[getCurrentType()]);
    currentPlaying = current;
    Mix_VolumeMusic(getMaxVolume(current));
    CHECK(Mix_PlayMusic(music[current], 1) == 0) << Mix_GetError();
  } else
    Mix_FadeOutMusic(2000);
}

void Jukebox::setCurrent(MusicType c) {
  current = Random.choose(byType[c]);
}

void Jukebox::continueCurrent() {
  if (byType[getCurrentType()].size() >= 2)
    while (current == currentPlaying)
      setCurrent(getCurrentType());
}

MusicType Jukebox::getCurrentType() {
  for (auto& elem : byType)
    if (contains(elem.second, current))
      return elem.first;
  FAIL << "Track type not found " << current;
  return MusicType::PEACEFUL;
}

const int volumeDec = 10;

void Jukebox::setType(MusicType c, bool now) {
  MusicLock lock(musicMutex);
  if (!now)
    nextType = c;
  else {
    nextType.reset();
    if (byType[c].empty())
      return;
    if (getCurrentType() != c)
      current = Random.choose(byType[c]);
  }
}

void Jukebox::refresh() {
  MusicLock lock(musicMutex);
  if (!on || !numTracks)
    return;
  if (current != currentPlaying) {
    if (!Mix_PlayingMusic()) {
      currentPlaying = current;
      Mix_VolumeMusic(getMaxVolume(currentPlaying));
      Mix_PlayMusic(music[currentPlaying], 1);
    } else
      Mix_FadeOutMusic(2000);
  } else
  if (!Mix_PlayingMusic()) {
    if (nextType) {
      setCurrent(*nextType);
      nextType.reset();
    } else
      continueCurrent();
    currentPlaying = current;
    Mix_VolumeMusic(getMaxVolume(current));
    Mix_PlayMusic(music[current], 1);
  }
}
