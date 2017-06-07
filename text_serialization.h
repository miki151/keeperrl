#pragma once

#include "extern/iomanip.h"
#include "parse_game.h"
#include "cereal/cereal.hpp"

class TextOutputArchive : public cereal::OutputArchive<TextOutputArchive, cereal::AllowEmptyClassElision> {
  public:
    TextOutputArchive(std::ostream& stream)
          : OutputArchive<TextOutputArchive, cereal::AllowEmptyClassElision>(this), os(stream) {}

    ~TextOutputArchive() CEREAL_NOEXCEPT = default;

    std::ostream& os;
};

class TextInputArchive : public cereal::InputArchive<TextInputArchive, cereal::AllowEmptyClassElision> {
  public:
    //! Construct, loading from the provided stream
    TextInputArchive(std::istream& stream)
          : InputArchive<TextInputArchive, cereal::AllowEmptyClassElision>(this), is(stream) {}

    ~TextInputArchive() CEREAL_NOEXCEPT = default;

    std::istream& is;
};

template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
CEREAL_SAVE_FUNCTION_NAME(TextOutputArchive& ar, T const& t) {
  ar.os << t << " ";
}

//! Loading for POD types from binary
template<class T> inline
typename std::enable_if<std::is_arithmetic<T>::value, void>::type
CEREAL_LOAD_FUNCTION_NAME(TextInputArchive& ar, T& t) {
  ar.is >> t;
}

inline void CEREAL_SAVE_FUNCTION_NAME(TextOutputArchive& ar, std::string const& t) {
  ar.os << std::quoted(t) << " ";
}

inline void CEREAL_LOAD_FUNCTION_NAME(TextInputArchive& ar, std::string& t) {
  ar.is >> std::quoted(t);
}

inline void CEREAL_LOAD_FUNCTION_NAME(TextInputArchive& ar, char& c) {
  string s;
  ar.is >> std::quoted(s);
  c = s.at(0);
}

inline void CEREAL_SAVE_FUNCTION_NAME(TextOutputArchive& ar, char c) {
  string s {c};
  ar.os << std::quoted(s);
}

//! Serializing NVP types to binary
template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(TextInputArchive, TextOutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar1, cereal::NameValuePair<T>& t) {
  ar1(t.value);
}
//! Serializing SizeTags to binary
template <class Archive, class T> inline
CEREAL_ARCHIVE_RESTRICT(TextInputArchive, TextOutputArchive)
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar1, cereal::SizeTag<T> & t) {
  ar1(t.size);
}

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(TextOutputArchive)
CEREAL_REGISTER_ARCHIVE(TextInputArchive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(TextInputArchive, TextOutputArchive)


typedef StreamCombiner<ostringstream, TextOutputArchive> TextOutput;
typedef StreamCombiner<istringstream, TextInputArchive> TextInput;

