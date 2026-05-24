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
  SoundLibrary();
  milliseconds playSound(const Sound&);
  void setVolume(int); // between 1..100
  void setMuteWhenUnfocused(bool);
  void setUnfocusedCheck(function<bool()>);

  private:
  void addSounds(SoundId, const DirectoryPath&);
  HashMap<SoundId, vector<SoundBuffer>> sounds;
  double volume;
  AudioDevice* audioDevice = nullptr;
  bool isUnfocused() const;
  bool muteWhenUnfocused = false;
  function<bool()> unfocusedCheck;
};
