#include "stdafx.h"

using namespace std;


const static string prefix = "[title]";

string View::getTitlePrefix(const string& name) {
  return prefix + name;
}

bool View::hasTitlePrefix(const string& name) {
  return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
}

string View::removeTitlePrefix(const string& name) {
  CHECK(hasTitlePrefix(name));
  return name.substr(prefix.size());
}


