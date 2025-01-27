#pragma once

#include "scroll_position.h"
#include "stdafx.h"
#include "util.h"
#include "view_index.h"
#include "pretty_archive.h"
#include "keybinding.h"
#include "t_string.h"

struct ScriptedUIData;

namespace SDL {
struct SDL_Keysym;
}

namespace ScriptedUIDataElems {

using Label = TString;
struct Callback {
  function<bool()> fun;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize Callback";
  }
};
struct FocusKeysCallbacks {
  vector<pair<Keybinding, function<bool()>>> callbacks;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize KeyCallbacks";
  }
};
struct KeyCatcherCallback {
  function<void(SDL::SDL_Keysym)> fun;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize KeyCatcherCallback";
  }
};
struct DynamicWidthCallback {
  function<double()> fun;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize DynamicWidthCallback";
  }
};
struct SliderData {
  function<bool(double)> callback;
  double initialPos;
  bool continuousCallback;
  template <class Archive> void serialize(Archive& ar1, const unsigned int) {
    FATAL << "Can't deserialize SliderData";
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
  X(List, 5)\
  X(KeyCatcherCallback, 6)\
  X(DynamicWidthCallback, 7)\
  X(FocusKeysCallbacks, 8)

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
  map<int, ScrollPosition> scrollPos;
  optional<int> scrollButtonHeld;
  optional<int> highlightedElem;
  unordered_map<int, ScriptedUIDataElems::SliderState> sliderState;
  unordered_map<int, milliseconds> tooltipTimeouts;
  unordered_map<string, int> paragraphSizeCache;
  ScriptedUIData highlightNext = ScriptedUIDataElems::Callback{
      [&elem = this->highlightedElem] { elem = elem.value_or(-1) + 1; return false; }};
  ScriptedUIData highlightPrevious = ScriptedUIDataElems::Callback{
      [&elem = this->highlightedElem] { elem = elem.value_or(1) - 1; return false; }};
  ScriptedUIData exit = ScriptedUIDataElems::Callback{[] { return true; }};
};