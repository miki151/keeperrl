#include "stdafx.h"
#include "scripted_ui.h"
#include "mouse_button_id.h"
#include "pretty_archive.h"
#include "scripted_ui_data.h"
#include "renderer.h"
#include "gui_elem.h"
#include "sdl_keycodes.h"
#include "clock.h"
#include "container_range.h"
#include "tileset.h"

namespace EnumsDetail {
enum class TextureFlip;
enum class PlacementPos;
enum class Direction;
}

RICH_ENUM(EnumsDetail::TextureFlip, NONE, FLIP_X, FLIP_Y, FLIP_XY);
RICH_ENUM(EnumsDetail::PlacementPos, MIDDLE, TOP_STRETCHED, BOTTOM_STRETCHED, LEFT_STRETCHED, RIGHT_STRETCHED,
    TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, LEFT_CENTERED, RIGHT_CENTERED, TOP_CENTERED, BOTTOM_CENTERED,
    MIDDLE_STRETCHED_X, MIDDLE_STRETCHED_Y);
RICH_ENUM(EnumsDetail::Direction, HORIZONTAL, VERTICAL);

namespace ScriptedUIElems {

using namespace EnumsDetail;

struct Texture : ScriptedUIInterface {

  const ::Texture* getTexture(ScriptedContext& context) const {
    if (id)
      return &context.factory->get(*id);
    if (auto ret = getReferenceMaybe(context.renderer->getTileSet().scriptedUITextures, scriptedId))
      return &*ret;
    return nullptr;
  }
  void render(const ScriptedUIData&, ScriptedContext& context, Rectangle area) const override {
    if (auto texture = getTexture(context))
      context.renderer->drawSprite(area.topLeft(), Vec2(0, 0), area.getSize(), *texture, none, none,
          Renderer::SpriteOrientation(flip == TextureFlip::FLIP_Y || flip == TextureFlip::FLIP_XY,
              flip == TextureFlip::FLIP_X || flip == TextureFlip::FLIP_XY));
    else
      context.renderer->drawText(Color::RED, area.topLeft(), "Texture not found \"" + scriptedId + "\"");
  }

  Vec2 getSize(const ScriptedUIData&, ScriptedContext& context) const override {
    if (auto texture = getTexture(context))
      return texture->getSize();
    else
      return Vec2(80, 20);
  }

  optional<TextureId> SERIAL(id);
  string SERIAL(scriptedId);
  TextureFlip SERIAL(flip) = TextureFlip::NONE;
  template <class Archive> void serialize(Archive& ar, const unsigned int) {
    if (ar.peek(2)[0] == '\"')
      ar(roundBracket(), NAMED(scriptedId));
    else
      ar(roundBracket(), NAMED(id));
    ar(OPTION(flip));
  }
};

REGISTER_SCRIPTED_UI(Texture);

struct ViewId : ScriptedUIInterface {
  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    if (viewId)
      context.renderer->drawViewObject(area.topLeft(), *viewId, true, zoom, Color::WHITE);
    else
    if (auto ids = data.getReferenceMaybe<ScriptedUIDataElems::ViewIdList>())
      for (auto& id : *ids)
        context.renderer->drawViewObject(area.topLeft(), id, true, zoom, Color::WHITE);
    else
      context.renderer->drawText(Color::RED, area.topLeft(), "not a viewid");
  }

  Vec2 getSize(const ScriptedUIData&, ScriptedContext& context) const override {
    return Vec2(1, 1) * context.renderer->nominalSize * zoom;
  }

  optional<::ViewId> SERIAL(viewId);
  int SERIAL(zoom) = 1;
  SERIALIZE_ALL(roundBracket(), OPTION(zoom), NAMED(viewId))
};

REGISTER_SCRIPTED_UI(ViewId);

void performAction(const ScriptedUIData& data, ScriptedContext& context, EventCallback& callback) {
  if (auto c = data.getReferenceMaybe<ScriptedUIDataElems::Callback>())
    callback = c->fun;
  else {
    USER_FATAL << "Expected callback";
    fail();
  }
}

struct Button : ScriptedUIInterface {
  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    if (id == MouseButtonId::LEFT && (pos.inRectangle(bounds) == !reverse))
      performAction(data, context, callback);
  }

  bool SERIAL(reverse);
  SERIALIZE_ALL(roundBracket(), OPTION(reverse))
};

