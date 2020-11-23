#include "stdafx.h"
#include "scripted_ui.h"
#include "mouse_button_id.h"
#include "pretty_archive.h"
#include "scripted_ui_data.h"
#include "renderer.h"
#include "gui_elem.h"
#include "sdl_keycodes.h"

namespace {
struct DefaultType {
  template <typename T>
  DefaultType(const T&) {}
};
}

static Vec2 getSize(const DefaultType&, const ScriptedUIData&, ScriptedContext&) {
  return Vec2(0, 0);
}

static void render(const DefaultType&, const ScriptedUIData&, ScriptedContext&, Rectangle) {
}

static void onClick(const DefaultType&, const ScriptedUIData&, ScriptedContext&, MouseButtonId, Rectangle, Vec2,
    EventCallback&) {
}

static void onKeypressed(const DefaultType&, const ScriptedUIData&, ScriptedContext&, SDL::SDL_Keysym, EventCallback&) {
}

static void render(const ScriptedUIElems::Texture& t, const ScriptedUIData&, ScriptedContext& context, Rectangle area) {
  auto& texture = context.factory->get(t.id);
  using namespace ScriptedUIElems;
  context.renderer->drawSprite(area.topLeft(), Vec2(0, 0), area.getSize(), texture, none, none,
      Renderer::SpriteOrientation(t.flip == TextureFlip::FLIP_Y || t.flip == TextureFlip::FLIP_XY,
          t.flip == TextureFlip::FLIP_X || t.flip == TextureFlip::FLIP_XY));
}

static Vec2 getSize(const ScriptedUIElems::Texture& t, const ScriptedUIData&, ScriptedContext& context) {
  return context.factory->get(t.id).getSize();
}

static void render(const ScriptedUIElems::ViewId& t, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  if (auto ids = data.getReferenceMaybe<ScriptedUIDataElems::ViewIdList>())
    for (auto& id : *ids)
      context.renderer->drawViewObject(area.topLeft(), id, true, t.zoom, Color::WHITE);
  else
    context.renderer->drawText(Color::RED, area.topLeft(), "not a viewid");
}

static Vec2 getSize(const ScriptedUIElems::ViewId& t, const ScriptedUIData&, ScriptedContext& context) {
  return Vec2(1, 1) * context.renderer->nominalSize * t.zoom;
}

static void render(const ScriptedUIElems::Button&, const ScriptedUIData&, ScriptedContext& context, Rectangle area) {
//  context.renderer->drawFilledRectangle(area, Color::RED);
}

static void performAction(const ScriptedUIData& data, ScriptedContext& context, EventCallback& callback) {
  if (auto c = data.getReferenceMaybe<ScriptedUIDataElems::Callback>())
    callback = c->fun;
  else {
    USER_FATAL << "Expected callback";
    fail();
  }
}

static void onClick(const ScriptedUIElems::Button& b, const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
    Rectangle bounds, Vec2 pos, EventCallback& callback) {
  if (id == MouseButtonId::LEFT && (pos.inRectangle(bounds) == !b.reverse))
    performAction(data, context, callback);
}

struct SubElemInfo {
  const ScriptedUI& elem;
  const ScriptedUIData& data;
  Rectangle bounds;
};

namespace {
struct KeyReader {
  SDL::SDL_Keycode SERIAL(key);
  void serialize(PrettyInputArchive& ar, const unsigned int v) {
    string key = ar.eat();
    if (auto k = keycodeFromString(key.data()))
      this->key = *k;
    else
      ar.error("Unknown key value: \"" + key + "\"");
  }
};
}

void ScriptedUIElems::KeyHandler::serialize(PrettyInputArchive& ar, const unsigned int v) {
  KeyReader key;
  ar(roundBracket(), NAMED(key));
  ar(endInput());
  this->key = key.key;
}

static void onKeypressed(const ScriptedUIElems::KeyHandler& handler, const ScriptedUIData& data, ScriptedContext& context,
    SDL::SDL_Keysym sym, EventCallback& callback) {
  if (handler.key == sym.sym)
    performAction(data, context, callback);
}

