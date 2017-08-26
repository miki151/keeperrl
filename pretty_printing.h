#pragma once

#include "stdafx.h"
#include "util.h"

class EffectType;

class PrettyPrinting {
  public:
  static optional<EffectType> getEffect(const string&);
};
