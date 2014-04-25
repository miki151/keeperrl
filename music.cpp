#include "stdafx.h"
#include "music.h"
#include "options.h"

using sf::Music;

Jukebox jukebox;

void Jukebox::initialize(const string& introPath, const string& peacefulPath, const string& warPath) {
  music.reset(new Music[3]);
  music[0].openFromFile(introPath);
  music[1].openFromFile(peacefulPath);
  music[2].openFromFile(warPath);
  music[1].setLoop(true);
  music[2].setLoop(true);
  currentPlaying = current;
  if (!turnedOff())
    get(current).play();
  else
    on = false;
  Options::addTrigger(OptionId::MUSIC, [this](bool turnOn) { if (turnOn != on) toggle(); });
}

bool Jukebox::turnedOff() {
  return !Options::getValue(OptionId::MUSIC);
}

sf::Music& Jukebox::get(Type type) {
  return music[int(type)];
}

void Jukebox::toggle() {
  if ((on = !on)) {
    currentPlaying = current;
    get(current).play();
  } else
    get(current).stop();
}

void Jukebox::setCurrent(Type c) {
  if (current != INTRO)
    current = c;
}

const int volumeDec = 20;

void Jukebox::update() {
  if (turnedOff())
    return;
  if (currentPlaying == INTRO && music[0].getStatus() == sf::SoundSource::Stopped) {
    currentPlaying = current = PEACEFUL;
    get(current).play();
  }
  if (current != currentPlaying) {
    if (get(currentPlaying).getVolume() == 0) {
      get(currentPlaying).stop();
      currentPlaying = current;
      get(currentPlaying).setVolume(100);
      get(currentPlaying).play();
    } else
      get(currentPlaying).setVolume(max(0.0f, get(currentPlaying).getVolume() - volumeDec));
  }
}
