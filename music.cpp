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

using sf::Music;

Jukebox::Jukebox(const string& introPath) {
  music.reset(new Music[20]);
  music[0].openFromFile(introPath);
  byType[INTRO].push_back(0);
  currentPlaying = current;
  if (!turnedOff())
    music[current].play();
  else
    on = false;
  Options::addTrigger(OptionId::MUSIC, [this](bool turnOn) { if (turnOn != on) toggle(); });
}

void Jukebox::addTrack(Type type, const string& path) {
  music[numTracks].openFromFile(path);
  byType[type].push_back(numTracks);
  ++numTracks;
}

bool Jukebox::turnedOff() {
  return !Options::getValue(OptionId::MUSIC);
}

void Jukebox::toggle() {
  if ((on = !on)) {
    currentPlaying = current;
    music[current].play();
  } else
    music[current].stop();
}

void Jukebox::setCurrent(Type c) {
  current = chooseRandom(byType[c]);
}

Jukebox::Type Jukebox::getCurrentType() {
  for (auto& elem : byType)
    if (contains(elem.second, current))
      return elem.first;
  FAIL << "Track type not found " << current;
  return PEACEFUL;
}

const int volumeDec = 20;

void Jukebox::update() {
  if (turnedOff())
    return;
  if (currentPlaying == 0 && music[0].getStatus() == sf::SoundSource::Stopped) {
    setCurrent(PEACEFUL);
    currentPlaying = current;
    music[current].play();
  }
  if (current != currentPlaying) {
    if (music[currentPlaying].getVolume() == 0) {
      music[currentPlaying].stop();
      currentPlaying = current;
      music[currentPlaying].setVolume(100);
      music[currentPlaying].play();
    } else
      music[currentPlaying].setVolume(max(0.0f, music[currentPlaying].getVolume() - volumeDec));
  } else
  if (music[current].getStatus() == sf::SoundSource::Stopped) {
    while (current == currentPlaying)
      setCurrent(getCurrentType());
    currentPlaying = current;
    music[current].play();
  }
}
