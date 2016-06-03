#include "stdafx.h"
#include "dirent.h"
#include "sound_library.h"
#include "sound.h"
#include "options.h"

SoundLibrary::SoundLibrary(Options* options, const string& path) {
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
      sounds[id].push_back(Mix_LoadWAV((path + "/" + name).c_str()));
    }
  }
}

void SoundLibrary::playSound(const Sound& s) {
  if (!on)
    return;
  if (int numSounds = sounds[s.getId()].size()) {
    int ind = Random.get(numSounds);
//    sounds[s.getId()][ind]->setPitch(s.getPitch());
    Mix_PlayChannel(-1, sounds[s.getId()][ind], 0);
  }
}
