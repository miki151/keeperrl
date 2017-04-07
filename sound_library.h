#pragma once

#include "util.h"
#include "sound.h"

class Options;
class AudioDevice;
class SoundBuffer;
class DirectoryPath;

class SoundLibrary {
  public:
  SoundLibrary(Options*, AudioDevice&, const DirectoryPath&);
  void playSound(const Sound&);

  private:
  void addSounds(SoundId, const DirectoryPath&);
  EnumMap<SoundId, vector<SoundBuffer>> sounds;
  bool on;
  AudioDevice& audioDevice;
};
