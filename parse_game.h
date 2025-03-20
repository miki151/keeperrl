#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "gzstream.h"
#include "file_path.h"
#include "pretty_archive.h"
#include "t_string.h"

typedef StreamCombiner<ogzstream, OutputArchive> CompressedOutput;
typedef StreamCombiner<igzstream, InputArchive> CompressedInput;

template <typename InputType>
optional<pair<TString, int>> getNameAndVersionUsing(const FilePath& filename) {
  try {
    InputType input(filename.getPath());
    int version;
    string name;
    input.getArchive() >> version >> name;
    if (name.substr(0, 3) == "OF(" || name.substr(0, 14) == "CAPITAL_FIRST(") {
      PrettyInputArchive input({name}, {}, nullptr);
      TString ret;
      input(ret);
      return make_pair(std::move(ret), version);
    } else {
      auto ind = name.find(" of ");
      if (ind != string::npos) {
        string name1 = name.substr(0, ind);
        string name2 = name.substr(ind + 4);
        return make_pair(TString(TSentence("OF", TString(name1), TString(name2))), version);
      }
      return make_pair(TString(name), version);
    }
  } catch (std::exception&) {
    return none;
  }
}

inline optional<pair<TString, int>> getNameAndVersion(const FilePath& filename) {
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
  } catch (std::exception&) {
    return none;
  }
}

inline optional<SavedGameInfo> loadSavedGameInfo(const FilePath& filename) {
  return getSavedGameInfoUsing<CompressedInput>(filename);
}
