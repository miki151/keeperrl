#pragma once

#include "util.h"
#include "pretty_archive.h"

class KeyVerifier {
  public:
  struct KeyInfo {
    string name;
    string position;
  };
  struct Verifier {
    vector<KeyInfo> toVerify;
    set<string> keys;
    vector<string> duplicateKeys;
  };

  template <typename T>
  void verifyContentId(string pos, string id) {
    verifiers[typeid(T)].toVerify.push_back({std::move(id), std::move(pos)});
  }

  template <typename T>
  void addKey(string id) {
    auto& v = verifiers[typeid(T)];
    if (v.keys.count(id))
      v.duplicateKeys.push_back(id);
    v.keys.insert(std::move(id));
  }

  vector<string> verify();

  private:
  map<std::type_index, Verifier> verifiers;
};