REGISTER_SCRIPTED_UI(Button);

struct KeyReader {
  SDL::SDL_Keycode key;
  void serialize(PrettyInputArchive& ar, const unsigned int v) {
    string key = ar.eat();
    if (auto k = keycodeFromString(key.data()))
      this->key = *k;
    else
      ar.error("Unknown key value: \"" + key + "\"");
  }
};

struct KeyHandler : ScriptedUIInterface {
  void serialize(PrettyInputArchive& ar, const unsigned int v) {
    KeyReader key;
    ar(roundBracket(), NAMED(key));
    ar(endInput());
    this->key = key.key;
  }

  void onKeypressed(const ScriptedUIData& data, ScriptedContext& context,
      SDL::SDL_Keysym sym, Rectangle, EventCallback& callback) const override {
    if (key == sym.sym)
      performAction(data, context, callback);
  }

  SDL::SDL_Keycode SERIAL(key);
};

REGISTER_SCRIPTED_UI(KeyHandler);

struct Label : ScriptedUIInterface {
  Label(optional<string> text = none, int size = Renderer::textSize(), Color color = Color::WHITE)
      : text(std::move(text)), size(size), color(color) {}
  string getText(const ScriptedUIData& data) const {
    if (text)
      return *text;
    if (auto label = data.getReferenceMaybe<ScriptedUIDataElems::Label>())
      return label->data();
    else
      return "not a label";
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    context.renderer->drawText(font, size, color, area.topLeft(), getText(data));
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return Vec2(context.renderer->getTextLength(getText(data), size, font), 20);
  }

  optional<string> SERIAL(text);
  int SERIAL(size) = Renderer::textSize();
  Color SERIAL(color) = Color::WHITE;
  FontId SERIAL(font) = FontId::TEXT_FONT;
  SERIALIZE_ALL(roundBracket(), NAMED(text), OPTION(size), OPTION(color), OPTION(font))
};

REGISTER_SCRIPTED_UI(Label);

struct Paragraph : ScriptedUIInterface {
  Paragraph() {}

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    for (auto line : Iter(context.factory->breakText(text, width, size)))
      context.renderer->drawText(font, size, color, area.topLeft() + Vec2(0, line.index() * size), *line);
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return Vec2(width, size * context.factory->breakText(text, width, size).size());
  }

  string SERIAL(text);
  int SERIAL(width);
  int SERIAL(size) = Renderer::textSize();
  Color SERIAL(color) = Color::WHITE;
  FontId SERIAL(font) = FontId::TEXT_FONT;
  SERIALIZE_ALL(roundBracket(), NAMED(width), NAMED(text), OPTION(size), OPTION(color), OPTION(font))
};

REGISTER_SCRIPTED_UI(Paragraph);

struct Container : ScriptedUIInterface {
  struct SubElemInfo {
    const ScriptedUI& elem;
    const ScriptedUIData& data;
    Rectangle bounds;
  };

  virtual vector<SubElemInfo> getElemBounds(const ScriptedUIData&, ScriptedContext&, Rectangle area) const = 0;

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    for (auto& subElem : getElemBounds(data, context, area))
      subElem.elem->render(subElem.data, context, subElem.bounds);
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id, Rectangle area,
      Vec2 pos, EventCallback& callback) const override {
    for (auto& subElem : getElemBounds(data, context, area))
      subElem.elem->onClick(subElem.data, context, id, subElem.bounds, pos, callback);
  }

  void onKeypressed(const ScriptedUIData& data, ScriptedContext& context, SDL::SDL_Keysym sym,
      Rectangle bounds, EventCallback& callback) const override {
    for (auto& subElem : getElemBounds(data, context, bounds))
      subElem.elem->onKeypressed(subElem.data, context, sym, subElem.bounds, callback);
  }

  vector<SubElemInfo> getError(const string& s) const {
    static map<string, ScriptedUI> errors;
    if (!errors.count(s))
      errors[s] = ScriptedUI{unique<Label>(s)};
    return {SubElemInfo{errors.at(s), ScriptedUIData{}, Rectangle()}};
  }
};

