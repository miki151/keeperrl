#pragma once

#include "stdafx.h"
#include "util.h"
#include "view_index.h"
#include "pretty_archive.h"

struct ScriptedUIData;

namespace ScriptedUIDataElems {

using Label = string;
using Callback = function<void()>;
struct Record {
  map<string, ScriptedUIData> SERIAL(elems);
  SERIALIZE_ALL(elems)
};
using List = vector<ScriptedUIData>;
using ::ViewIdList;

#define VARIANT_TYPES_LIST\
  X(Label, 0)\
  X(ViewIdList, 1)\
  X(Callback, 2)\
  X(Record, 3)\
  X(List, 4)

#define VARIANT_NAME ScriptedUIDataImpl
#include "gen_variant.h"
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

struct ScriptedUIData : ScriptedUIDataElems::ScriptedUIDataImpl {
  using ScriptedUIDataImpl::ScriptedUIDataImpl;
};

struct ScriptedUIState {
  double scrollPos;
  optional<int> scrollButtonHeld;
};