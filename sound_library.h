#ifndef _SOUND_LIBRARY
#define _SOUND_LIBRARY

#include <SFML/Audio/Music.hpp>

#include "sound.h"

class SoundLibrary {
  public:
  SoundLibrary(const string& path);
  void playSound(const Sound&);

  private:
  void addSounds(SoundId, const string& path);
  EnumMap<SoundId, vector<unique_ptr<sf::Music>>> sounds;
};


#endif