struct MarginsImpl : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context,
      Rectangle area) const override {
    return {SubElemInfo{inside, data, area.minusMargin(width)}};
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return inside->getSize(data, context) + Vec2(width, width) * 2;
  }

  int SERIAL(width);
  ScriptedUI SERIAL(inside);
  SERIALIZE_ALL(roundBracket(), NAMED(width), NAMED(inside))
};

REGISTER_SCRIPTED_UI(MarginsImpl);

struct Fill : ScriptedUIInterface {
  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    context.renderer->drawFilledRectangle(area, color);
  }

  SERIALIZE_ALL(roundBracket(), NAMED(color));

  Color SERIAL(color);
};

REGISTER_SCRIPTED_UI(Fill);

struct Frame : ScriptedUIInterface {
  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    context.renderer->drawFilledRectangle(area, Color::TRANSPARENT, color);
  }

  SERIALIZE_ALL(roundBracket(), NAMED(color));

  Color SERIAL(color);
};

REGISTER_SCRIPTED_UI(Frame);

struct Position : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context,
      Rectangle area) const override {
    auto size = elem->getSize(data, context);
    switch (position) {
      case PlacementPos::MIDDLE_STRETCHED_X:
        return {SubElemInfo{elem, data, Rectangle(area.left(), area.middle().y - size.y / 2,
            area.right(), area.middle().y - size.y / 2 + size.y)}};
      case PlacementPos::MIDDLE_STRETCHED_Y:
        return {SubElemInfo{elem, data, Rectangle(area.middle().x - size.x / 2, area.top(),
            area.middle().x - size.x / 2 + size.x, area.bottom())}};
      case PlacementPos::MIDDLE:
        return {SubElemInfo{elem, data, Rectangle(area.middle() - size / 2, area.middle() + size / 2)}};
      case PlacementPos::TOP_STRETCHED:
        return {SubElemInfo{elem, data, Rectangle(area.topLeft(), area.topRight() + Vec2(0, size.y))}};
      case PlacementPos::BOTTOM_STRETCHED:
        return {SubElemInfo{elem, data, Rectangle(area.bottomLeft() - Vec2(0, size.y), area.bottomRight())}};
      case PlacementPos::LEFT_STRETCHED:
        return {SubElemInfo{elem, data, Rectangle(area.topLeft(), area.bottomLeft() + Vec2(size.x, 0))}};
      case PlacementPos::RIGHT_STRETCHED:
        return {SubElemInfo{elem, data, Rectangle(area.topRight() - Vec2(size.x, 0), area.bottomRight())}};
      case PlacementPos::TOP_LEFT:
        return {SubElemInfo{elem, data, Rectangle(area.topLeft(), area.topLeft() + size)}};
      case PlacementPos::TOP_RIGHT:
        return {SubElemInfo{elem, data, Rectangle(area.topRight() - Vec2(size.x, 0), area.topRight() + Vec2(0, size.y))}};
      case PlacementPos::BOTTOM_RIGHT:
        return {SubElemInfo{elem, data, Rectangle(area.bottomRight() - size, area.bottomRight())}};
      case PlacementPos::BOTTOM_LEFT:
        return {SubElemInfo{elem, data, Rectangle(area.bottomLeft() - Vec2(0, size.y), area.bottomLeft() + Vec2(size.x, 0))}};
      case PlacementPos::TOP_CENTERED:
        return {SubElemInfo{elem, data, Rectangle(area.middle().x - size.x / 2, area.top(),
            area.middle().x - size.x / 2 + size.x, area.top() + size.y)}};
      case PlacementPos::BOTTOM_CENTERED:
        return {SubElemInfo{elem, data, Rectangle(area.middle().x - size.x / 2, area.bottom() - size.y,
            area.middle().x - size.x / 2 + size.x, area.bottom())}};
      case PlacementPos::LEFT_CENTERED:
        return {SubElemInfo{elem, data, Rectangle(area.left(), area.middle().y - size.y / 2,
            area.left() + size.x, area.middle().y - size.y / 2 + size.y)}};
      case PlacementPos::RIGHT_CENTERED:
        return {SubElemInfo{elem, data, Rectangle(area.right() - size.x, area.middle().y - size.y / 2,
            area.right(), area.middle().y - size.y / 2 + size.y)}};
    }
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return elem->getSize(data, context);
  }

  ScriptedUI SERIAL(elem);
  PlacementPos SERIAL(position);
  SERIALIZE_ALL(roundBracket(), NAMED(position), NAMED(elem))
};

