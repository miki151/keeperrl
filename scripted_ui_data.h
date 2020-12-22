#pragma once

#include "scroll_position.h"
#include "stdafx.h"
#include "util.h"
#include "view_index.h"
#include "pretty_archive.h"

struct ScriptedUIData;

namespace ScriptedUIDataElems {

using Label = string;
struct Callback {
  function<bool()> fun;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize Callback";
  }
};
struct SliderData {
  function<bool(double)> callback;
  double initialPos;
  bool continuousCallback;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize Callback";
  }
};
struct SliderState {
  double sliderPos = 0;
  bool sliderHeld = false;
};
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
  X(SliderData, 3)\
  X(Record, 4)\
  X(List, 5)

#define VARIANT_NAME ScriptedUIDataImpl
#include "gen_variant.h"
#include "gen_variant_serialize.h"
#define DEFAULT_ELEM "Record"
inline
#include "gen_variant_serialize_pretty.h"
#undef DEFAULT_ELEM
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

struct ScriptedUIData : ScriptedUIDataElems::ScriptedUIDataImpl {
  using ScriptedUIDataImpl::ScriptedUIDataImpl;
};


struct ScriptedUIState {
  ScrollPosition scrollPos;
  optional<int> scrollButtonHeld;
  optional<int> highlightedElem;
  unordered_map<int, ScriptedUIDataElems::SliderState> sliderState;
  unordered_map<int, milliseconds> tooltipTimeouts;
  ScriptedUIData highlightNext = ScriptedUIDataElems::Callback{
      [&elem = this->highlightedElem] { elem = elem.value_or(-1) + 1; return false; }};
  ScriptedUIData highlightPrevious = ScriptedUIDataElems::Callback{
      [&elem = this->highlightedElem] { elem = elem.value_or(1) - 1; return false; }};
  ScriptedUIData exit = ScriptedUIDataElems::Callback{[] { return true; }};
};