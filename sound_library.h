#ifndef _SOUND_LIBRARY
#define _SOUND_LIBRARY

#include "sound.h"
#include "SDL2/SDL_mixer.h"

class Options;

class SoundLibrary {
  public:
  SoundLibrary(Options*, const string& path);
  void playSound(const Sound&);

  private:
  void addSounds(SoundId, const string& path);
  EnumMap<SoundId, vector<Mix_Chunk*>> sounds;
  bool on;
};


#endif