REGISTER_SCRIPTED_UI(Position);

struct Chain : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context,
      Rectangle area) const override {
    return elems.transform([&](auto& elem) { return SubElemInfo{elem, data, area}; });
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    Vec2 res;
    for (auto& elem : elems) {
      auto size = elem->getSize(data, context);
      res.x = max(res.x, size.x);
      res.y = max(res.y, size.y);
    }
    return res;
  }

  vector<ScriptedUI> SERIAL(elems);
  SERIALIZE_ALL(elems)
};

REGISTER_SCRIPTED_UI(Chain);

struct Width : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    return {SubElemInfo{elem, data, area}};
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return Vec2(value, elem->getSize(data, context).y);
  }

  int SERIAL(value);
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(value, elem)
};

REGISTER_SCRIPTED_UI(Width);

struct Height : Width {
  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return Vec2(elem->getSize(data, context).x, value);
  }
};

REGISTER_SCRIPTED_UI(Height);

struct MaxWidth : Width {
  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    auto size = elem->getSize(data, context);
    return Vec2(min(size.x, value), size.y);
  }
};

REGISTER_SCRIPTED_UI(MaxWidth);

struct MaxHeight : Width {
  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    auto size = elem->getSize(data, context);
    return Vec2(size.x, min(size.y, value));
  }
};

REGISTER_SCRIPTED_UI(MaxHeight);

static vector<Range> getStaticListBounds(Range total, vector<int> widths, int stretched) {
  vector<Range> ret;
  ret.reserve(widths.size());
  auto startPos = total.getStart();
  for (int i : Range(stretched)) {
    auto width = widths[i];
    ret.push_back(Range(startPos, startPos + width));
    startPos += width;
  }
  vector<Range> backElems;
  backElems.reserve(widths.size() - stretched);
  auto endPos = total.getEnd();
  for (int i = widths.size() - 1; i > stretched; --i) {
    auto width = widths[i];
    backElems.push_back(Range(endPos - width, endPos));
    endPos -= width;
  }
  if (stretched < widths.size())
    ret.push_back(Range(startPos, endPos));
  for (int i : All(backElems).reverse())
    ret.push_back(std::move(backElems[i]));
  return ret;
}

template <typename T>
static void serializeVerticalHorizontal(T& elem, PrettyInputArchive& ar1, const unsigned int) {
  auto bracketType = BracketType::CURLY;
  ar1.openBracket(bracketType);
  elem.stretchedElem = -1;
  while (!ar1.isCloseBracket(bracketType)) {
    ScriptedUI t;
    if (ar1.eatMaybe("Stretch"))
      elem.stretchedElem = elem.elems.size();
    ar1(t);
    elem.elems.push_back(std::move(t));
  }
  if (elem.stretchedElem == -1)
    elem.stretchedElem = elem.elems.size();
  ar1.closeBracket(bracketType);
}

struct Horizontal : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    auto ranges = getStaticListBounds(area.getXRange(),
        elems.transform([&](auto& elem) { return elem->getSize(data, context).x; }), stretchedElem);
    vector<SubElemInfo> ret;
    ret.reserve(ranges.size());
    for (int i : All(ranges))
      ret.push_back(SubElemInfo{elems[i], data, Rectangle(ranges[i].getStart(), area.top(), ranges[i].getEnd(), area.bottom())});
    return ret;
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    Vec2 res;
    for (auto& elem : elems) {
      auto size = elem->getSize(data, context);
      res.x += size.x;
      res.y = max(res.y, size.y);
    }
    return res;
  }

  vector<ScriptedUI> elems;
  int stretchedElem;
  void serialize(PrettyInputArchive& ar1, const unsigned int v) {
    serializeVerticalHorizontal(*this, ar1, v);
  }
};

REGISTER_SCRIPTED_UI(Horizontal);

