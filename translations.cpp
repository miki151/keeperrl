#include "stdafx.h"
#include "translations.h"
#include "pretty_archive.h"
#include "file_path.h"
#include "pretty_printing.h"

string Translations::get(const string& language, const TString& s) const {
  return s.text.visit(
      [](const string& s) { return s; },
      [&](const TSentence& id) { return get(language, id); }
  );
}

static string makePlural(const string& s) {
  if (s.empty())
    return "";
  if (s.back() == 'y')
    return s.substr(0, s.size() - 1) + "ies";
  if (endsWith(s, "ph"))
    return s + "s";
  if (s.back() == 'h')
    return s + "es";
  if (s.back() == 's')
    return s;
  if (endsWith(s, "shelf"))
    return s.substr(0, s.size() - 5) + "shelves";
  return s + "s";
}

string addAParticle(const string& s) {
  if (isupper(s[0]))
    return s;
  if (contains({'a', 'e', 'u', 'i', 'o'}, s[0]))
    return string("an ") + s;
  else
    return string("a ") + s;
}

string Translations::get(const string& language, const TSentence& s) const {
  if (s.id == TStringId("CAPITAL_FIRST"))
    return capitalFirst(get(language, s.params[0]));
  if (s.id == TStringId("MAKE_PLURAL"))
    return makePlural(get(language, s.params[0]));
  if (s.id == TStringId("MAKE_SENTENCE"))
    return makeSentence(get(language, s.params[0]));
  if (s.id == TStringId("A_ARTICLE"))
    return addAParticle(get(language, s.params[0]));
  if (auto elem = getReferenceMaybe(strings.at(language), s.id)) {
    auto sentence = *elem;
    for (int i = 0; i < sentence.size(); ++i)
      if (sentence[i] == '{') {
        auto index = sentence[i + 1] - '1';
        string part;
        if (index >= 0 && index < s.params.size())
          part = get(language, s.params[index]);
        else if (!s.optionalParams)
          part = "<"_s + s.id.data() + " bad index " + toString(index) + ">";
        sentence.replace(i, 3, part);
      }
    return sentence;
  } else {
    string ret = s.id.data();
    if (!s.params.empty()) {
      ret.push_back('(');
      for (int i : All(s.params)) {
        ret += get(language, s.params[i]);
        if (i < s.params.size() - 1)
          ret += ",";
      }
      ret.push_back(')');
    }
    return ret;
  }
}

optional<string> Translations::addLanguage(string name, FilePath path) {
  Dictionary dict;
  auto res = PrettyPrinting::parseObject(dict, {*path.readContents()}, {string(path.getPath())});
  if (!res)
    strings.insert(make_pair(std::move(name), std::move(dict)));
  return res;
}

void Translations::loadFromDir(DirectoryPath dir) {
  for (auto file : dir.getFiles())
    if (file.hasSuffix(".txt")) {
      auto error = addLanguage(file.changeSuffix(".txt", "").getFileName(), file);
      if (error)
        USER_FATAL << *error;
    }
}

vector<string> Translations::getLanguages() const {
  return getKeys(strings);
}
