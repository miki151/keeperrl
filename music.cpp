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
#include "audio_device.h"

Jukebox::Jukebox(AudioDevice& audio, vector<pair<MusicType, FilePath>> tracks, double maxVol)
    : numTracks(tracks.size()), maxVolume(maxVol), audioDevice(audio) {
  for (int i : All(tracks)) {
    music.emplace_back(tracks[i].second);
    byType[tracks[i].first].push_back(i);
  }
  refreshLoop.emplace([this] {
    refresh();
    sleep_for(milliseconds(200));
});
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
    play(current);
  } else
    stream.reset();
}

void Jukebox::setCurrentVolume(int v) {
  toggle(v > 0);
  currentVolume = double(v) / 100;
  if (stream)
    stream->setVolume(maxVolume * currentVolume);
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
    if (elem.second.contains(current))
      return elem.first;
  FATAL << "Track type not found " << current;
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

void Jukebox::play(int index) {
  stream.reset();
  stream.reset(new SoundStream(music[current], maxVolume * currentVolume));
}

void Jukebox::refresh() {
  MusicLock lock(musicMutex);
  if (!on || !numTracks)
    return;
  if (current != currentPlaying) {
    if (!stream || !stream->isPlaying() || stream->getVolume() == 0) {
      currentPlaying = current;
      play(current);
    } else
      stream->setVolume(max(0.0, stream->getVolume() - volumeDec));
  } else
  if (!stream || !stream->isPlaying()) {
    if (nextType) {
      setCurrent(*nextType);
      nextType.reset();
    } else
      continueCurrent();
    currentPlaying = current;
    play(current);
  }
}
