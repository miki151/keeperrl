#pragma once

#include "stdafx.h"
#include "util.h"

class Effect;
class ItemType;

class PrettyPrinting {
  public:
  template<typename T>
  static optional<string> parseObject(T& object, const string&);
};