struct Vertical : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
  auto ranges = getStaticListBounds(area.getYRange(),
      elems.transform([&](auto& elem) { return elem->getSize(data, context).y; }), stretchedElem);
  vector<SubElemInfo> ret;
  ret.reserve(ranges.size());
  for (int i : All(ranges)) {
    ret.push_back(SubElemInfo{elems[i], data, Rectangle(area.left(), ranges[i].getStart(), area.right(), ranges[i].getEnd())});
  }
  return ret;
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    Vec2 res;
    for (auto& elem : elems) {
      auto size = elem->getSize(data, context);
      res.x = max(res.x, size.x);
      res.y += size.y;
    }
    return res;
  }

  vector<ScriptedUI> elems;
  int stretchedElem;
  void serialize(PrettyInputArchive& ar1, const unsigned int v) {
    serializeVerticalHorizontal(*this, ar1, v);
  }
};

REGISTER_SCRIPTED_UI(Vertical);

struct Using : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    if (key == "EXIT")
      return {SubElemInfo{elem, context.state.exit, area}};
    else
    if (key == "HIGHLIGHT_NEXT")
      return {SubElemInfo{elem, context.state.highlightNext, area}};
    else
    if (key == "HIGHLIGHT_PREVIOUS")
      return {SubElemInfo{elem, context.state.highlightPrevious, area}};
    else
    if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
      if (auto value = getReferenceMaybe(record->elems, key))
        return {SubElemInfo{elem, *value, area}};
      else
        return {};
    } else
      return getError("not a record");
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    if (isOneOf(key, "EXIT", "HIGHLIGHT_NEXT", "HIGHLIGHT_PREVIOUS")) {
      static ScriptedUIData fun = ScriptedUIDataElems::Callback{[] { return true; }};
      return elem->getSize(fun, context);
    } else
    if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
      if (auto value = getReferenceMaybe(record->elems, key))
        return elem->getSize(*value, context);
      else
        return Vec2(0, 0);
    }
    return Vec2(100, 20);
  }
  string SERIAL(key);
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(key, elem)
};

REGISTER_SCRIPTED_UI(Using);

struct Focusable : ScriptedUIInterface {
  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return elem->getSize(data, context);
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const override {
    if (context.state.highlightedElem == context.elemCounter)
      elem->render(data, context, bounds);
    ++context.elemCounter;
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
    Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    if (id == MouseButtonId::MOVED) {
      if (pos.inRectangle(bounds))
        callback = [counter = context.elemCounter, &context] { context.state.highlightedElem = counter; return false; };
      else
      if (context.state.highlightedElem == context.elemCounter)
        callback = [&] { context.state.highlightedElem = none; return false;};
    }
    ++context.elemCounter;
  }

  void onKeypressed(const ScriptedUIData& data, ScriptedContext& context,
      SDL::SDL_Keysym sym, Rectangle bounds, EventCallback& callback) const override {
    if (isOneOf(sym.sym, SDL::SDLK_KP_ENTER, SDL::SDLK_RETURN) && context.elemCounter == context.state.highlightedElem)
      performAction(data, context, callback);
    callback = [&, y = bounds.middle().y, callback, myCounter = context.elemCounter] {
      auto ret = callback ? callback() : false;
      if (context.state.highlightedElem == myCounter)
        context.highlightedElemHeight = y;
      return ret;
    };
    ++context.elemCounter;
  }
  
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(elem)
};

REGISTER_SCRIPTED_UI(Focusable);

struct MouseOver : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    return {SubElemInfo{elem, data, area}};
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return elem->getSize(data, context);
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const override {
    if (context.renderer->getMousePos().inRectangle(bounds))
      elem->render(data, context, bounds);
  }
  
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(elem)
};

REGISTER_SCRIPTED_UI(MouseOver);

struct List : Container {
  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    Vec2 startPos = area.topLeft();
    if (auto list = data.getReferenceMaybe<ScriptedUIDataElems::List>()) {
      vector<SubElemInfo> ret;
      ret.reserve(list->size());
      for (auto& dataElem : *list) {
        Rectangle thisArea;
        Vec2 nextPos;
        if (direction == Direction::HORIZONTAL) {
          auto width = elem->getSize(dataElem, context).x;
          thisArea = Rectangle(startPos, startPos + Vec2(width, area.height()));
          nextPos = startPos + Vec2(width, 0);
        } else {
          auto height = elem->getSize(dataElem, context).y;
          thisArea = Rectangle(startPos, startPos + Vec2(area.width(), height));
          nextPos = startPos + Vec2(0, height);
        }
        ret.push_back(SubElemInfo{elem, dataElem, thisArea});
        startPos = nextPos;
      }
      return ret;
    } else
      return getError("not a list");
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    if (auto list = data.getReferenceMaybe<ScriptedUIDataElems::List>()) {
      Vec2 res;
      for (auto& dataElem : *list) {
        auto size = elem->getSize(dataElem, context);
        if (direction == Direction::VERTICAL) {
          res.y += size.y;
          res.x = max(res.x, size.x);
        } else {
          res.y = max(res.y, size.y);
          res.x += size.x;
        }
      }
      return res;
    }
    return Vec2(100, 20);
  }

  Direction SERIAL(direction);
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(roundBracket(), NAMED(direction), NAMED(elem))
};