template <typename T, REQUIRE(getElemBounds(TVALUE(const T&), TVALUE(const ScriptedUIData&), TVALUE(ScriptedContext&),
  TVALUE(Rectangle)))>
static void render(const T& elem, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  for (auto& subElem : getElemBounds(elem, data, context, area))
    subElem.elem.render(subElem.data, context, subElem.bounds);
}

template <typename T, REQUIRE(getElemBounds(TVALUE(const T&), TVALUE(const ScriptedUIData&), TVALUE(ScriptedContext&),
  TVALUE(Rectangle)))>
static void onClick(const T& elem, const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id, Rectangle area,
    Vec2 pos, EventCallback& callback) {
  for (auto& subElem : getElemBounds(elem, data, context, area))
    subElem.elem.onClick(subElem.data, context, id, subElem.bounds, pos, callback);
}

template <typename T, REQUIRE(getElemBounds(TVALUE(const T&), TVALUE(const ScriptedUIData&), TVALUE(ScriptedContext&),
  TVALUE(Rectangle)))>
static void onKeypressed(const T& elem, const ScriptedUIData& data, ScriptedContext& context, SDL::SDL_Keysym sym,
    EventCallback& callback) {
  auto elems = getElemBounds(elem, data, context, Rectangle(0, 0, 10000, 10000));
  for (auto& subElem : getElemBounds(elem, data, context, Rectangle(0, 0, 10000, 10000)))
    subElem.elem.onKeypressed(subElem.data, context, sym, callback);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::MarginsImpl& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return {SubElemInfo{*f.inside, data, area.minusMargin(f.width)}};
}

static Vec2 getSize(const ScriptedUIElems::MarginsImpl& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.inside->getSize(data, context) + Vec2(f.width, f.width) * 2;
}

string getText(const ScriptedUIElems::Label& f, const ScriptedUIData& data) {
  if (f.text)
    return *f.text;
  if (auto label = data.getReferenceMaybe<ScriptedUIDataElems::Label>())
    return label->data();
  else
    return "not a label";
}

static void render(const ScriptedUIElems::Label& f, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  context.renderer->drawText(f.color, area.topLeft(), getText(f, data), Renderer::CenterType::NONE,
    f.size.value_or(Renderer::textSize()));
}

static Vec2 getSize(const ScriptedUIElems::Label& f, const ScriptedUIData& data, ScriptedContext& context) {
  return Vec2(context.renderer->getTextLength(getText(f, data), f.size.value_or(19)), 20);
}

static void render(const ScriptedUIElems::Fill& f, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  context.renderer->drawFilledRectangle(area, f);
}

static void render(const ScriptedUIElems::Frame& f, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  context.renderer->drawFilledRectangle(area, Color::TRANSPARENT, f.color);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Position& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  auto size = f.elem->getSize(data, context);
  using namespace ScriptedUIElems;
  switch (f.position) {
    case PlacementPos::MIDDLE:
      return {SubElemInfo{*f.elem, data, Rectangle(area.middle() - size / 2, area.middle() + size / 2)}};
    case PlacementPos::TOP_STRETCHED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.topLeft(), area.topRight() + Vec2(0, size.y))}};
    case PlacementPos::BOTTOM_STRETCHED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.bottomLeft() - Vec2(0, size.y), area.bottomRight())}};
    case PlacementPos::LEFT_STRETCHED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.topLeft(), area.bottomLeft() + Vec2(size.x, 0))}};
    case PlacementPos::RIGHT_STRETCHED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.topRight() - Vec2(size.x, 0), area.bottomRight())}};
    case PlacementPos::TOP_LEFT:
      return {SubElemInfo{*f.elem, data, Rectangle(area.topLeft(), area.topLeft() + size)}};
    case PlacementPos::TOP_RIGHT:
      return {SubElemInfo{*f.elem, data, Rectangle(area.topRight() - Vec2(size.x, 0), area.topRight() + Vec2(0, size.y))}};
    case PlacementPos::BOTTOM_RIGHT:
      return {SubElemInfo{*f.elem, data, Rectangle(area.bottomRight() - size, area.bottomRight())}};
    case PlacementPos::BOTTOM_LEFT:
      return {SubElemInfo{*f.elem, data, Rectangle(area.bottomLeft() - Vec2(0, size.y), area.bottomLeft() + Vec2(size.x, 0))}};
    case PlacementPos::TOP_CENTERED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.middle().x - size.x / 2, area.top(),
          area.middle().x - size.x / 2 + size.x, area.top() + size.y)}};
    case PlacementPos::BOTTOM_CENTERED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.middle().x - size.x / 2, area.bottom() - size.y,
          area.middle().x - size.x / 2 + size.x, area.bottom())}};
    case PlacementPos::LEFT_CENTERED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.left(), area.middle().y - size.y / 2,
          area.left() + size.x, area.middle().y - size.y / 2 + size.y)}};
    case PlacementPos::RIGHT_CENTERED:
      return {SubElemInfo{*f.elem, data, Rectangle(area.right() - size.x, area.middle().y - size.y / 2,
          area.right(), area.middle().y - size.y / 2 + size.y)}};
  }
}

