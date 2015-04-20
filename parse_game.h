#ifndef _PARSE_GAME_H
#define _PARSE_GAME_H

typedef StreamCombiner<ogzstream, OutputArchive> CompressedOutput;
typedef StreamCombiner<igzstream, InputArchive> CompressedInput;
typedef StreamCombiner<igzstream, InputArchive2> CompressedInput2;

template <typename InputType>
optional<pair<string, int>> getNameAndVersionUsing(const string& filename) {
  try {
    InputType input(filename.c_str());
    pair<string, int> ret;
    input.getArchive() >> BOOST_SERIALIZATION_NVP(ret.second) >> BOOST_SERIALIZATION_NVP(ret.first);
    return ret;
  } catch (boost::archive::archive_exception& ex) {
    return none;
  }
}

inline optional<pair<string, int>> getNameAndVersion(const string& filename) {
  if (auto ret = getNameAndVersionUsing<CompressedInput>(filename))
    return ret;
  return getNameAndVersionUsing<CompressedInput2>(filename);
}


#endif
