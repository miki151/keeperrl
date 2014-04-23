#ifndef _JUKEBOX_H
#define _JUKEBOX_H

#include <SFML/Audio/Music.hpp>

class Jukebox {
  public:
  enum Type { INTRO, PEACEFUL, BATTLE };
  void initialize(const string& introPath, const string& peacefulPath, const string& warPath);

  void setCurrent(Type);
  void toggle();
  void update();

  private:
  sf::Music& get(Type);
  sf::Music music[3];
  Type current = INTRO;
  Type currentPlaying;
  bool on = true;
};

extern Jukebox jukebox;

#endif
