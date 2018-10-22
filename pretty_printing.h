#pragma once

#include "stdafx.h"
#include "util.h"

class Effect;
class ItemType;

class PrettyPrinting {
  public:
  template<typename T>
  static optional<T> parseObject(const string& input);

  template<typename T>
  static vector<T> parseObjects(int count, const string& input);
};
