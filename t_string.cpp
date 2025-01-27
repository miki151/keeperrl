#include "t_string.h"
#include "pretty_archive.h"


TString::TString(string s) : text(std::move(s)) {}
TString::TString(TSentence id) : text(std::move(id)) {}
TString::TString(TStringId id) : text(TSentence(std::move(id))) {}
TString::TString() : text(string()) {}

template <typename Archive>
void TString::serialize(Archive& ar) {
  if (Archive::is_loading::value) {
    string s;
    ar(s);
    if (s[0] == 'T' && s[1] == '_') {
      vector<TString> params;
      ar(params);
      *this = TSentence(s.substr(2).data(), std::move(params));
    } else
      *this = std::move(s);
  } else {
    string out;
    text.visit(
      [&] (string s) { ar(s); },
      [&] (TSentence s) { string id = "T_"_s + s.id.data(); ar(id); ar(s.params); }
    );
  }
}

bool TString::empty() const {
  return text.visit(
    [](const TSentence& id) { return false; },
    [](const string& s) { return s.empty(); }
  );
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

template void TString::serialize(InputArchive&);
template void TString::serialize(OutputArchive&);

TString& TString::operator = (TSentence s) {
  text = std::move(s);
  return *this;
}

TString& TString::operator = (string s) {
  text = std::move(s);
  return *this;
}

TString& TString::operator = (TStringId id) {
  text = TSentence(std::move(id));
  return *this;
}

TString toText(int num) {
  switch (num) {
    case 0: return TStringId("ZERO");
    case 1: return TStringId("ONE");
    case 2: return TStringId("TWO");
    case 3: return TStringId("THREE");
    case 4: return TStringId("FOUR");
    case 5: return TStringId("FIVE");
    case 6: return TStringId("SIX");
    case 7: return TStringId("SEVEN");
    case 8: return TStringId("EIGHT");
    case 9: return TStringId("NINE");
    case 10: return TStringId("TEN");
    case 11: return TStringId("ELEVEN");
    case 12: return TStringId("TWELVE");
    default: return toString(num);
  }
}

TString combineWithAnd(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  if (v.size() == 2)
    return TSentence("AND", std::move(v[0]), std::move(v[1]));
  return TSentence("COMMA", std::move(v[0]), combineWithAnd(v.getSuffix(v.size() - 1)));
}

TString combineWithCommas(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  return TSentence("COMMA", std::move(v[0]), combineWithCommas(v.getSuffix(v.size() - 1)));
}

TString combineWithSpace(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  return TSentence("SPACE", std::move(v[0]), combineWithAnd(v.getSuffix(v.size() - 1)));
}

TString combineWithNewLine(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  return TSentence("NEW_LINE", std::move(v[0]), combineWithAnd(v.getSuffix(v.size() - 1)));
}

TString combineWithOr(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  if (v.size() == 2)
    return TSentence("OR", std::move(v[0]), std::move(v[1]));
  return TSentence("COMMA", std::move(v[0]), combineWithAnd(v.getSuffix(v.size() - 1)));
}

TString combineSentences(TString s1, TString s2) {
  return combineSentences({std::move(s1), std::move(s2)});
}

TString combineSentences(vector<TString> v) {
  return combineWithSpace(std::move(v));
}

bool TString::operator == (const TString& s) const {
  return text == s.text;
}

bool TString::operator != (const TString& s) const {
  return text != s.text;
}

bool TString::operator < (const TString& s) const {
  if (text.index() == s.text.index())
    return text.visit(
      [&](const TSentence& id) { return id < *s.text.getReferenceMaybe<TSentence>(); },
      [&](const string& text) { return text < *s.text.getReferenceMaybe<string>(); }
    );
  else
    return text.index() < s.text.index();
}

int TSentence::getHash() const {
  return combineHash(id, params);
}

int TString::getHash() const {
  return text.visit([](const auto& elem) { return combineHash(elem); } );
}

const char* TString::data() const {
  return text.visit(
    [](const TSentence& id) { return id.id.data(); },
    [](const string& s) { return s.data(); }
  );
}

TSentence::TSentence(TStringId id, TString param) : id(id), params({std::move(param)}) {}

TSentence::TSentence(TStringId id, TString param1, TString param2)
    : id(id), params({std::move(param1), std::move(param2)}) {}

TSentence::TSentence(TStringId id, vector<TString> params) : id(id), params(std::move(params)) {}

TSentence::TSentence(const char* id, vector<TString> params) : TSentence(TStringId(id), std::move(params)) {
}

TSentence::TSentence(const char* id, TString param) : TSentence(TStringId(id), std::move(param)) {
}

TSentence::TSentence(const char* id, TString param1, TString param2)
    : TSentence(TStringId(id), std::move(param1), std::move(param2)) {}

bool TSentence::operator == (const TSentence& s) const {
  return id == s.id && params == s.params;
}

bool TSentence::operator != (const TSentence& s) const {
  return !(*this == s);
}

bool TSentence::operator < (const TSentence& s) const {
  if (id == s.id) {
    if (params.size() == s.params.size()) {
      for (int i : Range(params.size())) {
        if (params[i] == s.params[i])
          continue;
        return params[i] < s.params[i];
      }
      return false;
    } else
      return params.size() < s.params.size();
  } else
    return id < s.id;
}


SERIALIZE_DEF(TSentence, id, params)
SERIALIZATION_CONSTRUCTOR_IMPL(TSentence)