static Vec2 getSize(const ScriptedUIElems::Position& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.elem->getSize(data, context);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Chain& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return f.elems.transform([&](auto& elem) { return SubElemInfo{elem, data, area}; });
}

static Vec2 getSize(const ScriptedUIElems::Chain& f, const ScriptedUIData& data, ScriptedContext& context) {
  Vec2 res;
  for (auto& elem : f.elems) {
    auto size = elem.getSize(data, context);
    res.x = max(res.x, size.x);
    res.y = max(res.y, size.y);
  }
  return res;
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Width& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return {SubElemInfo{*f.elem, data, area}};
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Height& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return {SubElemInfo{*f.elem, data, area}};
}

static Vec2 getSize(const ScriptedUIElems::Height& f, const ScriptedUIData& data, ScriptedContext& context) {
  return Vec2(f.elem->getSize(data, context).x, f.value);
}

static Vec2 getSize(const ScriptedUIElems::Width& f, const ScriptedUIData& data, ScriptedContext& context) {
  return Vec2(f.value, f.elem->getSize(data, context).y);
}

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

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Horizontal& f, const ScriptedUIData& data,
    ScriptedContext& context, Rectangle area) {
  auto ranges = getStaticListBounds(area.getXRange(),
      f.elems.transform([&](auto& elem) { return elem.getSize(data, context).x; }), f.stretchedElem);
  vector<SubElemInfo> ret;
  ret.reserve(ranges.size());
  for (int i : All(ranges))
    ret.push_back(SubElemInfo{f.elems[i], data, Rectangle(ranges[i].getStart(), area.top(), ranges[i].getEnd(), area.bottom())});
  return ret;
}

static Vec2 getSize(const ScriptedUIElems::Horizontal& f, const ScriptedUIData& data, ScriptedContext& context) {
  Vec2 res;
  for (auto& elem : f.elems) {
    auto size = elem.getSize(data, context);
    res.x += size.x;
    res.y = max(res.y, size.y);
  }
  return res;
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Vertical& f, const ScriptedUIData& data,
    ScriptedContext& context, Rectangle area) {
  auto ranges = getStaticListBounds(area.getYRange(),
      f.elems.transform([&](auto& elem) { return elem.getSize(data, context).y; }), f.stretchedElem);
  vector<SubElemInfo> ret;
  ret.reserve(ranges.size());
  for (int i : All(ranges)) {
    ret.push_back(SubElemInfo{f.elems[i], data,
        Rectangle(area.left(), ranges[i].getStart(), area.right(), ranges[i].getEnd())});
  }
  return ret;
}

