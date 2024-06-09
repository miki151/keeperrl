#include "stdafx.h"
// #include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

#include "audio_device.h"

SoundLibrary::SoundLibrary() {}

SoundLibrary::SoundLibrary(AudioDevice& audio, const DirectoryPath& path) : audioDevice(&audio) {
  for (auto subdir : path.getSubDirs())
    addSounds(subdir, path.subdirectory(subdir));
}

void SoundLibrary::addSounds(SoundId id, const DirectoryPath& path) {
  for (auto& file : path.getFiles())
    if (file.hasSuffix(".ogg"))
      sounds[id].emplace_back(file);
}

milliseconds SoundLibrary::playSound(const Sound& s) {
  if (!audioDevice || volume < 0.0001)
    return milliseconds{1000};
  if (auto clips = getReferenceMaybe(sounds, toLower(s.getId()))) {
    auto& sound = Random.choose(*clips);
    audioDevice->play(sound, s.getVolume() * volume, s.getPitch());
    return audioDevice->getDuration(sound);
  } else
    USER_INFO << "Sound not found: " << toLower(s.getId());
  return milliseconds{1000};
}

void SoundLibrary::setVolume(int v) {
  volume = double(v) / 100;
}
