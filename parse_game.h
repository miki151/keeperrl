#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "gzstream.h"

typedef StreamCombiner<ogzstream, OutputArchive> CompressedOutput;
typedef StreamCombiner<igzstream, InputArchive> CompressedInput;

template <typename InputType>
optional<pair<string, int>> getNameAndVersionUsing(const FilePath& filename) {
  try {
    InputType input(filename.getPath());
    pair<string, int> ret;
    input.getArchive() >> ret.second >> ret.first;
    return ret;
  } catch (cereal::Exception&) {
    return none;
  }
}

inline optional<pair<string, int>> getNameAndVersion(const FilePath& filename) {
  return getNameAndVersionUsing<CompressedInput>(filename);
}

template <typename InputType>
optional<SavedGameInfo> getSavedGameInfoUsing(const FilePath& filename) {
  try {
    InputType input(filename.getPath());
    string discard2;
    int discard;
    SavedGameInfo ret;
    input.getArchive() >> discard >> discard2 >> ret;
    return ret;
  } catch (cereal::Exception&) {
    return none;
  }
}

inline optional<SavedGameInfo> getSavedGameInfo(const FilePath& filename) {
  return getSavedGameInfoUsing<CompressedInput>(filename);
}