static Vec2 getSize(const ScriptedUIElems::Vertical& f, const ScriptedUIData& data, ScriptedContext& context) {
  Vec2 res;
  for (auto& elem : f.elems) {
    auto size = elem.getSize(data, context);
    res.x = max(res.x, size.x);
    res.y += size.y;
  }
  return res;
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

void ScriptedUIElems::Vertical::serialize(PrettyInputArchive& ar1, const unsigned int v) {
  serializeVerticalHorizontal(*this, ar1, v);
}

void ScriptedUIElems::Horizontal::serialize(PrettyInputArchive& ar1, const unsigned int v) {
  serializeVerticalHorizontal(*this, ar1, v);
}

static vector<SubElemInfo> getError(const string& s) {
  static map<string, ScriptedUI> errors;
  if (!errors.count(s))
    errors.insert(make_pair(s, ScriptedUIElems::Label{s}));
  return {SubElemInfo{errors.at(s), ScriptedUIData{}, Rectangle()}};
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Using& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  if (f.key == "EXIT")
    return {SubElemInfo{*f.elem, context.state.exit, area}};
  else
  if (f.key == "HIGHLIGHT_NEXT")
    return {SubElemInfo{*f.elem, context.state.highlightNext, area}};
  else
  if (f.key == "HIGHLIGHT_PREVIOUS")
    return {SubElemInfo{*f.elem, context.state.highlightPrevious, area}};
  else
  if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
    if (auto value = getReferenceMaybe(record->elems, f.key))
      return {SubElemInfo{*f.elem, *value, area}};
    else
      return {};
  } else
    return getError("not a record");
}

static Vec2 getSize(const ScriptedUIElems::Using& f, const ScriptedUIData& data, ScriptedContext& context) {
  if (isOneOf(f.key, "EXIT", "HIGHLIGHT_NEXT", "HIGHLIGHT_PREVIOUS")) {
    static ScriptedUIData fun = ScriptedUIDataElems::Callback{[] { return true; }};
    return f.elem->getSize(fun, context);
  } else
  if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
    if (auto value = getReferenceMaybe(record->elems, f.key))
      return f.elem->getSize(*value, context);
    else
      return Vec2(0, 0);
  }
  return Vec2(100, 20);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::If& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
    if (record->elems.count(f.key))
      return {SubElemInfo{*f.elem, data, area}};
    else
      return {};
  } else
    return getError("not a record");
}

static Vec2 getSize(const ScriptedUIElems::If& f, const ScriptedUIData& data, ScriptedContext& context) {
  if (auto record = data.getReferenceMaybe<ScriptedUIDataElems::Record>()) {
    if (record->elems.count(f.key))
      return f.elem->getSize(data, context);
    else
      return Vec2(0, 0);
  }
  return Vec2(100, 20);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Focusable& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return {SubElemInfo{*f.elem, data, area}};
}

static Vec2 getSize(const ScriptedUIElems::Focusable& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.elem->getSize(data, context);
}

static void render(const ScriptedUIElems::Focusable& f, const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) {
  if (context.state.highlightedElem == context.elemCounter)
    f.elem->render(data, context, bounds);
  ++context.elemCounter;
}

static void onClick(const ScriptedUIElems::Focusable& f, const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
  Rectangle bounds, Vec2 pos, EventCallback& callback) {
  if (id == MouseButtonId::MOVED) {
    if (pos.inRectangle(bounds))
      callback = [counter = context.elemCounter, &context] { context.state.highlightedElem = counter; return false; };
    else
    if (context.state.highlightedElem == context.elemCounter)
      callback = [&] { context.state.highlightedElem = none; return false;};
  }
  ++context.elemCounter;
}

static void onKeypressed(const ScriptedUIElems::Focusable& f, const ScriptedUIData& data, ScriptedContext& context,
    SDL::SDL_Keysym sym, EventCallback& callback) {
  if (sym.sym == SDL::SDLK_RETURN && context.elemCounter == context.state.highlightedElem)
    performAction(data, context, callback);
  ++context.elemCounter;
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::MouseOver& f, const ScriptedUIData& data, ScriptedContext context,
    Rectangle area) {
  return {SubElemInfo{*f.elem, data, area}};
}