REGISTER_SCRIPTED_UI(List);

static void scroll(const ScriptedContext& context, int dir) {
  context.state.scrollPos.add(100 * dir, Clock::getRealMillis());
}

struct Scrollable : ScriptedUIInterface {
  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return elem->getSize(data, context);
  }

  static Rectangle getContentBounds(Rectangle bounds, int offset, int scrollBarWidth, int elemHeight) {
    return Rectangle(bounds.left(), bounds.top() - offset,
          bounds.right() - scrollBarWidth, bounds.top() - offset + elemHeight);
  }

  static Rectangle getScrollBarBounds(Rectangle bounds, int offset, int scrollBarWidth) {
    return Rectangle(bounds.topRight() - Vec2(scrollBarWidth, 0), bounds.bottomRight());
  }

  template <typename ContentFun, typename ScrollbarFun>
  void processScroller(const ScriptedUIData& data, ScriptedContext& context,
      Rectangle bounds, ContentFun contentFun, ScrollbarFun scrollbarFun) const {
    auto height = elem->getSize(data, context).y;
    if (height <= bounds.height())
      contentFun(bounds);
    else {
      int offset = context.state.scrollPos.get(Clock::getRealMillis(), 0, height - bounds.height());
      int scrollBarWidth = scrollbar->getSize(data, context).x;
      auto contentBounds = getContentBounds(bounds, offset, scrollBarWidth, height);
      context.renderer->setScissor(bounds);
      contentFun(contentBounds);
      context.renderer->setScissor(none);
      scrollbarFun(getScrollBarBounds(bounds, offset, scrollBarWidth));
    }
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const override {
    processScroller(data, context, bounds,
        [&] (Rectangle contentBounds) { elem->render(data, context, contentBounds); },
        [&] (Rectangle bounds) { scrollbar->render(data, context, bounds); }      
    );
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    if (pos.inRectangle(bounds)) {
      if (id == MouseButtonId::WHEEL_DOWN)
        callback = [&] { scroll(context, 1); return false; };
      else
      if (id == MouseButtonId::WHEEL_UP)
        callback = [&] { scroll(context, -1); return false; };
    }
    processScroller(data, context, bounds,
        [&] (Rectangle bounds) { elem->onClick(data, context, id, bounds, pos, callback); },
        [&] (Rectangle bounds) { scrollbar->onClick(data, context, id, bounds, pos, callback); }
    );
  }

  static bool needsScrolling(Range range, int position) {
    return position < range.getStart() + range.getLength() / 3 || position > range.getEnd() - range.getLength() / 3;
  }

  void onKeypressed(const ScriptedUIData& data, ScriptedContext& context,
      SDL::SDL_Keysym sym, Rectangle bounds, EventCallback& callback) const override {
    processScroller(data, context, bounds,
        [&] (Rectangle contentBounds) {
          elem->onKeypressed(data, context, sym, contentBounds, callback);
          callback = [&context, callback, bounds] {
            context.highlightedElemHeight = none;
            auto ret = callback ? callback() : false;
            if (auto height = context.highlightedElemHeight)
              if (needsScrolling(bounds.getYRange(), *height) &&
                  !context.state.scrollPos.isScrolling(Clock::getRealMillis()))
                context.state.scrollPos.add(*height - bounds.middle().y, Clock::getRealMillis());
            context.highlightedElemHeight = none;
            return ret;
          };
        },
        [&] (Rectangle bounds) { scrollbar->onKeypressed(data, context, sym, bounds, callback); }
    );
  }

  ScriptedUI SERIAL(scrollbar);
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(roundBracket(), NAMED(scrollbar), NAMED(elem))
};

