#include "stdafx.h"
#include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

#include "audio_device.h"

SoundLibrary::SoundLibrary(AudioDevice& audio, const DirectoryPath& path) : audioDevice(audio) {
  for (auto subdir : path.getSubDirs())
    addSounds(subdir, path.subdirectory(subdir));
}

void SoundLibrary::addSounds(SoundId id, const DirectoryPath& path) {
  for (auto& file : path.getFiles())
    if (file.hasSuffix(".ogg"))
      sounds[id].emplace_back(file);
}

void SoundLibrary::playSound(const Sound& s) {
  if (volume < 0.0001)
    return;
  if (auto clips = getReferenceMaybe(sounds, toLower(s.getId())))
    audioDevice.play(Random.choose(*clips), s.getVolume() * volume, s.getPitch());
  else
    USER_INFO << "Sound not found: " << toLower(s.getId());
}

void SoundLibrary::setVolume(int v) {
  volume = double(v) / 100;
}
