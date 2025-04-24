#pragma once

#include "stdafx.h"
#include "util.h"
#include "t_string.h"
#include "directory_path.h"

class FilePath;

class Translations {
  public:
  Translations(DirectoryPath vanillaTranslations, DirectoryPath modsDir,
      map<TStringId, TString>* sentences = nullptr);
  void loadFromDir();
  void setCurrentMods(vector<string>);
  string get(const string& language, const TString&, vector<string> form = {}) const;
  string get(const string& language, const TSentence&, vector<string> form = {}) const;
  vector<string> getLanguages() const;

  private:

  optional<string> addLanguage(string name, FilePath);
  vector<string> getTags(const string& language, const TString&) const;
  struct TranslationInfo {
    string SERIAL(primary);
    vector<string> tags;
    vector<vector<string>> otherForms;
    string getBestForm(const string& language, const vector<string>& form) const;
    void serialize(PrettyInputArchive&);
  };
  using Dictionary = HashMap<TStringId, TranslationInfo>;
  HashMap<string, Dictionary> strings;
  DirectoryPath vanillaDir;
  DirectoryPath modsDir;
  vector<DirectoryPath> currentDirs;
  map<TStringId, TString>* sentences = nullptr;
};