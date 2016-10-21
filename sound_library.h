#pragma once

#include "util.h"
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
