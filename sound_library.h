#ifndef _SOUND_LIBRARY
#define _SOUND_LIBRARY

#include "sound.h"

class Options;
class AudioDevice;
class SoundBuffer;

class SoundLibrary {
  public:
  SoundLibrary(Options*, AudioDevice&, const string& path);
  void playSound(const Sound&);

  private:
  void addSounds(SoundId, const string& path);
  EnumMap<SoundId, vector<SoundBuffer>> sounds;
  bool on;
  AudioDevice& audioDevice;
};


#endif