REGISTER_SCRIPTED_UI(Scrollable);

struct ScrollButton : ScriptedUIInterface {
  void onClick(const ScriptedUIData&, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    if (id == MouseButtonId::LEFT && pos.inRectangle(bounds))
      callback = [&] { scroll(context, direction); return false; };
  }
  int SERIAL(direction);
  SERIALIZE_ALL(roundBracket(), NAMED(direction))
};

REGISTER_SCRIPTED_UI(ScrollButton);

struct Scroller : ScriptedUIInterface {
  Rectangle getSliderPos(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const {
    auto height = slider->getSize(data, context).y;
    if (auto& held = context.state.scrollButtonHeld)
      context.state.scrollPos.setRatio(
          double(context.renderer->getMousePos().y - *held - bounds.top()) / (bounds.height() - height),
          Clock::getRealMillis());
    auto pos = bounds.top() + context.state.scrollPos.getRatio(
        Clock::getRealMillis()) * (bounds.height() - height);
    return Rectangle(bounds.left(), pos, bounds.right(), pos + height);
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    if (id == MouseButtonId::LEFT && pos.inRectangle(bounds)) {
      auto sliderHeight = slider->getSize(data, context).y;
      auto sliderPos = getSliderPos(data, context, bounds);
      callback = [pos, &context, bounds, sliderHeight, sliderPos] {
        context.state.scrollPos.setRatio(double(pos.y - sliderHeight / 2 - bounds.top()) / (bounds.height() - sliderHeight),
            Clock::getRealMillis());
        if (pos.inRectangle(sliderPos))
          context.state.scrollButtonHeld = pos.y - sliderPos.top();
        return false;
      };
    }
    else if (id == MouseButtonId::RELEASED)
      callback = [&] { context.state.scrollButtonHeld = none; return false; };
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return slider->getSize(data, context);
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const override {
    slider->render(data, context, getSliderPos(data, context, bounds));
  }

  ScriptedUI SERIAL(slider);
  SERIALIZE_ALL(slider)
};

REGISTER_SCRIPTED_UI(Scroller);

struct Slider : ScriptedUIInterface {
  Rectangle getSliderPos(const ScriptedUIDataElems::SliderState& state, const ScriptedUIData& data, ScriptedContext& context,
      Rectangle bounds) const {
    auto width = slider->getSize(data, context).x;
    auto pos = bounds.left() + state.sliderPos * (bounds.width() - width);
    return Rectangle(pos, bounds.top(), pos + width, bounds.bottom());
  }

  auto& getSliderState(const ScriptedUIDataElems::SliderData& data, ScriptedContext& context) const {
    ++context.sliderCounter;
    if (!context.state.sliderState.count(context.sliderCounter))
      context.state.sliderState.insert({context.sliderCounter, ScriptedUIDataElems::SliderState{data.initialPos, false}});
    return context.state.sliderState.at(context.sliderCounter);
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    auto sliderData = data.getReferenceMaybe<ScriptedUIDataElems::SliderData>();
    if (!sliderData)
      return;
    auto& state = getSliderState(*sliderData, context);
    if ((id == MouseButtonId::LEFT && pos.inRectangle(bounds)) || (id == MouseButtonId::MOVED && state.sliderHeld)) {
      auto sliderWidth = slider->getSize(data, context).x;
      auto& posCallback = sliderData->callback;
      callback = [pos, &state, bounds, sliderWidth, &posCallback,
          continuous = sliderData->continuousCallback] {
        auto value = max(0.0, min(1.0, double(pos.x - sliderWidth / 2 - bounds.left()) / (bounds.width() - sliderWidth)));
        state.sliderPos = value;
        state.sliderHeld = true;
        if (continuous)
          return posCallback(value);
        return false;
      };
    } else
    if (id == MouseButtonId::RELEASED && state.sliderHeld) {
      if (!sliderData->continuousCallback) {
        auto& posCallback = sliderData->callback;
        callback = [&state, &posCallback] {
          state.sliderHeld = false;
          return posCallback(state.sliderPos);
        };
      } else
        callback = [&state] {
          state.sliderHeld = false;
          return false;
        };
    }
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return slider->getSize(data, context);
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) const override {
    auto sliderData = data.getReferenceMaybe<ScriptedUIDataElems::SliderData>();
    if (!sliderData)
      return;
    auto& state = getSliderState(*sliderData, context);
    slider->render(data, context, getSliderPos(state, data, context, bounds));
  }

  ScriptedUI SERIAL(slider);
  SERIALIZE_ALL(slider)
};

REGISTER_SCRIPTED_UI(Slider);

struct Tooltip : Container {
  Rectangle getTooltipBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const {
    return Rectangle(area.bottomLeft(), area.bottomLeft() + elem->getSize(data, context)).translate(Vec2(15, 15));
  }

  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    ++context.tooltipCounter;
    auto time = getValueMaybe(context.state.tooltipTimeouts, context.tooltipCounter);
    if (time && *time <= Clock::getRealMillis()) {
      context.renderer->setTopLayer();
      elem->render(data, context, getTooltipBounds(data, context, area));
      context.renderer->popLayer();
    }
  }

  void onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
      Rectangle bounds, Vec2 pos, EventCallback& callback) const override {
    ++context.tooltipCounter;
    auto time = getValueMaybe(context.state.tooltipTimeouts, context.tooltipCounter);
    if (id == MouseButtonId::MOVED) {
      if (pos.inRectangle(bounds) && !time)
        callback = [&, counter = context.tooltipCounter] {
          context.state.tooltipTimeouts.insert({counter, Clock::getRealMillis() + milliseconds{500}});
          return false;
        };
      if (!pos.inRectangle(bounds) && context.state.tooltipTimeouts.count(context.tooltipCounter))
        callback = [&, counter = context.tooltipCounter] {
          context.state.tooltipTimeouts.erase(counter);
          return false;
        };
    }
  }

  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    return {SubElemInfo{elem, data, getTooltipBounds(data, context, area)}};
  }
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(roundBracket(), NAMED(elem))
};

