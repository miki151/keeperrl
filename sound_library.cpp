#include "stdafx.h"
#include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

#include "audio_device.h"

SoundLibrary::SoundLibrary(Options* options, AudioDevice& audio, const DirectoryPath& path) : audioDevice(audio) {
#ifdef DISABLE_SFX
  on = false;
#else
  on = options->getBoolValue(OptionId::SOUND);
#endif
  options->addTrigger(OptionId::SOUND, [this](bool turnOn) { on = turnOn; });
  for (SoundId id : ENUM_ALL(SoundId))
    addSounds(id, path.subdirectory(toLower(EnumInfo<SoundId>::getString(id))));
}

void SoundLibrary::addSounds(SoundId id, const DirectoryPath& path) {
  for (auto& file : path.getFiles())
    if (file.hasSuffix(".ogg"))
      sounds[id].emplace_back(file);
}

void SoundLibrary::playSound(const Sound& s) {
  if (!on)
    return;
  if (int numSounds = sounds[s.getId()].size()) {
    int ind = Random.get(numSounds);
    audioDevice.play(sounds[s.getId()][ind], 1.0, s.getPitch());
  }
}
