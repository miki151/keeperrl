#pragma once

#include "stdafx.h"
#include "util.h"
#include "t_string.h"
#include "directory_path.h"

class FilePath;

class Translations {
  public:
  void loadFromDir(DirectoryPath);
  string get(const string& language, const TString&) const;
  string get(const string& language, const TSentence&) const;
  vector<string> getLanguages() const;

  private:

  optional<string> addLanguage(string name, FilePath);
  using Dictionary = HashMap<TStringId, string>;
  HashMap<string, Dictionary> SERIAL(strings);
};