static Vec2 getSize(const ScriptedUIElems::MouseOver& f, const ScriptedUIData& data, ScriptedContext context) {
  return f.elem->getSize(data, context);
}

static void render(const ScriptedUIElems::MouseOver& f, const ScriptedUIData& data, ScriptedContext context, Rectangle bounds) {
  if (context.renderer->getMousePos().inRectangle(bounds))
    f.elem->render(data, context, bounds);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::List& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  using namespace ScriptedUIElems;
  Vec2 startPos = area.topLeft();
  if (auto list = data.getReferenceMaybe<ScriptedUIDataElems::List>()) {
    vector<SubElemInfo> ret;
    ret.reserve(list->size());
    for (auto& elem : *list) {
      Rectangle thisArea;
      Vec2 nextPos;
      if (f.direction == Direction::HORIZONTAL) {
        auto width = f.elem->getSize(elem, context).x;
        thisArea = Rectangle(startPos, startPos + Vec2(width, area.height()));
        nextPos = startPos + Vec2(width, 0);
      } else {
        auto height = f.elem->getSize(elem, context).y;
        thisArea = Rectangle(startPos, startPos + Vec2(area.width(), height));
        nextPos = startPos + Vec2(0, height);
      }
      ret.push_back(SubElemInfo{*f.elem, elem, thisArea});
      startPos = nextPos;
    }
    return ret;
  } else
    return getError("not a list");
}

