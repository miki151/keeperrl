#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "gzstream.h"

typedef StreamCombiner<ogzstream, OutputArchive> CompressedOutput;
typedef StreamCombiner<igzstream, InputArchive> CompressedInput;
typedef StreamCombiner<igzstream, InputArchive2> CompressedInput2;
typedef StreamCombiner<ostringstream, text_oarchive> TextOutput;
typedef StreamCombiner<istringstream, text_iarchive> TextInput;

template <typename InputType>
optional<pair<string, int>> getNameAndVersionUsing(const FilePath& filename) {
  try {
    InputType input(filename.getPath());
    pair<string, int> ret;
    input.getArchive() >> BOOST_SERIALIZATION_NVP(ret.second) >> BOOST_SERIALIZATION_NVP(ret.first);
    return ret;
  } catch (boost::archive::archive_exception& ex) {
    return none;
  }
}

inline optional<pair<string, int>> getNameAndVersion(const FilePath& filename) {
  if (auto ret = getNameAndVersionUsing<CompressedInput>(filename))
    return ret;
  return getNameAndVersionUsing<CompressedInput2>(filename);
}

template <typename InputType>
optional<SavedGameInfo> getSavedGameInfoUsing(const FilePath& filename) {
  try {
    InputType input(filename.getPath());
    string discard2;
    int discard;
    SavedGameInfo ret;
    input.getArchive() >> BOOST_SERIALIZATION_NVP(discard) >> BOOST_SERIALIZATION_NVP(discard2)
       >> BOOST_SERIALIZATION_NVP(ret);
    return ret;
  } catch (boost::archive::archive_exception& ex) {
    return none;
  }
}

inline optional<SavedGameInfo> getSavedGameInfo(const FilePath& filename) {
  if (auto ret = getSavedGameInfoUsing<CompressedInput>(filename))
    return ret;
  return getSavedGameInfoUsing<CompressedInput2>(filename);
}


