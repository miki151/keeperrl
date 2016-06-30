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

#include <cAudio.h>

Jukebox::Jukebox(Options* options, cAudio::IAudioManager* audio, vector<pair<MusicType, string>> tracks,
    float maxVol, map<MusicType, float> maxV)
    : numTracks(tracks.size()), maxVolume(maxVol), maxVolumes(maxV), cAudio(audio) {
  music.resize(numTracks);
  for (int i : All(tracks)) {
    CHECK(music[i] = cAudio->create(tracks[i].second.c_str(), tracks[i].second.c_str()))
      << "Failed to load track " << tracks[i].second;
    byType[tracks[i].first].push_back(i);
  }
  options->addTrigger(OptionId::MUSIC, [this](bool turnOn) { toggle(turnOn); });
  refreshLoop.emplace([this] {
    refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
});
}

float Jukebox::getMaxVolume(int track) {
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
    music[current]->setVolume(getMaxVolume(current));
    CHECK(music[current]->play2d()) << "Failed to play track.";
  } else
    music[current]->stop();
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

const float volumeDec = 0.1f;

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
    if (!music[currentPlaying]->isPlaying() || music[currentPlaying]->getVolume() == 0) {
      currentPlaying = current;
      music[current]->setVolume(getMaxVolume(current));
      CHECK(music[current]->play2d()) << "Failed to play track.";
    } else
      music[currentPlaying]->setVolume(max(0.0f, music[currentPlaying]->getVolume() - volumeDec));
  } else
  if (!music[currentPlaying]->isPlaying()) {
    if (nextType) {
      setCurrent(*nextType);
      nextType.reset();
    } else
      continueCurrent();
    currentPlaying = current;
    music[current]->setVolume(getMaxVolume(current));
    CHECK(music[current]->play2d()) << "Failed to play track.";
  }
}
