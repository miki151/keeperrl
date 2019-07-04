#pragma once

#include "util.h"

class KeyVerifier {
  public:
  struct Verifier {
    set<string> toVerify;
    set<string> keys;
    vector<string> duplicateKeys;
  };

  template <typename T>
  void verifyContentId(string id) {
    verifiers[typeid(T)].toVerify.insert(std::move(id));
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