static Vec2 getSize(const ScriptedUIElems::List& f, const ScriptedUIData& data, ScriptedContext& context) {
  if (auto list = data.getReferenceMaybe<ScriptedUIDataElems::List>()) {
    Vec2 res;
    for (auto& elem : *list) {
      auto size = f.elem->getSize(elem, context);
      if (f.direction == ScriptedUIElems::Direction::VERTICAL) {
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

static Vec2 getSize(const ScriptedUIElems::Scrollable& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.elem->getSize(data, context);
}

static void scroll(double& scrollState, int dir) {
  scrollState += 0.1 * dir;
  scrollState = max(0.0, min(1.0, scrollState));
}

static Rectangle getContentBounds(Rectangle bounds, int offset, int scrollBarWidth, int elemHeight) {
  return Rectangle(bounds.left(), bounds.top() - offset,
        bounds.right() - scrollBarWidth, bounds.top() - offset + elemHeight);
}

static Rectangle getScrollBarBounds(Rectangle bounds, int offset, int scrollBarWidth) {
  return Rectangle(bounds.topRight() - Vec2(scrollBarWidth, 0), bounds.bottomRight());
}

template <typename ContentFun, typename ScrollbarFun>
static void processScroller(const ScriptedUIElems::Scrollable& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle bounds, ContentFun contentFun, ScrollbarFun scrollbarFun) {
  auto height = f.elem->getSize(data, context).y;
  if (height <= bounds.height())
    contentFun(bounds);
  int offset = (height - bounds.height()) * context.state.scrollPos;
  int scrollBarWidth = f.scrollbar->getSize(data, context).x;
  auto contentBounds = getContentBounds(bounds, offset, scrollBarWidth, height);
  context.renderer->setScissor(bounds);
  contentFun(contentBounds);
  context.renderer->setScissor(none);
  scrollbarFun(getScrollBarBounds(bounds, offset, scrollBarWidth));
}

static void render(const ScriptedUIElems::Scrollable& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle bounds) {
  processScroller(f, data, context, bounds,
      [&] (Rectangle bounds) { f.elem->render(data, context, bounds); },
      [&] (Rectangle bounds) { f.scrollbar->render(data, context, bounds); }      
  );
}

static void onClick(const ScriptedUIElems::Scrollable& f, const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
    Rectangle bounds, Vec2 pos, EventCallback& callback) {
  if (pos.inRectangle(bounds)) {
    if (id == MouseButtonId::WHEEL_DOWN)
      callback = [&] { scroll(context.state.scrollPos, 1); return false; };
    else
    if (id == MouseButtonId::WHEEL_UP)
      callback = [&] { scroll(context.state.scrollPos, -1); return false; };
  }
  processScroller(f, data, context, bounds,
      [&] (Rectangle bounds) { f.elem->onClick(data, context, id, bounds, pos, callback); },
      [&] (Rectangle bounds) { f.scrollbar->onClick(data, context, id, bounds, pos, callback); }
  );
}

static void onClick(const ScriptedUIElems::ScrollButton& f, const ScriptedUIData&, ScriptedContext& context, MouseButtonId id,
    Rectangle bounds, Vec2 pos, EventCallback& callback) {
  if (id == MouseButtonId::LEFT && pos.inRectangle(bounds))
    callback = [&] { scroll(context.state.scrollPos, f.direction); return false; };
}

static Rectangle getSliderPos(const ScriptedUIElems::Scroller& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle bounds) {
  auto height = f.slider->getSize(data, context).y;
  if (auto& held = context.state.scrollButtonHeld)
    context.state.scrollPos = max(0.0, min(1.0, 
        double(context.renderer->getMousePos().y - *held - bounds.top()) / (bounds.height() - height)));
  auto pos = bounds.top() + context.state.scrollPos * (bounds.height() - height);
  return Rectangle(bounds.left(), pos, bounds.right(), pos + height);
}

static void onClick(const ScriptedUIElems::Scroller& f, const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id,
    Rectangle bounds, Vec2 pos, EventCallback& callback) {
  if (id == MouseButtonId::LEFT && pos.inRectangle(bounds))
    callback = [&] {
      auto sliderHeight = f.slider->getSize(data, context).y;
      context.state.scrollPos = max(0.0, min(1.0, double(pos.y - bounds.top()) / (bounds.height() - sliderHeight)));
      auto sliderPos = getSliderPos(f, data, context, bounds);
      if (pos.inRectangle(sliderPos))
        context.state.scrollButtonHeld = pos.y - sliderPos.top();
      return false;
    };
  else if (id == MouseButtonId::RELEASED)
    callback = [&] { context.state.scrollButtonHeld = none; return false; };
}

static Vec2 getSize(const ScriptedUIElems::Scroller& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.slider->getSize(data, context);
}

static void render(const ScriptedUIElems::Scroller& f, const ScriptedUIData& data, ScriptedContext& context, Rectangle bounds) {
  f.slider->render(data, context, getSliderPos(f, data, context, bounds));
}

static void render(const ScriptedUIElems::Scissor& s, const ScriptedUIData& data, ScriptedContext& context, Rectangle area) {
  context.renderer->setScissor(area);
  s.elem->render(data, context, area);
  context.renderer->setScissor(none);
}

static Vec2 getSize(const ScriptedUIElems::Scissor& f, const ScriptedUIData& data, ScriptedContext& context) {
  return f.elem->getSize(data, context);
}

static vector<SubElemInfo> getElemBounds(const ScriptedUIElems::Scissor& f, const ScriptedUIData& data, ScriptedContext& context,
    Rectangle area) {
  return {SubElemInfo{*f.elem, data, area}};
}

void ScriptedUI::render(const ScriptedUIData& data, ScriptedContext& context, Rectangle area) const {
  visit<void>([&] (const auto& ui) { ::render(ui, data, context, area); } );
}

Vec2 ScriptedUI::getSize(const ScriptedUIData& data, ScriptedContext& context) const {
  return visit<Vec2>([&] (const auto& ui) { return ::getSize(ui, data, context); } );
}

void ScriptedUI::onClick(const ScriptedUIData& data, ScriptedContext& context, MouseButtonId id, Rectangle bounds,
    Vec2 pos, EventCallback& callback) const {
  visit<void>([&] (const auto& ui) { ::onClick(ui, data, context, id, bounds, pos, callback); });
}

void ScriptedUI::onKeypressed(const ScriptedUIData& data, ScriptedContext& context, SDL::SDL_Keysym sym,
    EventCallback& callback) const {
  visit<void>([&] (const auto& ui) { ::onKeypressed(ui, data, context, sym, callback); });
}