#pragma once

#include "util.h"
#include "sound.h"

class Options;
class AudioDevice;
class SoundBuffer;
class DirectoryPath;

class SoundLibrary {
  public:
  SoundLibrary(AudioDevice&, const DirectoryPath&);
  void playSound(const Sound&);
  void setVolume(int); // between 1..100

  private:
  void addSounds(SoundId, const DirectoryPath&);
  EnumMap<SoundId, vector<SoundBuffer>> sounds;
  double volume;
  AudioDevice& audioDevice;
};
