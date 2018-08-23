#include "stdafx.h"
#include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

#include "audio_device.h"

SoundLibrary::SoundLibrary(AudioDevice& audio, const DirectoryPath& path) : audioDevice(audio) {
  for (SoundId id : ENUM_ALL(SoundId))
    addSounds(id, path.subdirectory(toLower(EnumInfo<SoundId>::getString(id))));
}

void SoundLibrary::addSounds(SoundId id, const DirectoryPath& path) {
  for (auto& file : path.getFiles())
    if (file.hasSuffix(".ogg"))
      sounds[id].emplace_back(file);
}

void SoundLibrary::playSound(const Sound& s) {
  if (volume < 0.0001)
    return;
  if (int numSounds = sounds[s.getId()].size()) {
    int ind = Random.get(numSounds);
    audioDevice.play(sounds[s.getId()][ind], volume, s.getPitch());
  }
}

void SoundLibrary::setVolume(int v) {
  volume = double(v) / 100;
}
