#include "t_string.h"
#include "pretty_archive.h"

template <typename Archive>
void TString::serialize(Archive& ar) {
  if (Archive::is_loading::value) {
    string s;
    ar(s);
    if (s[0] == 'T' && s[0] == '_')
      *this = TStringId(s.substr(2).data());
    else
      *this = std::move(s);
  } else {
    string out;
    visit(
      [&] (const string& s) { out = s; },
      [&] (TStringId id) { out = "T_"_s + id.data(); }
    );
    ar(out);
  }
}

template <>
void TString::serialize(PrettyInputArchive& ar) {
  if (ar.peek()[0] == '"') {
    string s;
    ar(s);
    *this = std::move(s);
  } else {
    TStringId id;
    ar(id);
    *this = std::move(id);
  }
}

TString& TString::operator = (TStringId id) {
  ((variant<TStringId, string>*)(this))->operator= (std::move(id));
  return *this;
}

TString& TString::operator = (string s) {
  ((variant<TStringId, string>*)(this))->operator= (std::move(s));
  return *this;
}
