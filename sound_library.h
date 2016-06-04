#ifndef _SOUND_LIBRARY
#define _SOUND_LIBRARY

#include "sound.h"

class Options;

namespace cAudio {
  class IAudioManager;
  class IAudioSource;
};

class SoundLibrary {
  public:
  SoundLibrary(Options*, cAudio::IAudioManager*, const string& path);
  void playSound(const Sound&);

  private:
  void addSounds(SoundId, const string& path);
  EnumMap<SoundId, vector<cAudio::IAudioSource*>> sounds;
  bool on;
  cAudio::IAudioManager* cAudio;
};


#endif
