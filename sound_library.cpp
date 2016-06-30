#include "stdafx.h"
#include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

#include <cAudio.h>

SoundLibrary::SoundLibrary(Options* options, cAudio::IAudioManager* audio, const string& path) : cAudio(audio) {
#ifdef DISABLE_SFX
  on = false;
#else
  on = options->getBoolValue(OptionId::SOUND);
#endif
  options->addTrigger(OptionId::SOUND, [this](bool turnOn) { on = turnOn; });
  for (SoundId id : ENUM_ALL(SoundId))
    addSounds(id, path + "/" + toLower(EnumInfo<SoundId>::getString(id)));
}

void SoundLibrary::addSounds(SoundId id, const string& path) {
  DIR* dir = opendir(path.c_str());
  CHECK(dir) << path;
  vector<string> files;
  while (dirent* ent = readdir(dir)) {
    string name(ent->d_name);
    if (endsWith(name, ".ogg")) {
      auto sound = cAudio->create(name.c_str(), (path + "/" + name).c_str());
      CHECK(sound) << "Failed to load sound " << path + "/" + name;
      sounds[id].push_back(sound);
    }
  }
}

void SoundLibrary::playSound(const Sound& s) {
  if (!on)
    return;
  if (int numSounds = sounds[s.getId()].size()) {
    int ind = Random.get(numSounds);
    sounds[s.getId()][ind]->setPitch(s.getPitch());
    sounds[s.getId()][ind]->play2d(false);
  }
}
