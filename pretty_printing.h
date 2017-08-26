#pragma once

#include "stdafx.h"
#include "util.h"

class Effect;

class PrettyPrinting {
  public:
  static optional<Effect> getEffect(const string&);
};