REGISTER_SCRIPTED_UI(Tooltip);

struct Scissor : Container {
  void render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    context.renderer->setScissor(area);
    elem->render(data, context, area);
    context.renderer->setScissor(none);
  }

  Vec2 getSize(const ScriptedUIData& data, ScriptedContext& context) const override {
    return elem->getSize(data, context);
  }

  vector<SubElemInfo> getElemBounds(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const override {
    return {SubElemInfo{elem, data, area}};
  }
  ScriptedUI SERIAL(elem);
  SERIALIZE_ALL(elem)
};
REGISTER_SCRIPTED_UI(Scissor);

}

void ScriptedUIInterface::render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const {
}

Vec2 ScriptedUIInterface::getSize(const ScriptedUIData& data, ScriptedContext& context) const {
  return Vec2(0, 0);
}

void ScriptedUIInterface::onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id, Rectangle bounds,
    Vec2 pos, EventCallback& callback) const {
}

void ScriptedUIInterface::onKeypressed(const ScriptedUIData& data, ScriptedContext& context, SDL::SDL_Keysym sym,
    Rectangle bounds, EventCallback& callback) const {
}

ScriptedUIInterface* ScriptedUI::operator->() {
  return ui.get();
}

const ScriptedUIInterface* ScriptedUI::operator->() const {
  return ui.get();
}

ScriptedUIInterface& ScriptedUI::operator*() {
  return *ui.get();
}

const ScriptedUIInterface& ScriptedUI::operator*() const {
  return *ui.get();
}

struct ScriptedUI::SubclassInfo {
  string name;
  SubclassSerialize serialize;
};

vector<ScriptedUI::SubclassInfo>& ScriptedUI::getSubclasses() {
  static vector<SubclassInfo> ret;
  return ret;
}

void ScriptedUI::serialize(PrettyInputArchive& ar1, const unsigned int) {
  string name;
  auto bookmark = ar1.bookmark();
  ar1.readText(name);
  for (auto& elem : getSubclasses())
    if (elem.name == name) {
      ui = elem.serialize(ar1);
      return;
    }
  name = "Chain";
  ar1.seek(bookmark);
  for (auto& elem : getSubclasses())
    if (elem.name == name) {
      ui = elem.serialize(ar1);
      break;
    }
}
ScriptedSubclassAdder::ScriptedSubclassAdder(const char* name, SubclassSerialize s) {
  ScriptedUI::getSubclasses().push_back(ScriptedUI::SubclassInfo{name, std::move(s)});
}
