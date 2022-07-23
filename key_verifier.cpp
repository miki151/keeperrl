#include "stdafx.h"
#include "key_verifier.h"


vector<string> KeyVerifier::verify() {
  vector<string> ret;
  for (auto& verifier : verifiers) {
    /*for (auto& id : verifier.second.duplicateKeys)
      ret.push_back("Duplicate "_s + verifier.first.name() + " key: \"" + id + "\"");*/
    for (auto& id : verifier.second.toVerify)
      if (!verifier.second.keys.count(id.name))
        ret.push_back(id.position + ": " + verifier.first.name() + " not found: \""_s + id.name + "\"");
  }
  return ret;
}
