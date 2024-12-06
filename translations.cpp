#include "stdafx.h"
#include "translations.h"
#include "pretty_archive.h"

string Translations::get(string language, const TString& s) const {
  return s.visit(
      [](const string& s) { return s; },
      [&](const TStringId& id) { return strings.at(language).at(id); }
  );
}


void Translations::serialize(PrettyInputArchive& ar, const unsigned int version) {
  HashMap<string, map<PrimaryId<TStringId>, string>> tmp;
  ar(tmp);
  for (auto& elem : tmp)
    strings.insert(make_pair(elem.first, convertKeysHash(elem.second)));
}

vector<string> Translations::getLanguages() const {
  return getKeys(strings);
}
