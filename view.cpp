#include "stdafx.h"

#include "view.h"

using namespace std;


const static string prefix[] {"[title]", "[inactive]"};

string View::getModifier(ElemMod mod, const string& name) {
  return prefix[int(mod)] + name;
}

bool View::hasModifier(vector<ElemMod> mods, const string& name) {
  for (ElemMod mod : mods) {
    string pref = prefix[int(mod)];
    if (name.size() >= pref.size() && name.substr(0, pref.size()) == pref)
      return true;
  }
  return false;
}

string View::removeModifier(ElemMod mod, const string& name) {
  CHECK(hasModifier({mod}, name));
  return name.substr(prefix[int(mod)].size());
}


