#include "stdafx.h"
#include "translations.h"
#include "pretty_archive.h"
#include "file_path.h"
#include "pretty_printing.h"

string Translations::get(const string& language, const TString& s, vector<string> form) const {
  return s.text.visit(
      [](const string& s) { return s; },
      [&](const TSentence& id) { return get(language, id, std::move(form)); }
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

static string addAParticle(const string& s) {
  if (isupper(s[0]))
    return s;
  if (contains({'a', 'e', 'u', 'i', 'o'}, s[0]))
    return string("an ") + s;
  else
    return string("a ") + s;
}

string Translations::TranslationInfo::getBestForm(const vector<string>& form) const {
  if (form.empty())
    return primary;
  string best = primary;
  int numMatches = 0;
  for (auto& elem : otherForms) {
    int cnt = 0;
    for (int i : Range(1, elem.size()))
      if (form.contains(elem[i]))
        ++cnt;
    if (cnt > numMatches) {
      best = elem[0];
      numMatches = cnt;
    }
  }
  return best;
}

static int getLastLetter(const string& s, int index) {
  while (index < s.size() - 1 && (isalpha(s[index + 1]) || s[index + 1] == '_'))
    ++index;
  return index;
}

const vector<string>& Translations::getTags(const string& language, const TString& s) const {
  static vector<string> empty;
  return *s.text.visit(
      [&](const string& s) -> const vector<string>* {
        return &empty;
      },
      [&](const TSentence& id) -> const vector<string>* {
        if (auto elem = getReferenceMaybe(strings.at(language), id.id))
          if (!elem->tags.empty())
            return &elem->tags;
        for (auto& param : id.params) {
          auto& tags = getTags(language, param);
          if (!tags.empty())
            return &tags;
        }
        return &empty;
      }
  );
}

string Translations::get(const string& language, const TSentence& s, vector<string> form) const {
  if (s.id == TStringId("CAPITAL_FIRST"))
    return capitalFirst(get(language, s.params[0], std::move(form)));
  if (s.id == TStringId("MAKE_PLURAL")) {
    if (language == "english")
      return makePlural(get(language, s.params[0]));
    form.push_back("plural");
    return get(language, s.params[0], std::move(form));
  }
  if (s.id == TStringId("MAKE_SENTENCE"))
    return makeSentence(get(language, s.params[0]));
  if (s.id == TStringId("A_ARTICLE")) {
    auto res = get(language, s.params[0], std::move(form));
    if (language == "english")
      return addAParticle(res);
    else
      return res;
  }
  if (auto elem = getReferenceMaybe(strings.at(language), s.id)) {
    auto sentence = elem->getBestForm(form);
    for (int i = 0; i < sentence.size(); ++i)
      if (sentence[i] == '{') {
        TString argument;
        int endArg;
        if (isdigit(sentence[i + 1])) {
          auto index = sentence[i + 1] - '1';
          endArg = i + 1;
          if (index >= 0 && index < s.params.size())
            argument = s.params[index];
        } else {
          endArg = getLastLetter(sentence, i + 1);
          string id = sentence.substr(i + 1, endArg - i);
          argument = TStringId(id.data());
        }
        auto newForms = form;
        int variableLength = 3;
        if (sentence[endArg + 1] == ':') {
          if (isdigit(sentence[endArg + 2])) {
            int tagIndex = sentence[endArg + 2] - '1';
            if (tagIndex >= 0 && tagIndex < s.params.size())
              newForms.append(getTags(language, s.params[tagIndex]));
            variableLength = 4 + endArg - i;
          } else
          do {
            int endForm = getLastLetter(sentence, i + 3);
            newForms.push_back(sentence.substr(i + 3, endForm - i - 2));
            variableLength = endForm - i + 2;
          } while (sentence[i + variableLength - 1] == ',');
        }
        auto part = get(language, std::move(argument), std::move(newForms));
        sentence.replace(i, variableLength, part);
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
  strings.clear();
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

void Translations::TranslationInfo::serialize(PrettyInputArchive& ar) {
  while (ar.peek()[0] != '\"') {
    tags.push_back(ar.peek());
    ar.eatMaybe(ar.peek());
  }
  ar(primary);
  while (ar.peek()[0] == '\"') {
    otherForms.push_back({""});
    ar(otherForms.back()[0]);
    ar.eat(":");
    do {
      auto tag = ar.peek();
      ar.eatMaybe(tag);
      otherForms.back().push_back(std::move(tag));
    } while (ar.eatMaybe(","));
  }
  if (!otherForms.empty()) {
    std::cout << primary << std::endl;
    for (auto& elem : otherForms) {
      std::cout << elem[0] << ": ";
      for (int i : Range(1, elem.size()))
        std::cout << elem[i] << " ";
      std::cout << "\n";
    }
  }
}