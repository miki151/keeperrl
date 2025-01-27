#pragma once

#include "stdafx.h"
#include "util.h"
#include "t_string.h"

class Translations {
  public:
  string get(const string& language, const TString&) const;
  string get(const string& language, const TSentence&) const;
  void serialize(PrettyInputArchive& ar, const unsigned int);
  vector<string> getLanguages() const;

  private:

  using Dictionary = HashMap<TStringId, string>;
  HashMap<string, Dictionary> SERIAL(strings);
};