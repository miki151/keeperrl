#pragma once

#include "stdafx.h"
#include "util.h"
#include "texture_id.h"
#include "pretty_archive.h"
#include "mouse_button_id.h"
#include "color.h"
#include "view_id.h"

struct ScriptedUI;
struct ScriptedUIState;

namespace ScriptedUIElems {
enum class PlacementPos;
enum class Direction;
enum class TextureFlip;
enum class ButtonAction;
}

RICH_ENUM(ScriptedUIElems::TextureFlip, NONE, FLIP_X, FLIP_Y, FLIP_XY);
RICH_ENUM(ScriptedUIElems::PlacementPos, MIDDLE, TOP_STRETCHED, BOTTOM_STRETCHED, LEFT_STRETCHED, RIGHT_STRETCHED,
    TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT);
RICH_ENUM(ScriptedUIElems::Direction, HORIZONTAL, VERTICAL);
RICH_ENUM(ScriptedUIElems::ButtonAction, CALLBACK, EXIT);

namespace ScriptedUIElems {

using Fill = Color;

struct Texture {
  TextureId SERIAL(id);
  TextureFlip SERIAL(flip) = TextureFlip::NONE;
  SERIALIZE_ALL(roundBracket(), NAMED(id), OPTION(flip))
};

struct ViewId {
  int SERIAL(zoom) = 1;
  SERIALIZE_ALL(roundBracket(), OPTION(zoom))
};

struct Frame {
  int SERIAL(width);
  Color SERIAL(color);
  SERIALIZE_ALL(roundBracket(), NAMED(width), NAMED(color))
};

struct Vertical {
  vector<ScriptedUI> elems;
  int stretchedElem;
  void serialize(PrettyInputArchive& ar1, const unsigned int);
};

struct Horizontal {
  vector<ScriptedUI> SERIAL(elems);
  int stretchedElem;
  void serialize(PrettyInputArchive& ar1, const unsigned int);
};

struct Label {
  optional<string> SERIAL(text);
  optional<int> SERIAL(size);
  Color SERIAL(color) = Color::WHITE;
  SERIALIZE_ALL(roundBracket(), NAMED(text), NAMED(size), OPTION(color))
};

struct Button {
  ButtonAction SERIAL(action);
  bool SERIAL(reverse);
  SERIALIZE_ALL(roundBracket(), NAMED(action), OPTION(reverse))
};

struct MarginsImpl {
  int SERIAL(width);
  HeapAllocated<ScriptedUI> SERIAL(inside);
  SERIALIZE_ALL(roundBracket(), NAMED(width), NAMED(inside))
};

struct Position {
  HeapAllocated<ScriptedUI> SERIAL(elem);
  PlacementPos SERIAL(position);
  SERIALIZE_ALL(roundBracket(), NAMED(position), NAMED(elem))
};

struct Chain {
  vector<ScriptedUI> SERIAL(elems);
  SERIALIZE_ALL(elems)
};

struct List {
  Direction SERIAL(direction);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(roundBracket(), NAMED(direction), NAMED(elem))
};

struct Using {
  string SERIAL(key);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(key, elem)
};

struct If {
  string SERIAL(key);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(key, elem)
};

struct Width {
  int SERIAL(value);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(value, elem)
};

struct Height {
  int SERIAL(value);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(value, elem)
};

struct MouseOver {
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(elem)
};

struct Scrollable {
  HeapAllocated<ScriptedUI> SERIAL(scrollbar);
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(roundBracket(), NAMED(scrollbar), NAMED(elem))
};

struct Scroller {
  HeapAllocated<ScriptedUI> SERIAL(slider);
  SERIALIZE_ALL(slider)
};

struct ScrollButton {
  int SERIAL(direction);
  SERIALIZE_ALL(roundBracket(), NAMED(direction))
};

struct NoScissor {
  HeapAllocated<ScriptedUI> SERIAL(elem);
  SERIALIZE_ALL(elem)
};

#define VARIANT_TYPES_LIST\
  X(Texture, 0)\
  X(Fill, 1)\
  X(Frame, 2)\
  X(If, 3)\
  X(Button, 4)\
  X(MarginsImpl, 5)\
  X(Position, 6)\
  X(Chain, 7)\
  X(List, 8)\
  X(NoScissor, 9)\
  X(Label, 10)\
  X(Using, 11)\
  X(Vertical, 12)\
  X(Horizontal, 13)\
  X(Width, 14)\
  X(Height, 15)\
  X(ViewId, 16)\
  X(MouseOver, 17)\
  X(ScrollButton, 18)\
  X(Scrollable, 19)\
  X(Scroller, 20)\

#define VARIANT_NAME ScriptedUIImpl

#include "gen_variant.h"
#define DEFAULT_ELEM "Chain"
inline
#include "gen_variant_serialize_pretty.h"
#undef DEFAULT_ELEM
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

struct ScriptedUIData;
class Renderer;
class GuiFactory;

struct ScriptedContext {
  Renderer* renderer;
  GuiFactory* factory;
  Semaphore* endSemaphore;
  ScriptedUIState& state;
};

struct ScriptedUI : ScriptedUIElems::ScriptedUIImpl {
  using ScriptedUIImpl::ScriptedUIImpl;
  void render(const ScriptedUIData&, ScriptedContext, Rectangle) const;
  Vec2 getSize(const ScriptedUIData&, ScriptedContext) const;
  bool onClick(const ScriptedUIData&, ScriptedContext, MouseButtonId, Rectangle, Vec2) const;
};
