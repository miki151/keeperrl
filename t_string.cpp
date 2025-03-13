#include "t_string.h"
#include "pretty_archive.h"
#include "game_config.h"
#include "gender.h"


TString::TString(string s) : text(std::move(s)) {}
TString::TString(TSentence id) : text(std::move(id)) {}
TString::TString(TStringId id) : text(TSentence(std::move(id))) {}
TString::TString() : text(string()) {}

template <typename Archive>
void TString::serialize(Archive& ar1) {
  if (Archive::is_loading::value) {
    string s;
    ar1(s);
    if (s[0] == 'T' && s[1] == '_') {
      vector<TString> params;
      ar1(params);
      *this = TSentence(s.substr(2).data(), std::move(params));
    } else
      *this = std::move(s);
  } else {
    string out;
    text.visit(
      [&] (string s) { ar1(s); },
      [&] (TSentence s) {
        string id = "T_"_s + s.id.data();
        ar1(id);
        ar1(s.params);
    }
    );
  }
}

bool TString::empty() const {
  return text.visit(
    [](const TSentence& id) { return false; },
    [](const string& s) { return s.empty(); }
  );
}

static vector<string> readStrings(const char* filename) {
  ifstream in(filename);
  char buf[10000];
  vector<string> ret;
  while (in.good()) {
    in.getline(buf, sizeof(buf));
    ret.push_back(buf);
  }
  while (ret.back().empty())
    ret.pop_back();
  return ret;
}

static pair<string, bool> getUniqueId(PrettyInputArchive& ar, string value) {
  static unordered_map<string, string> transIds;
  string id;
  if (ar.gameConfigId)
    id = EnumInfo<GameConfigId>::getString(*ar.gameConfigId);
  if (ar.lastPrimaryId) {
    if (!id.empty())
      id += "_";
    id += toUpper(*ar.lastPrimaryId);
  }
  if (ar.lastNamedField) {
    if (!id.empty())
      id += "_";
    id += toUpper(*ar.lastNamedField);
  }
  for (auto& c : id)
    if (c == ' ')
      c = '_';
  for (int i : Range(1000)) {
    string newId = id + (i == 0 ? "" : toString(i));
    if (getReferenceMaybe(transIds, newId) == value)
      return make_pair(std::move(newId), false);
    if (!TStringId::existsId(newId.data()) && !transIds.count(newId)) {
      transIds[newId] = value;
      return make_pair(std::move(newId), true);
    }
  }
  fail();
}

static void saveFile(const char* path, const vector<string> lines) {
  ofstream out(path);
  for (auto& line : lines)
    out << line << "\n";
}

static optional<int> getIndex(const string& s, const string& line, int tryColumn) {
  if (tryColumn < line.size() && line.substr(tryColumn, s.size()) == s)
    return tryColumn;
  int index = line.find(s);
  if (index == string::npos)
    return none;
  if (line.find(s, index + 1) != string::npos)
    return none;
  return index;
}

static string addQuotes(const string& s) {
  string ret = s;
  for (int i = 0; i < ret.size(); ++i)
    if (ret[i] == '"') {
      ret.insert(i, "\\");
      ++i;
    }
  return "\"" + ret + "\"";
}

void TString::enableExportingStrings(ostream* s) {
  exportStrings = s;
}

ostream* TString::exportStrings = nullptr;;

template <>
void TString::serialize(PrettyInputArchive& ar) {
  auto firstChar = ar.peek()[0];
  if (firstChar == '"' || isdigit(firstChar)) {
    string SERIAL(s);
    ar(s);
    int numPos = 0;
    if (exportStrings && !s.empty()) {
      bool success = false;
      for (auto& pos : ar.getCurrentPosition())
        if (pos.line > -1) {
          auto lines = readStrings(ar.filenames[pos.filename].data());
          if (pos.line <= lines.size()) {
            auto toSearch = addQuotes(s);
            auto findIndex = getIndex(toSearch, lines[pos.line - 1], pos.column);
            if (findIndex) {
              auto uniqueId = getUniqueId(ar, toSearch);
              if (uniqueId.second)
                (*exportStrings) << "\"" << uniqueId.first << "\"" << " " << toSearch << "\n";
              lines[pos.line - 1].replace(*findIndex, toSearch.size(), uniqueId.first);
              saveFile(ar.filenames[pos.filename].data(), lines);
              success = true;
              break;
            }
          }
        }
      if (!success)
        std::cout << "Unexported string:" << s << std::endl;
    }
    *this = std::move(s);
  } else {
    TStringId SERIAL(id);
    vector<TString> params;
    ar(id);
    if (ar.eatMaybe("(")) {
      while (!ar.eatMaybe(")")) {
        TString SERIAL(s);
        ar(s);
        params.push_back(std::move(s));
        ar.eatMaybe(",");
      }
    }
    *this = TSentence(id, std::move(params));
  }
}

ostream& operator << (ostream& o, const TString& t) {
  t.text.visit(
      [&](const string& s) { o << s; },
      [&](const TSentence& s) {
        o << s.id.data();
        if (!s.params.empty()) {
          o << "(";
          for (int i : All(s.params)) {
            if (i > 0)
              o << ",";
            o << s.params[i];
          }
          o << ")";
        }
      }
  );
  return o;
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

TString capitalFirst(TString s) {
  return TSentence("CAPITAL_FIRST", std::move(s));
}

TString makePlural(TString s) {
  return TSentence("MAKE_PLURAL", std::move(s));
}

TString setSubjectGender(TString s, Gender gender) {
  if (gender == Gender::FEMALE)
    s = TSentence("FEMININE_SUBJECT", std::move(s));
  return s;
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
  return TSentence("NEW_LINE", std::move(v[0]), combineWithNewLine(v.getSuffix(v.size() - 1)));
}

TString combineWithOr(vector<TString> v) {
  if (v.empty())
    return TString();
  if (v.size() == 1)
    return std::move(v[0]);
  if (v.size() == 2)
    return TSentence("OR", std::move(v[0]), std::move(v[1]));
  return TSentence("COMMA", std::move(v[0]), combineWithOr(v.getSuffix(v.size() - 1)));
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


SERIALIZE_DEF(TSentence, id, params, optionalParams)
SERIALIZATION_CONSTRUCTOR_IMPL(TSentence)

