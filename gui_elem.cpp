/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "gui_elem.h"
#include "util.h"
#include "view_object.h"
#include "tile.h"
#include "clock.h"
#include "spell.h"
#include "options.h"
#include "scroll_position.h"
#include "keybinding_map.h"
#include "attr_type.h"
#include "scripted_ui.h"
#include "scripted_ui_data.h"
#include "mouse_button_id.h"
#include "tileset.h"
#include "steam_input.h"

#include "sdl.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

DEF_SHARED_PTR(VerticalList);

Rectangle GuiElem::getBounds() {
  return bounds;
}

void GuiElem::setBounds(Rectangle b) {
  if (bounds != b) {
    bounds = b;
    onRefreshBounds();
  }
}

void GuiElem::setPreferredBounds(Vec2 origin) {
  setBounds(Rectangle(origin, origin + Vec2(*getPreferredWidth(), *getPreferredHeight())));
}

static map<int, int> lineNumbers;
static int totalGuiElems = 0;

void dumpGuiLineNumbers(ostream& o) {
  o << "Total elems " << totalGuiElems << "\n";
  vector<pair<int, int>> v;
  for (auto& l : lineNumbers)
    v.push_back(l);
  sort(v.begin(), v.end(), [](auto& p1, auto& p2) { return p1.second > p2.second; });
  for (auto& l : v)
    std::cout << "Line " << l.first << ": " << l.second << "\n";
}

GuiElem::~GuiElem() {
  if (lineNumber)
    --lineNumbers[*lineNumber];
  --totalGuiElems;
}

GuiElem::GuiElem() {
  ++totalGuiElems;
}

void GuiElem::setLineNumber(int l) {
  lineNumber = l;
  ++lineNumbers[*lineNumber];
}

class ButtonElem : public GuiElem {
  public:
  ButtonElem(function<void(Rectangle, Vec2)> f, bool capture) : fun(f), capture(capture) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    if (b == MouseButtonId::LEFT) {
      auto bounds = getBounds();
      if (pos.inRectangle(bounds)) {
        fun(bounds, pos - bounds.topLeft());
        return capture;
      }
    }
    return false;
  }

  protected:
  function<void(Rectangle, Vec2)> fun;
  bool capture;
};

class ReleaseButton : public GuiElem {
  public:
  ReleaseButton(function<void()> f, int but) : fun(f), button(but) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    switch (b) {
      case MouseButtonId::RELEASED:
        if (clicked && pos.inRectangle(getBounds())) {
          fun();
        }
        clicked = false;
        return false;
      case MouseButtonId::LEFT:
        if (button == 0 && pos.inRectangle(getBounds())) {
          clicked = true;
          return true;
        } else
          return false;
      case MouseButtonId::RIGHT:
        if (button == 1 && pos.inRectangle(getBounds())) {
          clicked = true;
          return true;
        } else
          return false;
      default:
        return false;
    }
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    if (!pos.inRectangle(getBounds()))
      clicked = false;
    return false;
  }

  virtual void onMouseGone() override {
    clicked = false;
  }

  protected:
  function<void()> fun;
  const int button;
  bool clicked = false;
};

bool GuiFactory::isShift(const SDL_Keysym& key) {
  return key.mod & (SDL::KMOD_LSHIFT | SDL::KMOD_RSHIFT);
}

bool GuiFactory::isCtrl(const SDL_Keysym& key) {
  return key.mod & (SDL::KMOD_LCTRL | SDL::KMOD_RCTRL);
}

bool GuiFactory::isAlt(const SDL_Keysym& key) {
  return key.mod & (SDL::KMOD_LALT | SDL::KMOD_RALT);
}

bool GuiFactory::keyEventEqual(const SDL_Keysym& k1, const SDL_Keysym& k2) {
  return k1.sym == k2.sym && isShift(k1) == isShift(k2) && isCtrl(k1) == isCtrl(k2) && isAlt(k1) == isAlt(k2);
}

SDL_Keysym GuiFactory::getKey(SDL_Keycode code) {
  SDL_Keysym ret {};
  ret.sym = code;
  return ret;
}

class ButtonKey : public ButtonElem {
  public:
  ButtonKey(function<void(Rectangle)> f, SDL_Keysym key, bool cap) : ButtonElem([f](Rectangle b, Vec2) { f(b);}, cap),
      hotkey(key), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (GuiFactory::keyEventEqual(hotkey, key)) {
      fun(getBounds(), Vec2(0, 0));
      return capture;
    }
    return false;
  }

  private:
  SDL_Keysym hotkey;
  bool capture;
};

GuiFactory::GuiFactory(Renderer& r, Clock* c, Options* o, const DirectoryPath& freeImages)
    : clock(c), renderer(r), options(o), imagesPath(freeImages) {
}

GuiFactory::~GuiFactory() {}

DragContainer& GuiFactory::getDragContainer() {
  return dragContainer;
}

SGuiElem GuiFactory::button(function<void()> fun, SDL_Keysym hotkey, bool capture) {
  return SGuiElem(new ButtonKey([=](Rectangle) { fun(); }, hotkey, capture));
}

SGuiElem GuiFactory::buttonRect(function<void(Rectangle)> fun) {
  return SGuiElem(new ButtonElem([=](Rectangle b, Vec2) {fun(b);}, false));
}

SGuiElem GuiFactory::button(function<void()> fun, bool capture) {
  return SGuiElem(new ButtonElem([=](Rectangle, Vec2) { fun(); }, capture));
}

namespace  {
class TextFieldElem : public GuiElem {
  public:
  TextFieldElem(function<string()> text, function<void(string)> callback, int maxLength, Clock* clock, bool alwaysFocused)
      : callback(std::move(callback)), getText(std::move(text)), clock(clock), maxLength(maxLength),
        alwaysFocused(alwaysFocused) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    if (b == MouseButtonId::LEFT) {
      focused = pos.inRectangle(getBounds());
      return focused;
    }
    return false;
  }

  bool isFocused() const {
    return alwaysFocused || focused;
  }

  virtual void render(Renderer& r) override {
    auto bounds = getBounds();
    auto rectBounds = bounds;
    auto toDraw = getText();
    if (isFocused())
      rectBounds = Rectangle(rectBounds.topLeft(),
          Vec2(max(rectBounds.right(), rectBounds.left() + r.getTextLength(toDraw) + 10), rectBounds.bottom()));
    if (isFocused() && (clock->getRealMillis().count() / 400) % 2 == 0)
      toDraw += "|";
    if (isFocused() && !alwaysFocused)
      r.setTopLayer();
    r.drawFilledRectangle(rectBounds, Color::BLACK, isFocused() ? Color::WHITE : Color::GRAY);
    if (!isFocused())
      r.setScissor(bounds.minusMargin(1));
    r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(6, 6), toDraw);
    r.drawText(Color::WHITE, bounds.topLeft() + Vec2(5, 4), toDraw);
    if (!isFocused())
      r.setScissor(none);
    if (isFocused() && !alwaysFocused)
      r.popLayer();
  }

  virtual bool onKeyPressed2(SDL::SDL_Keysym sym) override {
    if (isFocused()) {
      auto current = getText();
      switch (sym.sym) {
        case SDL::SDLK_BACKSPACE: {
          if (!current.empty())
            current.pop_back();
          callback(current);
          break;
        }
        case SDL::SDLK_ESCAPE:
        case SDL::SDLK_KP_ENTER:
        case SDL::SDLK_RETURN:
          callback(current);
          focused = false;
          return true;
        default:
          break;
      }
      return true;
    }
    return false;
  }

  virtual bool onTextInput(const char* s) override {
    if (isFocused()) {
      auto current = getText();
      current += s;
      current = current.substr(0, maxLength);
      callback(current);
      return true;
    }
    return false;
  }

  private:
  function<void(string)> callback;
  function<string()> getText;
  Clock* clock;
  int maxLength;
  bool focused = false;
  bool alwaysFocused = false;
};
}

SGuiElem GuiFactory::textField(int maxLength, function<string()> text, function<void(string)> callback) {
  return topMargin(-4, bottomMargin(4,
      make_shared<TextFieldElem>(std::move(text), std::move(callback), maxLength, clock, false)));
}

SGuiElem GuiFactory::textFieldFocused(int maxLength, function<string()> text, function<void(string)> callback) {
  return topMargin(-4, bottomMargin(4,
      make_shared<TextFieldElem>(std::move(text), std::move(callback), maxLength, clock, true)));
}

SGuiElem GuiFactory::buttonPos(function<void (Rectangle, Vec2)> fun) {
  return make_shared<ButtonElem>(fun, false);
}

namespace {
class ButtonRightClick : public GuiElem {
  public:
  ButtonRightClick(function<void(Rectangle)> f) : fun(f) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    if (b == MouseButtonId::RIGHT && pos.inRectangle(getBounds())) {
      fun(getBounds());
      return true;
    }
    return false;
  }

  protected:
  function<void(Rectangle)> fun;
};
}

SGuiElem GuiFactory::buttonRightClick(function<void ()> fun) {
  return SGuiElem(new ButtonRightClick([fun](Rectangle) { fun(); }));
}

SGuiElem GuiFactory::releaseLeftButton(function<void()> fun, optional<Keybinding> key) {
  auto ret = SGuiElem(new ReleaseButton(fun, 0));
  if (key)
    ret = stack(std::move(ret), keyHandler(fun, *key, false));
  return ret;
}

SGuiElem GuiFactory::releaseRightButton(function<void()> fun) {
  return SGuiElem(new ReleaseButton(fun, 1));
}

class ReverseButton : public GuiElem {
  public:
  ReverseButton(function<void()> f, bool cap) : fun(f), capture(cap) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    if (b == MouseButtonId::LEFT && !pos.inRectangle(getBounds())) {
      fun();
      return capture;
    }
    return false;
  }

  protected:
  function<void()> fun;
  bool capture;
};

SGuiElem GuiFactory::reverseButton(function<void()> fun, vector<SDL_Keysym> hotkey, bool capture) {
  return stack(
      keyHandler(fun, hotkey, true),
      SGuiElem(new ReverseButton(fun, capture)));
}


class MouseWheel : public GuiElem {
  public:
  MouseWheel(function<void(bool)> f) : fun(f) {}

  virtual bool onClick(MouseButtonId id, Vec2 mousePos) override {
    if (isOneOf(id, MouseButtonId::WHEEL_DOWN, MouseButtonId::WHEEL_UP) && mousePos.inRectangle(getBounds())) {
      fun(id == MouseButtonId::WHEEL_UP);
      return true;
    }
    return false;
  }

  private:
  function<void(bool)> fun;
};

SGuiElem GuiFactory::mouseWheel(function<void(bool)> fun) {
  return SGuiElem(new MouseWheel(fun));
}

class StopMouseMovement : public GuiElem {
  public:
  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    return pos.inRectangle(getBounds());
  }

  virtual bool onClick(MouseButtonId, Vec2 pos) override {
    return pos.inRectangle(getBounds());
  }
};

SGuiElem GuiFactory::stopMouseMovement() {
  return SGuiElem(new StopMouseMovement());
}

class DrawCustom : public GuiElem {
  public:
  DrawCustom(GuiFactory::CustomDrawFun draw, optional<int> width = none)
      : drawFun(draw), preferredWidth(width) {}

  DrawCustom(GuiFactory::CustomDrawFun draw, Vec2 preferredSize)
      : drawFun(draw), preferredWidth(preferredSize.x), preferredHeight(preferredSize.y) {}

  virtual void render(Renderer& renderer) override {
    drawFun(renderer, getBounds());
  }

  virtual optional<int> getPreferredWidth() override {
    return preferredWidth;
  }

  virtual optional<int> getPreferredHeight() override {
    return preferredHeight;
  }

  private:
  GuiFactory::CustomDrawFun drawFun;
  optional<int> preferredWidth;
  optional<int> preferredHeight;
};

SGuiElem GuiFactory::drawCustom(CustomDrawFun fun) {
  return make_shared<DrawCustom>(fun);
}

SGuiElem GuiFactory::rectangle(Color color, optional<Color> borderColor) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawFilledRectangle(bounds, color, borderColor);
        }));
}

SGuiElem GuiFactory::repeatedPattern(Texture& tex) {
  return SGuiElem(new DrawCustom(
        [&tex] (Renderer& r, Rectangle bounds) {
          r.drawSprite(bounds.topLeft(), Vec2(0, 0), Vec2(bounds.width(), bounds.height()), tex);
        }));
}

SGuiElem GuiFactory::sprite(Texture& tex, double height) {
  return SGuiElem(new DrawCustom(
        [&tex, height] (Renderer& r, Rectangle bounds) {
          Vec2 size = tex.getSize();
          r.drawSprite(bounds.topLeft(), Vec2(0, 0), size, tex,
              Vec2(height * size.x / size.y, height));
        }));
}

class DrawScripted : public GuiElem {
  public:
  DrawScripted(ScriptedContext context, ScriptedUIId id, const ScriptedUIData& data)
      : id(id), data(data), context(context) {}

  const ScriptedUI& get() {
    if (auto ret = getReferenceMaybe(context.renderer->getTileSet().scriptedUI, id))
      return *ret;
    else
      return context.renderer->getTileSet().scriptedUI.at("notfound");
  }

  virtual void render(Renderer& renderer) override {
    context.elemCounter = 0;
    context.tooltipCounter = 0;
    context.sliderCounter = 0;
    get()->render(data, context, getBounds());
    if (auto& elem = context.state.highlightedElem) {
      if (context.elemCounter == 0)
        elem = none;
      else
        *elem = (*elem % context.elemCounter + context.elemCounter) % context.elemCounter;
    }
  }

  bool handleCallback(const EventCallback& callback) {
    if (callback) {
      if (callback())
        context.endCallback();
      return true;
    }
    return false;
  }

  virtual bool onClick(MouseButtonId button, Vec2 pos) override {
    context.elemCounter = 0;
    context.sliderCounter = 0;
    context.tooltipCounter = 0;
    bool ret = true;
    EventCallback callback = [&ret] { ret = false; return false; };
    get()->onClick(data, context, button, getBounds(), pos, callback);
    if (callback())
      context.endCallback();
    return ret;
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    context.elemCounter = 0;
    context.sliderCounter = 0;
    context.tooltipCounter = 0;
    bool ret = true;
    EventCallback callback = [&ret] { ret = false; return false; };
    get()->onClick(data, context, MouseButtonId::MOVED, getBounds(), pos, callback);
    if (callback())
      context.endCallback();
    return ret;
  }

  //virtual void onMouseGone() {}
  //virtual void onRefreshBounds() {}
  //virtual void renderPart(Renderer& r, Rectangle) { render(r); }
  virtual bool onKeyPressed2(SDL::SDL_Keysym sym) override {
    context.elemCounter = 0;
    context.sliderCounter = 0;
    context.tooltipCounter = 0;
    bool ret = true;
    EventCallback callback = [&ret] { ret = false; return false; };
    get()->onKeypressed(data, context, sym, getBounds(), callback);
    if (callback())
      context.endCallback();
    return ret;
  }
  //virtual bool onTextInput(const char*) { return false; }
  virtual optional<int> getPreferredWidth() override {
    return get()->getSize(data, context).x;
  }
  virtual optional<int> getPreferredHeight() override {
    return get()->getSize(data, context).y;
  }

  ScriptedUIId id;
  const ScriptedUIData& data;
  ScriptedContext context;
};

SGuiElem GuiFactory::scripted(function<void()> endCallback, ScriptedUIId id, const ScriptedUIData& data, ScriptedUIState& state) {
  return SGuiElem(new DrawScripted(ScriptedContext{&renderer, this, endCallback, state, 0, 0}, id, std::move(data)));
}

SGuiElem GuiFactory::sprite(Texture& tex, Alignment align, bool vFlip, bool hFlip, Vec2 offset,
    optional<Color> col) {
  if (!tex.getTexId())
    return empty();
  return SGuiElem(new DrawCustom(
        [&tex, align, offset, col, vFlip, hFlip] (Renderer& r, Rectangle bounds) {
          Vec2 size = tex.getSize();
          optional<Vec2> stretchSize;
          Vec2 origin;
          Vec2 pos;
          switch (align) {
            case Alignment::TOP:
              pos = bounds.topLeft() + offset;
              size = Vec2(bounds.width() - 2 * offset.x, size.y);
              break;
            case Alignment::BOTTOM:
              pos = bounds.bottomLeft() + Vec2(0, -size.y) + offset;
              size = Vec2(bounds.width() - 2 * offset.x, size.y);
              break;
            case Alignment::RIGHT:
              pos = bounds.topRight() + Vec2(-size.x, 0) + offset;
              size = Vec2(size.x, bounds.height() - 2 * offset.y);
              break;
            case Alignment::LEFT:
              pos = bounds.topLeft() + offset;
              size = Vec2(size.x, bounds.height() - 2 * offset.y);
              break;
            case Alignment::TOP_LEFT:
              pos = bounds.topLeft() + offset;
              break;
            case Alignment::TOP_RIGHT:
              pos = bounds.topRight() - Vec2(size.x, 0) + offset;
              break;
            case Alignment::BOTTOM_RIGHT:
              pos = bounds.bottomRight() - size + offset;
              break;
            case Alignment::BOTTOM_LEFT:
              pos = bounds.bottomLeft() - Vec2(0, size.y) + offset;
              break;
            case Alignment::CENTER:
              pos = bounds.middle() - Vec2(size.x / 2, size.y / 2) + offset;
              break;
            case Alignment::TOP_CENTER:
              pos = (bounds.topLeft() + bounds.topRight()) / 2 - Vec2(size.x / 2, 0) + offset;
              break;
            case Alignment::LEFT_CENTER:
              pos = (bounds.topLeft() + bounds.bottomLeft()) / 2 - Vec2(0, size.y / 2) + offset;
              break;
            case Alignment::BOTTOM_CENTER:
              pos = (bounds.bottomLeft() + bounds.bottomRight()) / 2 - Vec2(size.x / 2, size.y) + offset;
              break;
            case Alignment::RIGHT_CENTER:
              pos = (bounds.topRight() + bounds.bottomRight()) / 2 - Vec2(size.x, size.y / 2) + offset;
              break;
            case Alignment::VERTICAL_CENTER:
              pos = (bounds.topLeft() + bounds.topRight()) / 2 - Vec2(size.x / 2, 0) + offset;
              size = Vec2(size.x, bounds.height() - 2 * offset.y);
              break;
            case Alignment::LEFT_STRETCHED:
              stretchSize = size * (double(bounds.height()) / size.y);
              pos = (bounds.topLeft() + bounds.bottomLeft()) / 2 - Vec2(0, stretchSize->y / 2) + offset;
              break;
            case Alignment::RIGHT_STRETCHED:
              stretchSize = size * (double(bounds.height()) / size.y);
              pos = (bounds.topRight() + bounds.bottomRight()) / 2
                  - Vec2(stretchSize->x, stretchSize->y / 2) + offset;
              break;
            case Alignment::CENTER_STRETCHED:
              stretchSize = size * (double(bounds.height()) / size.y);
              pos = (bounds.topRight() + bounds.topLeft()) / 2 - Vec2(stretchSize->x / 2, 0) + offset;
          }
          r.drawSprite(pos, origin, size, tex, stretchSize, !!col ? *col : Color::WHITE,
              Renderer::SpriteOrientation(vFlip, hFlip));
        }));
}

SGuiElem GuiFactory::label(const string& s, Color c, char hotkey) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          //r.setScissor(bounds);
          r.drawTextWithHotkey(Color::BLACK.transparency(100),
            bounds.topLeft() + Vec2(1, 2), s, 0);
          r.drawTextWithHotkey(c, bounds.topLeft(), s, hotkey);
          //r.setScissor(none);
        }, renderer.getTextSize(s)));
}

static vector<string> breakWord(Renderer& renderer, string word, int maxWidth, int size) {
  vector<string> ret;
  while (!word.empty()) {
    int maxSubstr = 0;
    for (int i : Range(word.size()))
      if (renderer.getTextLength(word.substr(0, i + 1), size) <= maxWidth)
        maxSubstr = i + 1;
    CHECK(maxSubstr > 0) << "Couldn't fit single character in line " << word << " line width " << maxWidth;
    ret.push_back(word.substr(0, maxSubstr));
    word = word.substr(maxSubstr);
  }
  return ret;
}

static vector<string> breakText(Renderer& renderer, const string& text, int maxWidth, int size = Renderer::textSize(),
    char delim = ' ') {
  if (text.empty())
    return {""};
  vector<string> rows;
  for (string line : split(text, {'\n'})) {
    rows.push_back("");
    for (string word : splitIncludeDelim(line, {delim})) {
      for (string subword : breakWord(renderer, word, maxWidth, size))
        if (rows.back().empty() || renderer.getTextLength(rows.back() + subword, size) <= maxWidth)
          rows.back() += subword;
        else
          rows.push_back(subword);
      //trim(rows.back());
    }
  }
  return rows;
}

vector<string> GuiFactory::breakText(const string& text, int maxWidth, int fontSize) {
  return ::breakText(renderer, text, maxWidth, fontSize);
}

class VariableLabel : public GuiElem {
  public:
  VariableLabel(function<string()> t, int line, int sz, Color c) : text(t), size(sz), color(c), lineHeight(line) {
  }

  virtual void render(Renderer& renderer) override {
    vector<string> lines = breakText(renderer, text(), getBounds().width(), size);
    int height = getBounds().top();
    for (int i : All(lines)) {
      renderer.drawText(color, Vec2(getBounds().left(), height), lines[i],
          Renderer::NONE, size);
      if (!lines[i].empty())
        height += lineHeight;
      else
        height += lineHeight / 3;
    }
  }

  private:
  function<string()> text;
  int size;
  Color color;
  int lineHeight;
};

SGuiElem GuiFactory::labelMultiLine(const string& s, int lineHeight, int size, Color c) {
  return SGuiElem(new VariableLabel([=]{ return s;}, lineHeight, size, c));
}

SGuiElem GuiFactory::labelMultiLineWidth(const string& s, int lineHeight, int width, int size, Color c, char delim) {
  auto lines = getListBuilder(lineHeight);
  for (auto& line : ::breakText(renderer, s, width, size, delim))
    lines.addElem(label(line, size, c));
  return lines.buildVerticalList();
}

static void lighten(Color& c) {
  if (3 * 255 - c.r - c.g - c.b < 75)
    c = Color::YELLOW;
  int a = 160;
  int b = 200;
  auto fun = [=] (int x) { return x * (255 - a) / b + a;};
  c.r = min(255, fun(c.r));
  c.g = min(255, fun(c.g));
  c.b = min(255, fun(c.b));
}

SGuiElem GuiFactory::labelHighlight(const string& s, Color c, char hotkey) {
  auto highlighted = c;
  lighten(highlighted);
  return mouseHighlight2(label(s, highlighted, hotkey), label(s, c, hotkey));
}

SGuiElem GuiFactory::buttonLabel(const string& s, function<void()> f, bool matchTextWidth, bool centerHorizontally, bool unicode) {
  return buttonLabel(s, button(std::move(f)), matchTextWidth, centerHorizontally, unicode);
}

SGuiElem GuiFactory::buttonLabelBlink(const string& s, function<void()> f) {
  return standardButtonBlink(label(s), button(std::move(f)), true);
}

SGuiElem GuiFactory::buttonLabel(const string& s, SGuiElem button, bool matchTextWidth, bool centerHorizontally, bool unicode) {
  auto text = unicode ? labelUnicode(s) : label(s);
  if (centerHorizontally)
    text = centerHoriz(std::move(text));
  return standardButton(std::move(text), std::move(button), matchTextWidth);
}

SGuiElem GuiFactory::standardButton(SGuiElem content, SGuiElem button, bool matchTextWidth) {
  auto ret = margins(stack(
        mouseHighlight2(standardButtonHighlight(), standardButton()),
        std::move(button)),
      -7, -5, -7, 3);
  if (matchTextWidth)
    ret = setWidth(*content->getPreferredWidth() + 1, std::move(ret));
  return stack(ret, std::move(content));
}

SGuiElem GuiFactory::standardButtonBlink(SGuiElem content, SGuiElem button, bool matchTextWidth) {
  auto ret = margins(stack(
        mouseHighlight2(standardButtonHighlight(), blink(standardButtonHighlight(), standardButton())),
        std::move(button)),
      -7, -5, -7, 3);
  if (matchTextWidth)
    ret = setWidth(*content->getPreferredWidth() + 1, std::move(ret));
  return stack(ret, std::move(content));
}

SGuiElem GuiFactory::buttonLabelWithMargin(const string& s, bool matchTextWidth) {
  auto ret = mouseHighlight2(
      stack(
          standardButtonHighlight(),
          centerVert(centerHoriz(margins(label(s, Color::WHITE), 0, 0, 0, 5)))),
      stack(
          standardButton(),
          centerVert(centerHoriz(margins(label(s, Color::WHITE), 0, 0, 0, 5))))
  );
  if (matchTextWidth)
    ret = setWidth(renderer.getTextLength(s) + 1, std::move(ret));
  return ret;
}

SGuiElem GuiFactory::buttonLabelSelected(const string& s, function<void()> f, bool matchTextWidth, bool centerHorizontally) {
  auto text = label(s, Color::WHITE);
  if (centerHorizontally)
    text = centerHoriz(text);
  auto ret = stack(margins(stack(standardButtonHighlight(), button(std::move(f))), -7, -5, -7, 3),
      std::move(text));
  if (matchTextWidth)
    ret = setWidth(renderer.getTextLength(s) + 1, std::move(ret));
  return ret;
}

SGuiElem GuiFactory::buttonLabelInactive(const string& s, bool matchTextWidth) {
  auto ret = stack(
      margins(standardButton(), -7, -5, -7, 3),
      label(s, Color::GRAY));
  if (matchTextWidth)
    ret = setWidth(renderer.getTextLength(s) + 1, std::move(ret));
  return ret;
}

SGuiElem GuiFactory::standardButton() {
  return stack(makeVec(
      repeatedPattern(get(TexId::BUTTON_BG)),
      sprite(get(TexId::BUTTON_BOTTOM), Alignment::BOTTOM, false, false),
      margins(sprite(get(TexId::BUTTON_BOTTOM), Alignment::TOP, true, false), 1, 0, 1, 0),
      sprite(get(TexId::BUTTON_SIDE), Alignment::RIGHT, false, true),
      sprite(get(TexId::BUTTON_SIDE), Alignment::LEFT, false, false),
      sprite(get(TexId::BUTTON_CORNER), Alignment::BOTTOM_RIGHT, false, true),
      sprite(get(TexId::BUTTON_CORNER), Alignment::BOTTOM_LEFT, false, false),
      sprite(get(TexId::BUTTON_CORNER), Alignment::TOP_RIGHT, true, true),
      sprite(get(TexId::BUTTON_CORNER), Alignment::TOP_LEFT, true, false)));
}

SGuiElem GuiFactory::standardButtonHighlight() {
  return stack(makeVec(
      repeatedPattern(get(TexId::BUTTON_BG)),
      sprite(get(TexId::BUTTON_BOTTOM_HIGHLIGHT), Alignment::BOTTOM, false, false),
      margins(sprite(get(TexId::BUTTON_BOTTOM_HIGHLIGHT), Alignment::TOP, true, false), 1, 0, 1, 0),
      sprite(get(TexId::BUTTON_SIDE_HIGHLIGHT), Alignment::RIGHT, false, true),
      sprite(get(TexId::BUTTON_SIDE_HIGHLIGHT), Alignment::LEFT, false, false),
      sprite(get(TexId::BUTTON_CORNER_HIGHLIGHT), Alignment::BOTTOM_RIGHT, false, true),
      sprite(get(TexId::BUTTON_CORNER_HIGHLIGHT), Alignment::BOTTOM_LEFT, false, false),
      sprite(get(TexId::BUTTON_CORNER_HIGHLIGHT), Alignment::TOP_RIGHT, true, true),
      sprite(get(TexId::BUTTON_CORNER_HIGHLIGHT), Alignment::TOP_LEFT, true, false)));
}

static double blinkingState(milliseconds time, int numBlinks, int numCycle) {
  const milliseconds period {250};
  if ((time.count() / period.count()) % numCycle >= numBlinks)
    return 1.0;
  double s = (time.count() % period.count()) / (double) period.count();
  return (cos(s * 2 * 3.14159) + 1) / 2;
}

static Color blinkingColor(Color c1, Color c2, milliseconds time) {
  double c = blinkingState(time, 2, 10);
  return Color(c1.r * c + c2.r * (1 - c), c1.g * c + c2.g * (1 - c), c1.b * c + c2.b * (1 - c));
}

SGuiElem GuiFactory::labelHighlightBlink(const string& s, Color c1, Color c2, char hotkey) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = blinkingColor(c1, c2, clock->getRealMillis());
          r.drawTextWithHotkey(Color::BLACK.transparency(100),
            bounds.topLeft() + Vec2(1, 2), s, hotkey);
          Color c1(c);
          if (r.getMousePos().inRectangle(bounds))
            lighten(c1);
          r.drawTextWithHotkey(c1, bounds.topLeft(), s, hotkey);
        }, renderer.getTextSize(s)));
}

SGuiElem GuiFactory::label(const string& s, function<Color()> colorFun, char hotkey) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          auto color = colorFun();
          r.drawText(Color::BLACK.transparency(min<Uint8>(100, color.a)), bounds.topLeft() + Vec2(1, 2), s);
          r.drawTextWithHotkey(colorFun(), bounds.topLeft(), s, hotkey);
        }, renderer.getTextSize(s)));
}

SGuiElem GuiFactory::labelFun(function<string()> textFun, function<Color()> colorFun) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), textFun());
          r.drawText(colorFun(), bounds.topLeft(), textFun());
        }));
}

SGuiElem GuiFactory::labelFun(function<string()> textFun, Color color) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), textFun());
          r.drawText(color, bounds.topLeft(), textFun());
        }));
}

SGuiElem GuiFactory::label(const string& s, int size, Color c) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), s, Renderer::NONE, size);
          r.drawText(c, bounds.topLeft(), s, Renderer::NONE, size);
        }, renderer.getTextSize(s, size)));
}

static Vec2 getTextPos(Rectangle bounds, Renderer::CenterType center) {
  switch (center) {
    case Renderer::HOR:
      return Vec2(bounds.middle().x, bounds.topLeft().y);
    case Renderer::VER:
      return Vec2(bounds.topLeft().x, bounds.middle().y);
    case Renderer::HOR_VER:
      return bounds.middle();
    default:
      return bounds.topLeft();
  }
}

SGuiElem GuiFactory::centeredLabel(Renderer::CenterType center, const string& s, int size, Color c) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Vec2 pos = getTextPos(bounds, center);
          r.drawText(Color::BLACK.transparency(100), pos + Vec2(1, 2), s, center, size);
          r.drawText(c, pos, s, center, size);
        }, renderer.getTextSize(s, size)));
}

SGuiElem GuiFactory::centeredLabel(Renderer::CenterType center, const string& s, Color c) {
  return centeredLabel(center, s, Renderer::textSize(), c);
}

SGuiElem GuiFactory::variableLabel(function<string()> fun, int lineHeight, int size, Color color) {
  return SGuiElem(new VariableLabel(fun, lineHeight, size, color));
}

SGuiElem GuiFactory::labelUnicode(const string& s, Color color, int size, FontId fontId) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(fontId, size, color, bounds.topLeft(), s);
  }, renderer.getTextSize(s, size, fontId)));
}

SGuiElem GuiFactory::labelUnicodeHighlight(const string& s, Color color, int size, FontId fontId) {
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = color;
          if (r.getMousePos().inRectangle(bounds))
            lighten(c);
          r.drawText(fontId, size, c, bounds.topLeft(), s);
  }, renderer.getTextSize(s, size, fontId)));
}

SGuiElem GuiFactory::crossOutText(Color color) {
  return SGuiElem(new DrawCustom([=] (Renderer& r, Rectangle bounds) {
      Rectangle pos(bounds.left(), bounds.middle().y - 3, bounds.right(), bounds.middle().y - 1);
      r.drawFilledRectangle(pos.translate(Vec2(1, 2)), Color::BLACK.transparency(100));
      r.drawFilledRectangle(pos, color);
  }));
}

class MainMenuLabel : public GuiElem {
  public:
  MainMenuLabel(Renderer& r, const string& s, Color c, double vPad) : text(s), color(c), vPadding(vPad), renderer(r) {}

  int getSize() {
    return (0.9 - 2 * vPadding) * getBounds().height();
  }

  virtual void render(Renderer& renderer) override {
    double height = getBounds().top() + vPadding * getBounds().height();
    int size = getSize();
    renderer.drawText(color, Vec2(getBounds().middle().x, height - size / 11), text, Renderer::HOR, size);
  }

  virtual optional<int> getPreferredWidth() override {
    return renderer.getTextLength(text, getSize());
  }

  private:
  string text;
  Color color;
  double vPadding;
  Renderer& renderer;
};

SGuiElem GuiFactory::mainMenuLabel(const string& s, double vPadding, Color c) {
  return SGuiElem(new MainMenuLabel(renderer, s, c, vPadding));
}

class GuiLayout : public GuiElem {
  public:
  GuiLayout(vector<SGuiElem> e) : elems(std::move(e)) {}
  GuiLayout(SGuiElem e) { elems.push_back(std::move(e)); }

  virtual bool onTextInput(const char* c) override {
    for (int i : AllReverse(elems))
      if (isVisible(i) && elems[i]->onTextInput(c))
        return true;
    return false;
  }

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    // Check visibility in advance, as it can potentially change in onClick
    vector<int> visible;
    for (int i : AllReverse(elems))
      if (isVisible(i))
        visible.push_back(i);
    for (int i : visible)
      if (elems[i]->onClick(b, pos))
        return true;
    return false;
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    bool gone = false;
    for (int i : AllReverse(elems)) {
      if (!gone && isVisible(i)) {
        gone = elems[i]->onMouseMove(pos, rel);
      } else
        elems[i]->onMouseGone();
    }
    return gone;
  }

  virtual void onMouseGone() override {
    for (int i : AllReverse(elems))
      elems[i]->onMouseGone();
  }

  virtual void render(Renderer& r) override {
    onRefreshBounds();
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->render(r);
  }

  virtual void onRefreshBounds() override {
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->setBounds(getElemBounds(i));
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    for (int i : AllReverse(elems))
      if (elems[i]->onKeyPressed2(key))
        return true;
    return false;
  }

  virtual Rectangle getElemBounds(int num) = 0;

  virtual bool isVisible(int num) {
    return true;
  }

  protected:
  vector<SGuiElem> elems;
};

class GuiStack : public GuiLayout {
  public:
  using GuiLayout::GuiLayout;

  virtual Rectangle getElemBounds(int num) override {
    return getBounds();
  }

  virtual optional<int> getPreferredWidth() override {
    optional<int> ret;
    for (int i : All(elems))
      if (isVisible(i))
        if (auto width = elems[i]->getPreferredWidth())
          if (!ret || *ret < *width)
            ret = width;
    return ret;
  }

  virtual optional<int> getPreferredHeight() override {
    optional<int> ret;
    for (int i : All(elems))
      if (isVisible(i))
        if (auto height = elems[i]->getPreferredHeight())
          if (!ret || *ret < *height)
            ret = height;
    return ret;
  }

};

SGuiElem GuiFactory::stack(vector<SGuiElem> elems) {
  return SGuiElem(new GuiStack(std::move(elems)));
}

SGuiElem GuiFactory::stack(SGuiElem g1, SGuiElem g2) {
  return stack(makeVec(std::move(g1), std::move(g2)));
}

SGuiElem GuiFactory::stack(SGuiElem g1, SGuiElem g2, SGuiElem g3) {
  return stack(makeVec(std::move(g1), std::move(g2), std::move(g3)));
}

SGuiElem GuiFactory::stack(SGuiElem g1, SGuiElem g2, SGuiElem g3, SGuiElem g4) {
  return stack(makeVec(std::move(g1), std::move(g2), std::move(g3), std::move(g4)));
}

class External : public GuiElem {
  public:
  External(GuiElem* e) : elem(e) {}

  virtual void render(Renderer& r) override {
    elem->render(r);
  }

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    return elem->onClick(b, v);
  }

  virtual bool onMouseMove(Vec2 v, Vec2 rel) override {
    return elem->onMouseMove(v, rel);
  }

  virtual void onMouseGone() override {
    elem->onMouseGone();
  }

  virtual void onRefreshBounds() override {
    elem->setBounds(getBounds());
  }

  virtual bool onKeyPressed2(SDL_Keysym ev) override {
    return elem->onKeyPressed2(ev);
  }

  virtual optional<int> getPreferredWidth() override {
    return elem->getPreferredWidth();
  }

  virtual optional<int> getPreferredHeight() override {
    return elem->getPreferredHeight();
  }

  private:
  GuiElem* elem;
};

SGuiElem GuiFactory::external(GuiElem* elem) {
  return SGuiElem(new External(elem)); 
}

class Focusable : public GuiStack {
  public:
  Focusable(SGuiElem content, Renderer& renderer, vector<SDL_Keysym> focus, vector<SDL_Keysym> defocus,
      bool& foc)
    : GuiStack(makeVec(std::move(content))), focusEvent(focus), defocusEvent(defocus), focused(foc),
      renderer(renderer) {}

  virtual bool onClick(MouseButtonId b, Vec2 pos) override {
    if (b == MouseButtonId::LEFT) {
      if (focused && !pos.inRectangle(getBounds())) {
        focused = false;
        renderer.getSteamInput()->popActionSet();
        return true;
      }
      return GuiLayout::onClick(b, pos);
    }
    return false;
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (!focused)
      for (auto& elem : focusEvent)
        if (GuiFactory::keyEventEqual(elem, key)) {
          focused = true;
          renderer.getSteamInput()->pushActionSet(MySteamInput::ActionSet::MENU);
          return true;
        }
    if (focused)
      for (auto& elem : defocusEvent)
        if (GuiFactory::keyEventEqual(elem, key)) {
          focused = false;
          renderer.getSteamInput()->popActionSet();
          return true;
        }
    if (focused) {
      GuiLayout::onKeyPressed2(key);
      return true;
    } else
      return false;
  }

  private:
  vector<SDL_Keysym> focusEvent;
  vector<SDL_Keysym> defocusEvent;
  bool& focused;
  Renderer& renderer;
};

SGuiElem GuiFactory::focusable(SGuiElem content, vector<SDL_Keysym> focusEvent,
    vector<SDL_Keysym> defocusEvent, bool& focused) {
  return SGuiElem(new Focusable(std::move(content), renderer, focusEvent, defocusEvent, focused));
}

class KeyHandler : public GuiElem {
  public:
  KeyHandler(function<void(SDL_Keysym)> f, bool cap) : fun(f), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    fun(key);
    return capture;
  }

  private:
  function<void(SDL_Keysym)> fun;
  bool capture;
};

class RenderInBounds : public GuiStack {
  public:
  RenderInBounds(SGuiElem e) : GuiStack(std::move(e)) {}

  virtual void render(Renderer& r) override {
    r.setScissor(getBounds());
    elems[0]->render(r);
    r.setScissor(none);
  }
};

SGuiElem GuiFactory::renderInBounds(SGuiElem elem) {
  return SGuiElem(new RenderInBounds(std::move(elem)));
}

class AlignmentGui : public GuiLayout {
  public:
  AlignmentGui(SGuiElem e, GuiFactory::Alignment align, optional<Vec2> sz)
      : GuiLayout(makeVec(std::move(e))), alignment(align), size(sz) {}

  int getWidth() {
    if (size)
      return size->x;
    else
      return *elems[0]->getPreferredWidth();
  }

  int getHeight() {
    if (size)
      return size->y;
    else
      return *elems[0]->getPreferredHeight();
  }

  virtual Rectangle getElemBounds(int num) override {
    switch (alignment) {
      case GuiFactory::Alignment::BOTTOM:
        return Rectangle(getBounds().left(), getBounds().bottom() - getHeight(),
            getBounds().right(), getBounds().bottom());
      case GuiFactory::Alignment::TOP_RIGHT:
        return Rectangle(getBounds().right() - getWidth(), getBounds().top(), getBounds().right(),
            getBounds().top() + getHeight());
      case GuiFactory::Alignment::RIGHT:
        return Rectangle(getBounds().right() - getWidth(), getBounds().top(), getBounds().right(),
            getBounds().bottom());
      case GuiFactory::Alignment::BOTTOM_RIGHT:
        return Rectangle(getBounds().right() - getWidth(), getBounds().bottom() - getHeight(), getBounds().right(),
            getBounds().bottom());
      case GuiFactory::Alignment::BOTTOM_LEFT:
        return Rectangle(getBounds().left(), getBounds().bottom() - getHeight(), getBounds().left() + getWidth(),
            getBounds().bottom());
      case GuiFactory::Alignment::BOTTOM_CENTER:
        return Rectangle(getBounds().middle().x - getWidth() / 2, getBounds().bottom() - getHeight(),
            getBounds().middle().x + getWidth() / 2, getBounds().bottom());
      default: FATAL << "Unhandled alignment, please implement me :) " << (int)alignment;
    }
    return Rectangle();
  }

  private:
  GuiFactory::Alignment alignment;
  optional<Vec2> size;
};

SGuiElem GuiFactory::alignment(GuiFactory::Alignment alignment, SGuiElem content, optional<Vec2> size) {
  return SGuiElem(new AlignmentGui(std::move(content), alignment, size));
}
 
SGuiElem GuiFactory::keyHandler(function<void(SDL_Keysym)> fun, bool capture) {
  return SGuiElem(new KeyHandler(fun, capture));
}

class KeybindingHandler : public GuiElem {
  public:
  KeybindingHandler(KeybindingMap* m, Keybinding key, function<void()> fun, bool cap)
      : fun(std::move(fun)), keybindingMap(m), key(key), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym sym) override {
    if (keybindingMap->matches(key, sym)) {
      fun();
      return capture;
    }
    return false;
  }

  private:
  function<void()> fun;
  KeybindingMap* keybindingMap;
  Keybinding key;
  bool capture;
};

SGuiElem GuiFactory::keyHandler(function<void()> fun, Keybinding keybinding, bool capture) {
  return SGuiElem(new KeybindingHandler(options->getKeybindingMap(), keybinding, std::move(fun), capture));
}

KeybindingMap* GuiFactory::getKeybindingMap() {
  return options->getKeybindingMap();
}

class KeyHandler2 : public GuiElem {
  public:
  KeyHandler2(function<void(Rectangle)> f, vector<SDL_Keysym> k, bool cap) : fun(f), key(k), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym k) override {
    for (auto& elem : key)
      if (GuiFactory::keyEventEqual(k, elem)) {
        fun(getBounds());
        return capture;
      }
    return false;
  }

  private:
  function<void(Rectangle)> fun;
  vector<SDL_Keysym> key;
  bool capture;
};

SGuiElem GuiFactory::keyHandler(function<void()> fun, vector<SDL_Keysym> key, bool capture) {
  return SGuiElem(new KeyHandler2([fun = std::move(fun)](Rectangle) { fun();}, key, capture));
}

SGuiElem GuiFactory::keyHandlerRect(function<void(Rectangle)> fun, vector<SDL::SDL_Keysym> key, bool capture) {
  return SGuiElem(new KeyHandler2(fun, key, capture));
}

class ElemList : public GuiLayout {
  public:
  ElemList(vector<SGuiElem> e, vector<int> h, int alignBack, bool middleEl)
    : GuiLayout(std::move(e)), heights(h), numAlignBack(alignBack), middleElem(middleEl) {
    CHECK(heights.size() == elems.size());
    int sum = 0;
    for (int h : heights) {
      accuHeights.push_back(sum);
      sum += h;
    }
    accuHeights.push_back(sum);
  }

  Range getBackPosition(int num, Range bounds) {
    int height = accuHeights[heights.size()] - accuHeights[num + 1];
    return Range(bounds.getEnd() - height - heights[num], bounds.getEnd() - height);
  }

  Range getMiddlePosition(Range bounds) {
    CHECK(heights.size() > numAlignBack);
    int height1 = accuHeights[heights.size() - numAlignBack - 1];
    int height2 = accuHeights[heights.size()] - accuHeights[heights.size() - numAlignBack];
    return Range(bounds.getStart() + height1, bounds.getEnd() - height2);
  }

  Range getElemPosition(int num, Range bounds) {
    if (middleElem && num == heights.size() - numAlignBack - 1)
      return getMiddlePosition(bounds);
    if (num >= heights.size() - numAlignBack)
      return getBackPosition(num, bounds);
    int height = accuHeights[num];
    return Range(bounds.getStart() + height, bounds.getStart() + height + heights[num]);
  }

  int getLastTopElem(int myHeight) {
    int totHeight = 0;
    for (int i = heights.size() - 1; i >= 0; --i) {
      if (totHeight + heights[i] > myHeight) {
        CHECK( i < heights.size() - 1) << "Couldn't fit list element in window";
        return i + 1;
      }
      totHeight += heights[i];
    }
    return 0;
  }

  int getSize() {
    return heights.size();
  }

  int getTotalHeight() {
    return accuHeights.back();
  }


  protected:
  vector<int> heights;
  vector<int> accuHeights;
  int numAlignBack;
  bool middleElem;
};

class VerticalList : public ElemList {
  public:
  using ElemList::ElemList;

  virtual void renderPart(Renderer& r, Rectangle rect) override {
    for (int i : All(elems))
      if (elems[i]->getBounds().intersects(rect))
        elems[i]->render(r);
  }

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().getXRange(), getElemPosition(num, getBounds().getYRange()));
  }

  optional<int> getPreferredWidth() override {
    optional<int> ret;
    for (auto& elem : elems)
      if (auto width = elem->getPreferredWidth())
        if (ret.value_or(-1) < *width)
          ret = width;
    return ret;
  }

  optional<int> getPreferredHeight() override {
    return getTotalHeight();
  }
};

SGuiElem GuiFactory::verticalList(vector<SGuiElem> e, int height) {
  vector<int> heights(e.size(), height);
  return SGuiElem(new VerticalList(std::move(e), heights, 0, false));
}

class HorizontalList : public ElemList {
  public:
  using ElemList::ElemList;

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getElemPosition(num, getBounds().getXRange()), getBounds().getYRange());
  }

  optional<int> getPreferredHeight() override {
    optional<int> ret;
    for (auto& elem : elems)
      if (auto height = elem->getPreferredHeight())
        if (ret.value_or(-1) < *height)
          ret = height;
    return ret;
  }

  optional<int> getPreferredWidth() override {
    return getTotalHeight();
  }
};

SGuiElem GuiFactory::horizontalList(vector<SGuiElem> e, int height) {
  vector<int> heights(e.size(), height);
  return SGuiElem(new HorizontalList(std::move(e), heights, 0, false));
}

GuiFactory::ListBuilder GuiFactory::getListBuilder(int defaultSize) {
  return ListBuilder(*this, defaultSize);
}

GuiFactory::ListBuilder::ListBuilder(GuiFactory& g, int defSz) : gui(g), defaultSize(defSz) {}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElem(SGuiElem elem, int size) {
  CHECK(!backElems);
  CHECK(!middleElem);
  if (size == 0) {
    CHECK(defaultSize > 0);
    size = defaultSize;
  }
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addSpace(int size) {
  if (!backElems && !middleElem)
    return addElem(gui.empty(), size);
  else
    return addBackElem(gui.empty(), size);
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElemAuto(SGuiElem elem) {
  CHECK(!backElems);
  CHECK(!middleElem);
  elems.push_back(std::move(elem));
  sizes.push_back(-1);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addMiddleElem(SGuiElem elem) {
  CHECK(!backElems);
  CHECK(!middleElem);
  elems.push_back(std::move(elem));
  sizes.push_back(-1);
  middleElem = true;
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackElemAuto(SGuiElem elem) {
  ++backElems;
  elems.push_back(std::move(elem));
  sizes.push_back(-1);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackElem(SGuiElem elem, int size) {
  ++backElems;
  if (size == 0) {
    CHECK(defaultSize > 0);
    size = defaultSize;
  }
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackSpace(int size) {
  return addBackElem(gui.empty(), size);
}

int GuiFactory::ListBuilder::getSize() const {
  return std::accumulate(sizes.begin(), sizes.end(), 0);
}

int GuiFactory::ListBuilder::getLength() const {
  return sizes.size();
}

bool GuiFactory::ListBuilder::isEmpty() const {
  return sizes.empty();
}

void GuiFactory::ListBuilder::clear() {
  elems.clear();
  sizes.clear();
  backElems = 0;
  middleElem = false;
}

SGuiElem GuiFactory::ListBuilder::buildVerticalList() {
  for (int i : All(sizes)) {
    if (sizes[i] == -1) {
      if (middleElem && i == sizes.size() - backElems - 1)
        sizes[i] = elems[i]->getPreferredHeight().value_or(1);
      else
        sizes[i] = *elems[i]->getPreferredHeight();
    }
  }
  auto ret = SGuiElem(new VerticalList(std::move(elems), sizes, backElems, middleElem));
  if (lineNumber)
    ret->setLineNumber(*lineNumber);
  return ret;
}

SGuiElem GuiFactory::ListBuilder::buildHorizontalList() {
  for (int i : All(sizes))
    if (sizes[i] == -1) {
      if (middleElem && i == sizes.size() - backElems - 1)
        sizes[i] = elems[i]->getPreferredWidth().value_or(1);
      else
        sizes[i] = *elems[i]->getPreferredWidth();
    }
  auto ret = SGuiElem(new HorizontalList(std::move(elems), sizes, backElems, middleElem));
  if (lineNumber)
    ret->setLineNumber(*lineNumber);
  return ret;
}

SGuiElem GuiFactory::ListBuilder::buildHorizontalListFit(double spacing) {
  SGuiElem ret = gui.horizontalListFit(std::move(elems), spacing);
  if (lineNumber)
    ret->setLineNumber(*lineNumber);
  return ret;
}

SGuiElem GuiFactory::ListBuilder::buildVerticalListFit() {
  SGuiElem ret = gui.verticalListFit(std::move(elems), 0);
  if (lineNumber)
    ret->setLineNumber(*lineNumber);
  return ret;
}

class VerticalListFit : public GuiLayout {
  public:
  VerticalListFit(vector<SGuiElem> e, double space) : GuiLayout(std::move(e)), spacing(space) {
    CHECK(!elems.empty());
  }

  virtual Rectangle getElemBounds(int num) override {
    int elemHeight = double(getBounds().height()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().topLeft() + Vec2(0, num * (elemHeight * (1.0 + spacing))), 
        getBounds().topRight() + Vec2(0, num * (elemHeight * (1.0 + spacing)) + elemHeight));
  }

  optional<int> getPreferredWidth() override {
    optional<int> ret;
    for (auto& elem : elems)
      if (auto width = elem->getPreferredWidth())
        if (ret.value_or(-1) < *width)
          ret = width;
    return ret;
  }

  protected:
  double spacing;
};


SGuiElem GuiFactory::verticalListFit(vector<SGuiElem> e, double spacing) {
  return SGuiElem(new VerticalListFit(std::move(e), spacing));
}

class HorizontalListFit : public GuiLayout {
  public:
  HorizontalListFit(vector<SGuiElem> e, double space) : GuiLayout(std::move(e)), spacing(space) {
    //CHECK(!elems.empty());
  }

  virtual Rectangle getElemBounds(int num) override {
    int elemHeight = double(getBounds().width()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().topLeft() + Vec2(num * (elemHeight * (1.0 + spacing)), 0), 
        getBounds().bottomLeft() + Vec2(num * (elemHeight * (1.0 + spacing)) + elemHeight, 0));
  }

  optional<int> getPreferredHeight() override {
    optional<int> ret;
    for (auto& elem : elems)
      if (auto height = elem->getPreferredHeight())
        if (ret.value_or(-1) < *height)
          ret = height;
    return ret;
  }

  protected:
  double spacing;
};


SGuiElem GuiFactory::horizontalListFit(vector<SGuiElem> e, double spacing) {
  return SGuiElem(new HorizontalListFit(std::move(e), spacing));
}

class VerticalAspect : public GuiLayout {
  public:
  VerticalAspect(SGuiElem e, double r) : GuiLayout(makeVec(std::move(e))), ratio(r) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0);
    double width = ratio * getBounds().height();
    return Rectangle((getBounds().topLeft() + getBounds().topRight()) / 2 - Vec2(width / 2, 0),
        (getBounds().bottomLeft() + getBounds().bottomRight()) / 2 + Vec2(width / 2, 0));
  }

  private:
  double ratio;
};

SGuiElem GuiFactory::verticalAspect(SGuiElem elem, double ratio) {
  return SGuiElem(new VerticalAspect(std::move(elem), ratio));
}

class CenterHoriz : public GuiLayout {
  public:
  CenterHoriz(SGuiElem elem, optional<int> w) : GuiLayout(makeVec(std::move(elem))),
      width(w) {}

  optional<int> getPreferredHeight() override {
    if (auto height = elems[0]->getPreferredHeight())
      return *height;
    else
      return none;
  }

  optional<int> getPreferredWidth() override {
    if (auto width = elems[0]->getPreferredWidth())
      return *width;
    else
      return none;
  }

  virtual Rectangle getElemBounds(int num) override {
    int center = (getBounds().left() + getBounds().right()) / 2;
    int myWidth = width ? *width : max(2, *elems[0]->getPreferredWidth());
    return Rectangle(center - myWidth / 2, getBounds().top(), center + myWidth / 2, getBounds().bottom());
  }

  private:
  optional<int> width;
};

SGuiElem GuiFactory::centerHoriz(SGuiElem e, optional<int> width) {
  if (width && *width == 0)
    return empty();
  return SGuiElem(new CenterHoriz(std::move(e), width));
}

class CenterVert : public GuiLayout {
  public:
  CenterVert(SGuiElem elem, optional<int> h) : GuiLayout(makeVec(std::move(elem))),
      height(h) {}

  optional<int> getPreferredWidth() override {
    if (auto width = elems[0]->getPreferredWidth())
      return *width;
    else
      return none;
  }

  virtual Rectangle getElemBounds(int num) override {
    int center = (getBounds().top() + getBounds().bottom()) / 2;
    int myHeight = height ? *height : max(2, *elems[0]->getPreferredHeight());
    return Rectangle(getBounds().left(), center - myHeight / 2, getBounds().right(), center + myHeight / 2);
  }

  private:
  optional<int> height;
};

SGuiElem GuiFactory::centerVert(SGuiElem e, optional<int> height) {
  if (height && *height == 0)
    return empty();
  return SGuiElem(new CenterVert(std::move(e), height));
}

class MarginGui : public GuiLayout {
  public:
  MarginGui(SGuiElem top, SGuiElem rest, int _width, GuiFactory::MarginType t)
    : GuiLayout(makeVec(std::move(top), std::move(rest))), width(_width), type(t) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0 || num == 1);
    if (num == 0)
      switch (type) { // the margin
        case GuiFactory::TOP:
          return Rectangle(getBounds().topLeft(), getBounds().topRight() + Vec2(0, width));
        case GuiFactory::LEFT:
          return Rectangle(getBounds().topLeft(), getBounds().bottomLeft() + Vec2(width, 0));
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().topRight() - Vec2(width, 0), getBounds().bottomRight());
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().bottomLeft() - Vec2(0, width), getBounds().bottomRight());
      }
    else
      switch (type) { // the remainder
        case GuiFactory::TOP:
          return Rectangle(getBounds().topLeft() + Vec2(0, width), getBounds().bottomRight());
        case GuiFactory::LEFT:
          return Rectangle(getBounds().topLeft() + Vec2(width, 0), getBounds().bottomRight());
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().topLeft(), getBounds().bottomRight() - Vec2(width, 0));
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().topLeft(), getBounds().bottomRight() - Vec2(0, width));
      }
  }

  private:
  int width;
  GuiFactory::MarginType type;
};

SGuiElem GuiFactory::margin(SGuiElem top, SGuiElem rest, int width, MarginType type) {
  return SGuiElem(new MarginGui(std::move(top), std::move(rest), width, type));
}

SGuiElem GuiFactory::marginAuto(SGuiElem top, SGuiElem rest, MarginType type) {
  int width;
  switch (type) {
    case MarginType::LEFT:
    case MarginType::RIGHT: width = *top->getPreferredWidth(); break;
    case MarginType::TOP:
    case MarginType::BOTTOM: width = *top->getPreferredHeight(); break;
  }
  return SGuiElem(new MarginGui(std::move(top), std::move(rest), width, type));
}

class MaybeMargin : public MarginGui {
  public:
  MaybeMargin(SGuiElem top, SGuiElem rest, int width, GuiFactory::MarginType type, function<bool(Rectangle)> p)
      : MarginGui(std::move(top), std::move(rest), width, type), pred(p) {}

  virtual Rectangle getElemBounds(int num) override {
    if (pred(getBounds()))
      return MarginGui::getElemBounds(num);
    else
      return getBounds();
  }

  virtual bool isVisible(int num) override {
    return pred(getBounds()) || num == 1;
  }

  private:
  function<bool(Rectangle)> pred;
};

SGuiElem GuiFactory::maybeMargin(SGuiElem top, SGuiElem rest, int width, MarginType type,
    function<bool(Rectangle)> pred) {
  return SGuiElem(new MaybeMargin(std::move(top), std::move(rest), width, type, pred));
}

class FullScreen : public GuiLayout {
  public:
  FullScreen(SGuiElem content, Renderer& r) : GuiLayout(makeVec(std::move(content))), renderer(r) {
  }

  virtual void render(Renderer& r) override {
    GuiLayout::render(r);
  }

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(renderer.getSize());
  }

  private:
  Renderer& renderer;
};

SGuiElem GuiFactory::fullScreen(SGuiElem content) {
  return SGuiElem(new FullScreen(std::move(content), renderer));
}

class AbsolutePosition : public GuiLayout {
  public:
  AbsolutePosition(SGuiElem content, Vec2 pos, Renderer& r)
      : GuiLayout(makeVec(std::move(content))), position(pos), renderer(r) {
  }

  virtual void render(Renderer& r) override {
    GuiLayout::render(r);
  }

  virtual Rectangle getElemBounds(int num) override {
    auto size = Vec2(*elems[0]->getPreferredWidth(), *elems[0]->getPreferredHeight());
    auto pos = position;
    pos.x = min(pos.x, renderer.getSize().x - size.x);
    pos.y = min(pos.y, renderer.getSize().y - size.y);
    return Rectangle(pos, pos + size);
  }

  private:
  Vec2 position;
  Renderer& renderer;
};

SGuiElem GuiFactory::absolutePosition(SGuiElem content, Vec2 pos) {
  return SGuiElem(new AbsolutePosition(std::move(content), pos, renderer));
}

class MarginFit : public GuiLayout {
  public:
  MarginFit(SGuiElem top, SGuiElem rest, double _width, GuiFactory::MarginType t)
    : GuiLayout(makeVec(std::move(top), std::move(rest))), width(_width), type(t) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0 || num == 1);
    int w = getBounds().width();
    int h = getBounds().height();
    if (num == 0)
      switch (type) { // the margin
        case GuiFactory::TOP:
          return Rectangle(getBounds().topLeft(), getBounds().topRight() + Vec2(0, width * h));
        case GuiFactory::LEFT:
          return Rectangle(getBounds().topLeft(), getBounds().bottomLeft() + Vec2(width * w, 0));
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().topRight() - Vec2(width * w, 0), getBounds().bottomRight());
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().bottomLeft() - Vec2(0, width * h), getBounds().bottomRight());
      }
    else
      switch (type) { // the remainder
        case GuiFactory::TOP:
          return Rectangle(getBounds().topLeft() + Vec2(0, width * h), getBounds().bottomRight());
        case GuiFactory::LEFT:
          return Rectangle(getBounds().topLeft() + Vec2(width * w, 0), getBounds().bottomRight());
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().topLeft(), getBounds().bottomRight() - Vec2(width * w, 0));
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().topLeft(), getBounds().bottomRight() - Vec2(0, width * h));
      }
  }

  private:
  double width;
  GuiFactory::MarginType type;
};

SGuiElem GuiFactory::marginFit(SGuiElem top, SGuiElem rest, double width, MarginType type) {
  return SGuiElem(new MarginFit(std::move(top), std::move(rest), width, type));
}

SGuiElem GuiFactory::progressBar(Color c, double state) {
  return SGuiElem(new DrawCustom([=] (Renderer& r, Rectangle bounds) {
          int width = bounds.width() * state;
          if (width > 0)
            r.drawFilledRectangle(Rectangle(bounds.topLeft(),
                  Vec2(bounds.left() + width, bounds.bottom())), c);
        }));
}

class Margins : public GuiLayout {
  public:
  Margins(SGuiElem content, int l, int t, int r, int b)
      : GuiLayout(makeVec(std::move(content))), left(l), top(t), right(r), bottom(b) {}

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().left() + left, getBounds().top() + top,
        getBounds().right() - right, getBounds().bottom() - bottom);
  }

  optional<int> getPreferredWidth() override {
    if (auto width = elems[0]->getPreferredWidth())
      return left + *width + right;
    else
      return none;
  }

  optional<int> getPreferredHeight() override {
    if (auto height = elems[0]->getPreferredHeight())
      return top + *height + bottom;
    else
      return none;
  }

  private:
  int left;
  int top;
  int right;
  int bottom;
};

SGuiElem GuiFactory::margins(SGuiElem content, int left, int top, int right, int bottom) {
  return SGuiElem(new Margins(std::move(content), left, top, right, bottom));
}

SGuiElem GuiFactory::margins(SGuiElem content, int all) {
  return SGuiElem(new Margins(std::move(content), all, all, all, all));
}

SGuiElem GuiFactory::leftMargin(int size, SGuiElem content) {
  return SGuiElem(new Margins(std::move(content), size, 0, 0, 0));
}

SGuiElem GuiFactory::rightMargin(int size, SGuiElem content) {
  return SGuiElem(new Margins(std::move(content), 0, 0, size, 0));
}

SGuiElem GuiFactory::topMargin(int size, SGuiElem content) {
  return SGuiElem(new Margins(std::move(content), 0, size, 0, 0));
}

SGuiElem GuiFactory::bottomMargin(int size, SGuiElem content) {
  return SGuiElem(new Margins(std::move(content), 0, 0, 0, size));
}

class Invisible : public GuiStack {
  public:
  Invisible(SGuiElem content) : GuiStack(makeVec(std::move(content))) {}

  virtual bool isVisible(int num) {
    return false;
  }
};

SGuiElem GuiFactory::invisible(SGuiElem content) {
  return SGuiElem(new Invisible(std::move(content)));
}

class Switchable : public GuiLayout {
  public:
  Switchable(vector<SGuiElem> elems, function<int()> fun) : GuiLayout(std::move(elems)), switchFun(fun) {}

  virtual bool isVisible(int num) {
    return switchFun() == num;
  }

  private:
  function<int()> switchFun;
};

namespace {


class PreferredSize : public GuiLayout {
  public:

  PreferredSize(SGuiElem content, optional<int> w, optional<int> h)
      : GuiLayout(std::move(content)), width(w), height(h) {}

  virtual optional<int> getPreferredWidth() override {
    return width ? width : elems[0]->getPreferredWidth();
  }
  
  virtual optional<int> getPreferredHeight() override {
    return height ? height : elems[0]->getPreferredHeight();
  }

  virtual Rectangle getElemBounds(int num) override {
    auto bounds = getBounds();
    return Rectangle(bounds.left(), bounds.top(),
        bounds.left() + max(1, min(bounds.width(), width.value_or(bounds.width()))),
        bounds.top() + max(1, min(bounds.height(), height.value_or(bounds.height()))));
  }

  private:
  optional<int> width;
  optional<int> height;
};

}

SGuiElem GuiFactory::preferredSize(int width, int height, SGuiElem elem) {
  return SGuiElem(new PreferredSize(std::move(elem), width, height));
}

SGuiElem GuiFactory::preferredSize(Vec2 size, SGuiElem elem) {
  return SGuiElem(new PreferredSize(std::move(elem), size.x, size.y));
}

SGuiElem GuiFactory::setHeight(int height, SGuiElem content) {
  return SGuiElem(new PreferredSize(std::move(content), none, height));
}

SGuiElem GuiFactory::setWidth(int width, SGuiElem content) {
  return SGuiElem(new PreferredSize(std::move(content), width, none));
}

SGuiElem GuiFactory::empty() {
  return SGuiElem(new PreferredSize(SGuiElem(new GuiElem()), 1, 1));
}

class ViewObjectGui : public GuiElem {
  public:
  ViewObjectGui(const ViewObject& obj, Vec2 sz, double sc, Color c) : object(obj), size(sz), scale(sc), color(c) {}
  ViewObjectGui(ViewIdList id, Vec2 sz, double sc, Color c) : object(id), size(sz), scale(sc), color(c) {
    //CHECK(int(id) >= 0 && int(id) < EnumInfo<ViewId>::size);
  }
  ViewObjectGui(function<ViewId()> id, Vec2 sz, double sc, Color c)
      : object(std::move(id)), size(sz), scale(sc), color(c) {}

  ViewObjectGui(function<ViewObject()> id, Vec2 sz, double sc, Color c)
      : object(std::move(id)), size(sz), scale(sc), color(c) {}

  virtual void render(Renderer& renderer) override {
    object.match(
          [&](const ViewObject& obj) {
            renderer.drawViewObject(getBounds().topLeft(), obj, true, scale, color);
          },
          [&](const ViewIdList& viewId) {
            renderer.drawViewObject(getBounds().topLeft(), viewId, true, scale, color);
          },
          [&](function<ViewId()> viewId) {
            renderer.drawViewObject(getBounds().topLeft(), viewId(), true, scale, color);
          },
          [&](function<ViewObject()> viewId) {
            renderer.drawViewObject(getBounds().topLeft(), viewId(), true, scale, color);
          }
    );
  }

  virtual optional<int> getPreferredWidth() override {
    return size.x;
  }

  virtual optional<int> getPreferredHeight() override {
    return size.y;
  }

  private:
  variant<ViewObject, ViewIdList  , function<ViewId()>, function<ViewObject()>> object;
  Vec2 size;
  double scale;
  Color color;
};

SGuiElem GuiFactory::viewObject(const ViewObject& object, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(object, Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::viewObject(ViewId id, double scale, Color color) {
  return SGuiElem(new ViewObjectGui({id}, Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::viewObject(ViewIdList list, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(list, Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::viewObject(function<ViewId()> id, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(std::move(id), Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::viewObject(function<ViewObject()> id, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(std::move(id), Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::asciiBackground(ViewId id) {
  return SGuiElem(
      new DrawCustom([=] (Renderer& renderer, Rectangle bounds) { renderer.drawAsciiBackground(id, bounds);}));
}

class DragSource : public GuiElem {
  public:
  DragSource(DragContainer& c, DragContent d, function<SGuiElem()> g) : container(c), content(d), gui(g) {}

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    if (b == MouseButtonId::LEFT && v.inRectangle(getBounds()))
      container.put(content, gui(), v);
    return false;
  }

  private:
  DragContainer& container;
  DragContent content;
  function<SGuiElem()> gui;
};

class OnMouseRelease : public GuiElem {
  public:
  OnMouseRelease(function<void()> f) : fun(f) {}

  virtual bool onClick(MouseButtonId id, Vec2 v) override {
    if (id == MouseButtonId::RELEASED && v.inRectangle(getBounds()))
      fun();
    return false;
  }

  private:
  function<void()> fun;
};

SGuiElem GuiFactory::dragSource(DragContent content, function<SGuiElem()> gui) {
  return SGuiElem(new DragSource(dragContainer, content, gui));
}

SGuiElem GuiFactory::dragListener(function<void(DragContent)> fun) {
  return SGuiElem(new OnMouseRelease([this, fun] {
        if (auto content = dragContainer.pop())
          fun(*content);
      }));
}

class TranslateGui : public GuiLayout {
  public:
  using Corner = GuiFactory::TranslateCorner;
  TranslateGui(SGuiElem e, Vec2 p, optional<Vec2> s, Corner corner)
      : GuiLayout(makeVec(std::move(e))), pos(p), size(s), corner(corner) {
  }

  Vec2 getCorner() {
    switch (corner) {
      case Corner::TOP_LEFT:
        return getBounds().topLeft();
      case Corner::TOP_RIGHT:
        return getBounds().topRight();
      case Corner::BOTTOM_LEFT:
        return getBounds().bottomLeft();
      case Corner::BOTTOM_RIGHT:
        return getBounds().bottomRight();
    }
  }

  virtual Rectangle getElemBounds(int num) override {
    Vec2 sz = size ? *size : getBounds().getSize();
    auto corner = getCorner();
    return Rectangle(corner + pos, corner + pos + sz);
  }

  virtual optional<int> getPreferredWidth() override {
    return elems[0]->getPreferredWidth();
  }

  virtual optional<int> getPreferredHeight() override {
    return elems[0]->getPreferredHeight();
  }

  private:
  Vec2 pos;
  optional<Vec2> size;
  GuiFactory::TranslateCorner corner;
};

SGuiElem GuiFactory::translate(SGuiElem e, Vec2 pos, optional<Vec2> size, TranslateCorner corner) {
  return SGuiElem(new TranslateGui(std::move(e), pos, size, corner));
}

class TranslateGui2 : public GuiLayout {
  public:
  TranslateGui2(SGuiElem e, function<Vec2()> v) : GuiLayout(makeVec(std::move(e))), vec(v) {
  }

  virtual Rectangle getElemBounds(int num) override {
    return getBounds().translate(vec());
  }

  private:
  function<Vec2()> vec;
};

SGuiElem GuiFactory::translate(function<Vec2()> f, SGuiElem e) {
  return SGuiElem(new TranslateGui2(std::move(e), f));
}

class TranslateAbsolute : public GuiLayout {
  public:
  TranslateAbsolute(SGuiElem e, function<Vec2()> v) : GuiLayout(makeVec(std::move(e))), vec(v) {
  }

  virtual Rectangle getElemBounds(int num) override {
    auto pos = vec();
    return Rectangle(pos, pos + Vec2(*elems[0]->getPreferredWidth(), *elems[0]->getPreferredHeight()));
  }

  private:
  function<Vec2()> vec;
};

SGuiElem GuiFactory::translateAbsolute(function<Vec2()> f, SGuiElem elem) {
  return SGuiElem(new TranslateAbsolute(std::move(elem), std::move(f)));
}

SGuiElem GuiFactory::onRenderedAction(function<void()> fun) {
  return SGuiElem(new DrawCustom([=] (Renderer& r, Rectangle bounds) { fun(); }));
}

class MouseOverAction : public GuiElem {
  public:
  MouseOverAction(function<void()> f, function<void()> f2) : callback(f), outCallback(f2) {}

  virtual void onMouseGone() override {
    if (in) {
      in = false;
      if (outCallback)
        outCallback();
    }
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    if ((!in || !outCallback) && pos.inRectangle(getBounds())) {
      callback();
      in = true;
    } else if (!pos.inRectangle(getBounds()))
      onMouseGone();
    return false;
  }

  ~MouseOverAction() {
    onMouseGone();
  }

  private:
  function<void()> callback;
  bool in = false;
  function<void()> outCallback;
};

SGuiElem GuiFactory::mouseOverAction(function<void()> callback, function<void()> outCallback) {
  return SGuiElem(new MouseOverAction(callback, outCallback));
}

class MouseButtonHeld : public GuiStack {
  public:
  MouseButtonHeld(SGuiElem elem, MouseButtonId but) : GuiStack(std::move(elem)), button(but) {}

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    if (b == MouseButtonId::RELEASED)
      on = false;
    if (button == b && v.inRectangle(getBounds()))
      on = true;
    return false;
  }

  virtual void onMouseGone() override {
    on = false;
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    if (!pos.inRectangle(getBounds()))
      on = false;
    return false;
  }

  virtual bool isVisible(int num) override {
    return on;
  }

  private:
  const MouseButtonId button;
  bool on = false;
};

SGuiElem GuiFactory::onMouseLeftButtonHeld(SGuiElem elem) {
  return SGuiElem(new MouseButtonHeld(std::move(elem), MouseButtonId::LEFT));
}

SGuiElem GuiFactory::onMouseRightButtonHeld(SGuiElem elem) {
  return SGuiElem(new MouseButtonHeld(std::move(elem), MouseButtonId::RIGHT));
}

class MouseHighlightBase : public GuiStack {
  public:
  MouseHighlightBase(SGuiElem h, int ind, optional<int>* highlight)
    : GuiStack(makeVec(std::move(h))), myIndex(ind), highlighted(highlight) {}

  virtual void render(Renderer& r) override {
    if (*highlighted == myIndex)
      elems[0]->render(r);
  }

  protected:
  int myIndex;
  optional<int>* highlighted;
  bool canTurnOff = false;
};

class MouseHighlight : public MouseHighlightBase {
  public:
  using MouseHighlightBase::MouseHighlightBase;

  virtual void onMouseGone() override {
    if (*highlighted == myIndex && canTurnOff) {
      *highlighted = none;
      canTurnOff = false;
    }
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    if (pos.inRectangle(getBounds())) {
      *highlighted = myIndex;
      canTurnOff = true;
      return true;
    } else if (*highlighted == myIndex && canTurnOff) {
      *highlighted = none;
      canTurnOff = false;
    }
    return false;
  }
};

class MouseHighlight2 : public GuiStack {
  public:
  MouseHighlight2(SGuiElem h, SGuiElem h2, bool capture)
      : GuiStack(h2 ? makeVec(std::move(h), std::move(h2)) : makeVec(std::move(h))), capture(capture) {}

  virtual void render(Renderer& r) override {
    if (over)
      elems[0]->render(r);
    else if (elems.size() > 1)
      elems[1]->render(r);
  }

  virtual void onMouseGone() override {
    over = false;
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    over = pos.inRectangle(getBounds());
    return over && capture;
  }

  bool over = false;
  bool capture;
};

SGuiElem GuiFactory::mouseHighlight(SGuiElem elem, int myIndex, optional<int>* highlighted) {
  return SGuiElem(new MouseHighlight(std::move(elem), myIndex, highlighted));
}

SGuiElem GuiFactory::mouseHighlight2(SGuiElem elem, SGuiElem noHighlight, bool capture) {
  return SGuiElem(new MouseHighlight2(std::move(elem), std::move(noHighlight), capture));
}

class RenderLayer : public GuiStack {
  public:
  RenderLayer(SGuiElem content) : GuiStack(std::move(content)) {}

  virtual void render(Renderer& r) override {
    r.setTopLayer();
    elems[0]->render(r);
    r.popLayer();
  }
};

SGuiElem GuiFactory::renderTopLayer(SGuiElem content) {
  return make_shared<RenderLayer>(std::move(content));
}

class Tooltip2 : public GuiElem {
  public:
  Tooltip2(SGuiElem e, GuiFactory::PositionFun pos)
      : elem(std::move(e)), size(*elem->getPreferredWidth(), *elem->getPreferredHeight()), positionFun(pos) {
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    canRender = pos.inRectangle(getBounds());
    return false;
  }

  virtual void onMouseGone() override {
    canRender = false;
  }

  virtual void render(Renderer& r) override {
    if (canRender) {
      Vec2 pos = positionFun(getBounds());
      pos.x = min(pos.x, r.getSize().x - size.x);
      pos.y = min(pos.y, r.getSize().y - size.y);
      r.setTopLayer();
      elem->setBounds(Rectangle(pos, pos + size));
      elem->render(r);
      r.popLayer();
    }
  }

  private:
  bool canRender = false;
  SGuiElem elem;
  Vec2 size;
  GuiFactory::PositionFun positionFun;
};

SGuiElem GuiFactory::tooltip2(SGuiElem elem, PositionFun positionFun) {
  return SGuiElem(new Tooltip2(std::move(elem), positionFun));
}

const static int tooltipLineHeight = 28;
const static int tooltipHMargin = 15;
const static int tooltipVMargin = 15;
const static Vec2 tooltipOffset = Vec2(10, 10);

class Tooltip : public GuiElem {
  public:
  Tooltip(const vector<string>& t, SGuiElem bg, Clock* c, milliseconds d) : text(t), background(std::move(bg)),
      lastTimeOut(c->getRealMillis()), clock(c), delay(d) {
  }

  virtual bool onMouseMove(Vec2 pos, Vec2 rel) override {
    canRender = pos.inRectangle(getBounds());
    return false;
  }

  virtual void onMouseGone() override {
    canRender = false;
  }

  virtual void render(Renderer& r) override {
    if (canRender) {
      if (clock->getRealMillis() > lastTimeOut + delay) {
        Vec2 size(0, text.size() * tooltipLineHeight + 2 * tooltipVMargin);
        for (const string& t : text)
          size.x = max(size.x, r.getTextLength(t) + 2 * tooltipHMargin);
        Vec2 pos = getBounds().bottomLeft() + tooltipOffset;
        pos.x = min(pos.x, r.getSize().x - size.x);
        pos.y = min(pos.y, r.getSize().y - size.y);
        r.setTopLayer();
        background->setBounds(Rectangle(pos, pos + size));
        background->render(r);
        for (int i : All(text))
          r.drawText(Color::WHITE, pos + Vec2(tooltipHMargin, tooltipVMargin + i * tooltipLineHeight), text[i]);
        r.popLayer();
      }
    } else 
      lastTimeOut = clock->getRealMillis();
  }

  private:
  bool canRender = false;
  vector<string> text;
  SGuiElem background;
  milliseconds lastTimeOut;
  Clock* clock;
  milliseconds delay;
};

SGuiElem GuiFactory::tooltip(const vector<string>& v, milliseconds delayMilli) {
  if (v.empty() || (v.size() == 1 && v[0].empty()))
    return empty();
  return SGuiElem(new Tooltip(v, stack(background(background1), miniBorder()), clock, delayMilli));
}
namespace {
class ScrollArea : public GuiElem {
  public:
  ScrollArea(SGuiElem c) : content(c) {
  }

  virtual void onRefreshBounds() override {
    if (!scrollPos)
      scrollPos = (Vec2(*content->getPreferredWidth(), *content->getPreferredHeight()) - getBounds().getSize()) / 2;
    content->setBounds(getBounds().translate(-*scrollPos));
  }

  virtual void render(Renderer& r) override {
    r.setScissor(getBounds());
    onRefreshBounds();
    content->renderPart(r, getBounds());
    r.setScissor(none);
  }

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    if (b == MouseButtonId::RELEASED)
      clickPos = none;
    else if (v.inRectangle(getBounds()) && b == MouseButtonId::RIGHT) {
      clickPos = *scrollPos + v;
      return true;
    } else {
      if (v.y >= getBounds().top() && v.y < getBounds().bottom())
        return content->onClick(b, v);
    }
    return false;
  }

  virtual void onMouseGone() override {
//    clickPos = none;
    content->onMouseGone();
  }

  virtual bool onMouseMove(Vec2 v, Vec2 rel) override {
    if (clickPos) {
      scrollPos = *clickPos - v;
      scrollPos->x = max(0, min(*content->getPreferredWidth() - getBounds().width(), scrollPos->x));
      scrollPos->y = max(0, min(*content->getPreferredHeight() - getBounds().height(), scrollPos->y));
      return true;
    } else {
      if (v.inRectangle(getBounds()))
        return content->onMouseMove(v, rel);
      else
        content->onMouseGone();
      return false;
    }
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    return content->onKeyPressed2(key);
  }

  virtual bool onTextInput(const char* c) override {
    return content->onTextInput(c);
  }

  private:
  SGuiElem content;
  optional<Vec2> scrollPos;
  optional<Vec2> clickPos;
};
}

SGuiElem GuiFactory::scrollArea(SGuiElem elem) {
  return make_shared<ScrollArea>(std::move(elem));
}

const static int notHeld = -1000;

int GuiFactory::getHeldInitValue() {
  return notHeld;
}

class ScrollBar : public GuiLayout {
  public:

  ScrollBar(SGuiElem b, SGuiElem _content, Vec2 butSize, int vMarg, ScrollPosition* scrollP, int* h,
      Clock* c)
      : GuiLayout(std::move(b)), buttonSize(butSize), vMargin(vMarg), scrollPos(scrollP), content(_content), clock(c) {
    if (h)
      held = h;
    else
      held = &localHeld;
  }

  virtual Rectangle getElemBounds(int num) override {
    Vec2 center(getBounds().middle().x, calcButHeight());
    return Rectangle(center - buttonSize / 2, center + buttonSize / 2);
  }

  virtual void render(Renderer& r) override {
    onRefreshBounds();
    GuiLayout::render(r);
  }

  double calcPos(int mouseHeight) {
    return max(0.0, min(1.0, 
          double(mouseHeight - getBounds().top() - vMargin - buttonSize.y / 2)
              / (getBounds().height() - 2 * vMargin - buttonSize.y)));
  }

  int scrollLength() {
    CHECK(*content->getPreferredHeight() > getBounds().height());
    return max(0, *content->getPreferredHeight() - getBounds().height());
  }

  int calcButHeight() {
    int ret = min<double>(getBounds().bottom() - buttonSize.y / 2 - vMargin,
        double(getBounds().top()) + buttonSize.y / 2 + vMargin + getScrollPos()
            * double(getBounds().height() - buttonSize.y - 2 * vMargin)
            / double(scrollLength()));
    return ret;
  }

  const int wheelScrollUnit = 100;

  double getScrollPos() {
    int size = getBounds().height() / 2;
    return scrollPos->get(clock->getRealMillis(), size, scrollLength() + size) - size;
  }

  void addScrollPos(double v) {
    scrollPos->add(v, clock->getRealMillis());
    scrollPos->setBounds(getBounds().height() / 2, scrollLength() + getBounds().height() / 2);
  }

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    if (b == MouseButtonId::RELEASED)
      *held = notHeld;
    else if (b == MouseButtonId::LEFT) {
      if (v.inRectangle(getElemBounds(0))) {
        *held = v.y - calcButHeight();
        return true;
      } else
      if (v.inRectangle(getBounds())) {
        if (v.y <= getBounds().top() + vMargin)
          addScrollPos(- wheelScrollUnit);
        else if (v.y >= getBounds().bottom() - vMargin)
          addScrollPos(wheelScrollUnit);
        else
          scrollPos->set(getBounds().height() / 2 + scrollLength() * calcPos(v.y), clock->getRealMillis());
        return true;
      }
    } else
    if (v.inRectangle(Rectangle(Vec2(content->getBounds().left(), getBounds().top()), getBounds().bottomRight()))) {
      if (b == MouseButtonId::WHEEL_UP)
        addScrollPos(- wheelScrollUnit);
      else if (b == MouseButtonId::WHEEL_DOWN)
        addScrollPos(wheelScrollUnit);
      return true;
    }
    return false;
  }

  virtual bool onKeyPressed2(SDL::SDL_Keysym key) override {
    if (key.sym == C_MENU_SCROLL_UP) {
      addScrollPos(- wheelScrollUnit);
      return true;
    }
    if (key.sym == C_MENU_SCROLL_DOWN) {
      addScrollPos(wheelScrollUnit);
      return true;
    }
    return false;
  }

  virtual bool onMouseMove(Vec2 v, Vec2 rel) override {
    if (*held != notHeld)
      scrollPos->reset(getBounds().height() / 2 + scrollLength() * calcPos(v.y - *held));
    return false;
  }

  virtual bool isVisible(int num) override {
    return getBounds().height() < *content->getPreferredHeight();
  }

  private:
  int* held;
  int localHeld = notHeld;
  Vec2 buttonSize;
  int vMargin;
  ScrollPosition* scrollPos;
  SGuiElem content;
  Clock* clock;
};

class Scrollable : public GuiElem {
  public:
  Scrollable(SGuiElem c, ScrollPosition* scrollP, Clock* cl) : content(c), scrollPos(scrollP), clock(cl) {
  }

  double getScrollPos() {
    int size = getBounds().height() / 2;
    return scrollPos->get(clock->getRealMillis(), size, *content->getPreferredHeight() - size);
  }

  virtual void onRefreshBounds() override {
    content->setBounds(getBounds().translate(Vec2(0, -getScrollPos() + getBounds().height() / 2)));
  }

  virtual void render(Renderer& r) override {
    Rectangle visible(0, getBounds().top(), r.getSize().x, getBounds().bottom());
    r.setScissor(visible);
    onRefreshBounds();
    content->renderPart(r, visible);
    r.setScissor(none);
  }

  virtual bool onClick(MouseButtonId b, Vec2 v) override {
    if (v.y >= getBounds().top() && v.y < getBounds().bottom())
      return content->onClick(b, v);
    return false;
  }

  virtual void onMouseGone() override {
    content->onMouseGone();
  }

  virtual bool onMouseMove(Vec2 v, Vec2 rel) override {
    if (v.inRectangle(getBounds()))
      return content->onMouseMove(v, rel);
    else
      content->onMouseGone();
    return false;
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    return content->onKeyPressed2(key);
  }

  virtual bool onTextInput(const char* c) override {
    return content->onTextInput(c);
  }

  private:
  SGuiElem content;
  ScrollPosition* scrollPos;
  Clock* clock;
};

class Slider : public GuiLayout {
  public:

  Slider(SGuiElem button, shared_ptr<int> position, int maxValue)
      : GuiLayout(std::move(button)), position(std::move(position)), maxValue(maxValue),
        buttonSize(*elems[0]->getPreferredWidth(), *elems[0]->getPreferredHeight()) {}

  virtual Rectangle getElemBounds(int num) override {
    Vec2 center(calcButHeight(), getBounds().middle().y);
    return Rectangle(center - buttonSize / 2, center + buttonSize / 2);
  }

  virtual void render(Renderer& r) override {
    onRefreshBounds();
    GuiLayout::render(r);
  }

  double calcPos(int mouseHeight) {
    return max(0.0, min(1.0,
          double(mouseHeight - getBounds().left()) / (getBounds().height())));
  }

  int scrollLength() {
    return getBounds().width();
  }

  int calcButHeight() {
    auto bounds = getBounds();
    return (int)(bounds.left() + getScrollPos() * bounds.width());
  }

  double getScrollPos() {
    return double(*position) / double(maxValue);
  }

  void setPosition(int pos) {
    *position = min(maxValue, max(0, pos));
  }

  void addScrollPos(int amount) {
    setPosition(*position + amount);
  }

  void setPositionFromClick(Vec2 v) {
    setPosition((int) round((double)(v.x - getBounds().left()) * maxValue / scrollLength()));
  }

  virtual bool onClick(MouseButtonId id, Vec2 v) override {
    if (id == MouseButtonId::RELEASED)
      held = false;
    else if (v.inRectangle(getBounds())) {
      if (id == MouseButtonId::WHEEL_UP)
        addScrollPos(-1);
      else if (id == MouseButtonId::WHEEL_DOWN)
        addScrollPos(1);
      else if (id == MouseButtonId::LEFT) {
        held = true;
        setPositionFromClick(v);
      }
      return true;
    }
    return false;
  }

  virtual bool onMouseMove(Vec2 v, Vec2 rel) override {
    if (held)
      setPositionFromClick(v);
    return false;
  }

  virtual bool isVisible(int) override {
    return true;
  }

  private:
  SGuiElem button;
  shared_ptr<int> position;
  int maxValue;
  Vec2 buttonSize;
  bool held = false;
};

SGuiElem GuiFactory::slider(SGuiElem button, shared_ptr<int> position, int max) {
  return make_shared<Slider>(std::move(button), std::move(position), max);
}

Texture& GuiFactory::get(TexId id) {
  return *textures[id];
}

void GuiFactory::loadImages() {
  iconTextures.clear();
  textures[TexId::SCROLLBAR] = Texture(imagesPath.file("ui/scrollbar.png"));
  textures[TexId::SCROLL_BUTTON] = Texture(imagesPath.file("ui/scrollmark.png"));
  textures[TexId::BACKGROUND_PATTERN] = Texture(imagesPath.file("window_bg.png"));
  text = Color::WHITE;
  titleText = Color::YELLOW;
  inactiveText = Color::LIGHT_GRAY;
  textures[TexId::HORI_CORNER1] = Texture(imagesPath.file("ui/horicorner1.png"));
  textures[TexId::HORI_CORNER2] = Texture(imagesPath.file("ui/horicorner2.png"));
  textures[TexId::HORI_LINE] = Texture(imagesPath.file("ui/horiline.png"));
  textures[TexId::HORI_MIDDLE] = Texture(imagesPath.file("ui/horimiddle.png"));
  textures[TexId::VERT_BAR] = Texture(imagesPath.file("ui/vertbar.png"));
  textures[TexId::HORI_BAR] = Texture(imagesPath.file("ui/horibar.png"));
  textures[TexId::CORNER_TOP_LEFT] = Texture(imagesPath.file("ui/cornerTOPL.png"));
  textures[TexId::CORNER_TOP_RIGHT] = Texture(imagesPath.file("ui/cornerTOPR.png"));
  textures[TexId::CORNER_BOTTOM_RIGHT] = Texture(imagesPath.file("ui/cornerBOTTOMR.png"));

  textures[TexId::HORI_BAR_MINI] = Texture(imagesPath.file("ui/horibarmini.png"));
  textures[TexId::VERT_BAR_MINI] = Texture(imagesPath.file("ui/vertbarmini.png"));
  textures[TexId::CORNER_MINI] = Texture(imagesPath.file("ui/cornermini.png"));

  textures[TexId::BUTTON_BG] = Texture(imagesPath.file("ui/button_bg.png"));
  textures[TexId::BUTTON_CORNER] = Texture(imagesPath.file("ui/button_corner.png"));
  textures[TexId::BUTTON_BOTTOM] = Texture(imagesPath.file("ui/button_corner.png"), 7, 4, 1, 4);
  textures[TexId::BUTTON_SIDE] = Texture(imagesPath.file("ui/button_corner.png"), 0, 0, 4, 1);
  textures[TexId::BUTTON_CORNER_HIGHLIGHT] = Texture(imagesPath.file("ui/button_corner_highlight.png"));
  textures[TexId::BUTTON_BOTTOM_HIGHLIGHT] = Texture(imagesPath.file("ui/button_corner_highlight.png"), 7, 4, 1, 4);
  textures[TexId::BUTTON_SIDE_HIGHLIGHT] = Texture(imagesPath.file("ui/button_corner_highlight.png"), 0, 0, 4, 1);

  textures[TexId::HORI_BAR_MINI2] = Texture(imagesPath.file("ui/horibarmini2.png"));
  textures[TexId::VERT_BAR_MINI2] = Texture(imagesPath.file("ui/vertbarmini2.png"));
  textures[TexId::CORNER_MINI2] = Texture(imagesPath.file("ui/cornermini2.png"));
  textures[TexId::CORNER_MINI2_LARGE] = Texture(imagesPath.file("ui/cornermini2_large.png"));
  textures[TexId::IMMIGRANT_BG] = Texture(imagesPath.file("ui/immigrantbg.png"));
  textures[TexId::IMMIGRANT2_BG] = Texture(imagesPath.file("ui/immigrant2bg.png"));
  textures[TexId::SCROLL_UP] = Texture(imagesPath.file("ui/up.png"));
  textures[TexId::SCROLL_DOWN] = Texture(imagesPath.file("ui/down.png"));
  textures[TexId::WINDOW_CORNER] = Texture(imagesPath.file("ui/corner1.png"));
  textures[TexId::WINDOW_CORNER_EXIT] = Texture(imagesPath.file("ui/corner2X.png"));
  textures[TexId::WINDOW_CORNER_EXIT_HIGHLIGHT] = Texture(imagesPath.file("ui/corner2X_highlight.png"));
  textures[TexId::WINDOW_VERT_BAR] = Texture(imagesPath.file("ui/vertibarmsg1.png"));
  textures[TexId::MAIN_MENU_HIGHLIGHT] = Texture(imagesPath.file("ui/menu_highlight.png"));
  textures[TexId::SPLASH1] = Texture(imagesPath.file("splash2f.png"));
  textures[TexId::SPLASH2] = Texture(imagesPath.file("splash2e.png"));
  textures[TexId::LOADING_SPLASH] = Texture(imagesPath.file(Random.choose(
            "splash2a.png"_s,
            "splash2b.png"_s,
            "splash2c.png"_s,
            "splash2d.png"_s)));
  textures[TexId::UI_HIGHLIGHT] = Texture(imagesPath.file("ui/ui_highlight.png"));
  textures[TexId::MINIMAP_BAR] = Texture(imagesPath.file("minimap_bar.png"));
  const int tabIconWidth = 42;
  for (int i = 0; i < 8; ++i)
    iconTextures.push_back(Texture(imagesPath.file("icons.png"), 0, i * tabIconWidth, tabIconWidth, tabIconWidth));
  auto loadIcons = [&] (int width, int count, const char* file) {
    for (int i = 0; i < count; ++i)
      iconTextures.push_back(Texture(imagesPath.file(file), 0, i * width, width, width));
  };
  loadIcons(16, 4, "morale_icons.png");
  loadIcons(16, 2, "team_icons.png");
  loadIcons(48, 6, "minimap_icons.png");
  loadIcons(32, 1, "expand_up.png");
  loadIcons(32, 1, "special_immigrant.png");
  textures[TexId::MENU_ITEM] = Texture(imagesPath.file("barmid.png"));
  // If menu_core fails to load, try the lower resolution versions
  if (auto tex = Texture::loadMaybe(imagesPath.file("menu_core.png"))) {
    textures[TexId::MENU_CORE] = std::move(*tex);
  } else {
    textures[TexId::MENU_CORE] = Texture(imagesPath.file("menu_core_sm.png"));
  }
}

SGuiElem GuiFactory::getScrollbar() {
  return stack(makeVec(
        sprite(get(TexId::SCROLLBAR), GuiFactory::Alignment::VERTICAL_CENTER),
        sprite(get(TexId::SCROLL_UP), GuiFactory::Alignment::TOP_RIGHT),
        sprite(get(TexId::SCROLL_DOWN), GuiFactory::Alignment::BOTTOM_RIGHT)));
}

SGuiElem GuiFactory::getScrollButton() {
  return sprite(get(TexId::SCROLL_BUTTON), Alignment::CENTER);
}

Vec2 GuiFactory::getScrollButtonSize() {
  return Vec2(get(TexId::SCROLL_BUTTON).getSize().x, get(TexId::SCROLL_BUTTON).getSize().x);
}

namespace {
class Conditional : public GuiStack {
  public:
  Conditional(vector<SGuiElem> e, function<int(GuiElem*)> f) : GuiStack(std::move(e)), cond(f) {}
  Conditional(SGuiElem e, function<bool(GuiElem*)> f) : GuiStack(makeVec(std::move(e))),
      cond([f = std::move(f)] (GuiElem* e) { return f(e) ? 0 : -1; }) {}

  virtual bool isVisible(int num) override {
    return cond(this) == num;
  }

  protected:
  function<int(GuiElem*)> cond;
};
}

SGuiElem GuiFactory::conditional(SGuiElem elem, function<bool()> f) {
  return SGuiElem(new Conditional(std::move(elem), [f](GuiElem*) { return f(); }));
}

SGuiElem GuiFactory::conditional(function<int()> f, vector<SGuiElem> elem) {
  return SGuiElem(new Conditional(std::move(elem), [f](GuiElem*) { return f(); }));
}

class ConditionalStopKeys : public Conditional {
  public:
  using Conditional::Conditional;

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (cond(this) == 0)
      return Conditional::onKeyPressed2(key);
    else
      return false;
  }
};

SGuiElem GuiFactory::conditionalStopKeys(SGuiElem elem, function<bool()> f) {
  return SGuiElem(new ConditionalStopKeys(std::move(elem), [f](GuiElem*) { return f(); }));
}

SGuiElem GuiFactory::conditional2(SGuiElem elem, function<bool(GuiElem*)> f) {
  return SGuiElem(new Conditional(std::move(elem), f));
}

SGuiElem GuiFactory::conditional2(SGuiElem elem, SGuiElem alter, function<bool(GuiElem*)> f) {
  return SGuiElem(new Conditional(makeVec(std::move(elem), std::move(alter)), [f](GuiElem* e){ return f(e) ? 0 : 1; }));
}

SGuiElem GuiFactory::conditional(SGuiElem elem, SGuiElem alter, function<bool()> f) {
  return conditional2(std::move(elem), std::move(alter), [=] (GuiElem*) { return f(); });
}

class GuiContainScrollPos : public GuiElem {
  public:
  ScrollPosition pos;
};

const int scrollbarWidth = 22;

SGuiElem GuiFactory::scrollable(SGuiElem content, ScrollPosition* scrollPos, int* held) {
  if (!scrollPos) {
    auto cont = new GuiContainScrollPos();
    scrollPos = &cont->pos;
    content = stack(SGuiElem(cont), std::move(content));
  }
  SGuiElem scrollable(new Scrollable(content, scrollPos, clock));
  int scrollBarMargin = get(TexId::SCROLL_UP).getSize().y;
  SGuiElem bar(new ScrollBar(
        getScrollButton(), content, getScrollButtonSize(), scrollBarMargin, scrollPos, held, clock));
  SGuiElem barButtons = getScrollbar();
  barButtons = conditional2(std::move(barButtons), [=] (GuiElem* e) {
      return e->getBounds().height() < *content->getPreferredHeight();});
  return setHeight(*content->getPreferredHeight(), maybeMargin(stack(std::move(barButtons), std::move(bar)),
        std::move(scrollable), scrollbarWidth, RIGHT,
        [=] (Rectangle bounds) { return bounds.height() < *content->getPreferredHeight(); }));
}

SGuiElem GuiFactory::background(Color c) {
  return stack(rectangle(c), repeatedPattern(get(TexId::BACKGROUND_PATTERN)));
}

SGuiElem GuiFactory::highlight(double height) {
  return margins(sprite(get(TexId::MAIN_MENU_HIGHLIGHT), height), -25, 0, 0, 0);
}

SGuiElem GuiFactory::highlightDouble() {
  return stack(
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::LEFT_CENTER),
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::RIGHT_CENTER, false, true));
}

SGuiElem GuiFactory::mainMenuHighlight() {
  return stack(
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::LEFT_STRETCHED),
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::RIGHT_STRETCHED, false, true));
}

SGuiElem GuiFactory::miniBorder() {
  return stack(makeVec(
        sprite(get(TexId::HORI_BAR_MINI), Alignment::BOTTOM, true, false),
        sprite(get(TexId::HORI_BAR_MINI), Alignment::TOP, false, false),
        sprite(get(TexId::VERT_BAR_MINI), Alignment::RIGHT, false, true),
        sprite(get(TexId::VERT_BAR_MINI), Alignment::LEFT, false, false),
        sprite(get(TexId::CORNER_MINI), Alignment::BOTTOM_RIGHT, true, true),
        sprite(get(TexId::CORNER_MINI), Alignment::BOTTOM_LEFT, true, false),
        sprite(get(TexId::CORNER_MINI), Alignment::TOP_RIGHT, false, true),
        sprite(get(TexId::CORNER_MINI), Alignment::TOP_LEFT, false, false)));
}

SGuiElem GuiFactory::miniBorder2() {
  return stack(makeVec(
        sprite(get(TexId::HORI_BAR_MINI2), Alignment::BOTTOM, false, false),
        sprite(get(TexId::HORI_BAR_MINI2), Alignment::TOP, true, false),
        sprite(get(TexId::VERT_BAR_MINI2), Alignment::RIGHT, false, false),
        sprite(get(TexId::VERT_BAR_MINI2), Alignment::LEFT, false, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::BOTTOM_RIGHT, true, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::BOTTOM_LEFT, true, false),
        sprite(get(TexId::CORNER_MINI2), Alignment::TOP_RIGHT, false, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::TOP_LEFT, false, false)));
}

SGuiElem GuiFactory::miniWindow2(SGuiElem content, function<void()> onExitButton, bool captureExitClick) {
  return stack(fullScreen(darken()),
      miniWindow(margins(std::move(content), 15), onExitButton, captureExitClick));
}

SGuiElem GuiFactory::miniWindow(SGuiElem content, function<void()> onExitButton, bool captureExitClick) {
  auto ret = makeVec(
        stopMouseMovement(),
        rectangle(Color::BLACK),
        background(background1));
  if (onExitButton)
    ret.push_back(reverseButton(onExitButton, {getKey(SDL::SDLK_ESCAPE), getKey(C_MENU_CANCEL)}, captureExitClick));
  append(ret, {
        margins(std::move(content), 1),
        miniBorder()
  });
  return stack(std::move(ret));
}

SGuiElem GuiFactory::miniWindow() {
  return stack(makeVec(
        stopMouseMovement(),
        rectangle(Color::BLACK),
        background(background1),
        miniBorder()));
}

SGuiElem GuiFactory::window(SGuiElem content, function<void()> onExitButton) {
  return stack(makeVec(
        fullScreen(stopMouseMovement()),
        alignment(Alignment::TOP_RIGHT, button(onExitButton, getKey(SDL::SDLK_ESCAPE), true), Vec2(38, 38)),
        rectangle(Color::BLACK),
        background(background1),
        margins(std::move(content), 20, 35, 20, 30),
        sprite(get(TexId::HORI_BAR), Alignment::BOTTOM, true, false),
        sprite(get(TexId::HORI_BAR), Alignment::TOP, false, false),
        sprite(get(TexId::WINDOW_VERT_BAR), Alignment::RIGHT, false, false),
        sprite(get(TexId::WINDOW_VERT_BAR), Alignment::LEFT, false, true),
        sprite(get(TexId::WINDOW_CORNER), Alignment::BOTTOM_RIGHT, true, true, Vec2(6, 2)),
        sprite(get(TexId::WINDOW_CORNER), Alignment::BOTTOM_LEFT, true, false, Vec2(-6, 2)),
        sprite(get(TexId::WINDOW_CORNER_EXIT), Alignment::TOP_RIGHT, false, false, Vec2(6, -2)),
        alignment(Alignment::TOP_RIGHT, mouseHighlight2(sprite(get(TexId::WINDOW_CORNER_EXIT_HIGHLIGHT), Alignment::TOP_RIGHT, false, false, Vec2(6, -2))), Vec2(47, 41)),
        sprite(get(TexId::WINDOW_CORNER), Alignment::TOP_LEFT, false, false, Vec2(-6, -2))
        ));
}

SGuiElem GuiFactory::mainDecoration(int rightBarWidth, int bottomBarHeight, optional<int> topBarHeight) {
  return margin(
      stack(makeVec(
          background(background1),
          sprite(get(TexId::HORI_BAR), Alignment::TOP),
          sprite(get(TexId::HORI_BAR), Alignment::BOTTOM, true),
          topBarHeight ? margin(sprite(get(TexId::HORI_BAR_MINI), Alignment::BOTTOM), empty(), *topBarHeight, TOP)
                       : empty(),
          sprite(get(TexId::VERT_BAR), Alignment::RIGHT, false, true),
          sprite(get(TexId::VERT_BAR), Alignment::LEFT),
          sprite(get(TexId::CORNER_TOP_LEFT), Alignment::TOP_RIGHT, false, true, Vec2(8, 0)),
          sprite(get(TexId::CORNER_TOP_RIGHT), Alignment::TOP_LEFT, false, true),
          sprite(get(TexId::CORNER_BOTTOM_RIGHT), Alignment::BOTTOM_LEFT, false, true)
      )),
      stack(makeVec(
          margin(background(background1), empty(), bottomBarHeight, BOTTOM),
          sprite(get(TexId::HORI_LINE), Alignment::BOTTOM),
 //         sprite(get(TexId::HORI_MIDDLE), Alignment::BOTTOM_CENTER),
          sprite(get(TexId::HORI_CORNER1), Alignment::BOTTOM_RIGHT, false, true),
          sprite(get(TexId::HORI_CORNER2), Alignment::BOTTOM_LEFT, false, true, Vec2(-93, 0)))),
      rightBarWidth,
      LEFT);
}

SGuiElem GuiFactory::translucentBackground(SGuiElem content) {
  return stack(
      stopMouseMovement(),
      background(std::move(content), translucentBgColor));
}

SGuiElem GuiFactory::translucentBackgroundPassMouse(SGuiElem content) {
  return background(std::move(content), translucentBgColor);
}

SGuiElem GuiFactory::translucentBackgroundWithBorderPassMouse(SGuiElem content) {
  return stack(
      rectangleBorder(Color::GRAY),
      background(std::move(content), translucentBgColor));
}

SGuiElem GuiFactory::translucentBackgroundWithBorder(SGuiElem content) {
  return stack(
      translucentBackground(),
      rectangleBorder(Color::GRAY),
      std::move(content)
  );
}

SGuiElem GuiFactory::translucentBackground() {
  return stack(
      stopMouseMovement(),
        rectangle(translucentBgColor));
}

namespace {

constexpr int lineHeight = 20;

class TextInputElem : public GuiElem {
  public:
  TextInputElem(int width, int maxLines, shared_ptr<string> text)
      : width(width), maxLines(maxLines), text(std::move(text)) {
    SDL::SDL_StartTextInput();
  }

  ~TextInputElem() {
    SDL::SDL_StopTextInput();
  }


  void render(Renderer& r) override {
    auto lines = breakText(r, *text, width);
    while (lines.size() > maxLines) {
      text->pop_back();
      lines = breakText(r, *text, width);
    }
    Vec2 origin = getBounds().topLeft();
    Vec2 cursorPos;
    for (auto& line : lines) {
      r.drawText(Color::WHITE, origin, line);
      cursorPos = origin + Vec2(r.getTextLength(line), 0);
      origin += Vec2(0, lineHeight);
    }
    r.drawFilledRectangle(Rectangle(cursorPos, cursorPos + Vec2(3, lineHeight)), Color::WHITE);
  }

  virtual optional<int> getPreferredWidth() override {
    return width;
  }

  virtual optional<int> getPreferredHeight() override {
    return maxLines * lineHeight;
  }

  virtual bool onTextInput(const char* c) override {
    *text += c;
    return true;
  }

  virtual bool onKeyPressed2(SDL::SDL_Keysym key) override {
    if (key.sym == SDL::SDLK_BACKSPACE) {
      if (!text->empty())
      text->pop_back();
      return true;
    } else
      return false;
  }

  int width;
  int maxLines;
  shared_ptr<string> text;
};

}

SGuiElem GuiFactory::textInput(int width, int maxLines, shared_ptr<string> text) {
  return make_shared<TextInputElem>(width, maxLines, text);
}

SGuiElem GuiFactory::minimapBar(SGuiElem icon1, SGuiElem icon2) {
  auto& tex = textures[TexId::MINIMAP_BAR];
  return preferredSize(tex->getSize(),
      stack(
          stopMouseMovement(),
          sprite(*tex, Alignment::CENTER, Color::WHITE),
          translate(std::move(icon1), Vec2(21, 0)),
          translate(std::move(icon2), Vec2(83, 0))
      ));
}

SGuiElem GuiFactory::background(SGuiElem content, Color color) {
  return stack(rectangle(color), std::move(content));
}

SGuiElem GuiFactory::icon(IconId id, Alignment alignment, Color color) {
  return sprite(iconTextures[(int) id], alignment, color);
}

static int trans1 = 1094;
static int trans2 = 1693;

SGuiElem GuiFactory::uiHighlightMouseOver(Color c) {
  return mouseHighlight2(uiHighlightLine(c));
}

SGuiElem GuiFactory::uiHighlight(Color c) {
  return rectangle(c.transparency(trans1), c.transparency(trans2));
}

SGuiElem GuiFactory::uiHighlightLine(Color c) {
  return margins(uiHighlight(c), -8, -3, -8, 3);
  //return leftMargin(-8, topMargin(-4, sprite(TexId::UI_HIGHLIGHT, Alignment::LEFT_STRETCHED, c)));
}

SGuiElem GuiFactory::blink(SGuiElem elem) {
  return conditional(std::move(elem), [this]() { return blinkingState(clock->getRealMillis(), 2, 4) < 0.5; });
}

SGuiElem GuiFactory::blink(SGuiElem elem, SGuiElem elem2) {
  return conditional2(std::move(elem), std::move(elem2),
      [this](GuiElem*) { return blinkingState(clock->getRealMillis(), 2, 4) < 0.5; });
}

SGuiElem GuiFactory::tutorialHighlight() {
  return blink(uiHighlightLine(Color::YELLOW));
}

SGuiElem GuiFactory::uiHighlightConditional(function<bool()> cond, Color c) {
  return conditional(uiHighlightLine(c), cond);
}

SGuiElem GuiFactory::rectangleBorder(Color col) {
  return rectangle(Color::TRANSPARENT, col);
}

SGuiElem GuiFactory::sprite(TexId id, Alignment a, optional<Color> c) {
  return sprite(get(id), a, false, false, Vec2(0, 0), c);
}

SGuiElem GuiFactory::sprite(Texture& t, Alignment a, Color c) {
  return sprite(t, a, false, false, Vec2(0, 0), c);
}

SGuiElem GuiFactory::mainMenuLabelBg(const string& s, double vPadding, Color color) {
  return stack(
      marginFit(empty(), sprite(get(TexId::MENU_ITEM), Alignment::CENTER_STRETCHED),
          0.12, BOTTOM),
      mainMenuLabel(s, vPadding, color));
}

SGuiElem GuiFactory::darken() {
  return stack(
      stopMouseMovement(),
      rectangle(Color{0, 0, 0, 150}));
}

void GuiFactory::propagateEvent(const Event& event, vector<SGuiElem> guiElems) {
  switch (event.type) {
    case SDL::SDL_MOUSEBUTTONUP:
      for (auto elem : guiElems)
        if (elem->onClick(MouseButtonId::RELEASED, Vec2(event.button.x, event.button.y)))
          break;
      dragContainer.pop();
      break;
    case SDL::SDL_MOUSEMOTION: {
      bool captured = false;
      for (auto elem : guiElems)
        if (!captured)
          captured |= elem->onMouseMove(Vec2(event.motion.x, event.motion.y),
              Vec2(event.motion.xrel, event.motion.yrel));
        else
          elem->onMouseGone();
      break;}
    case SDL::SDL_MOUSEBUTTONDOWN: {
      Vec2 clickPos(event.button.x, event.button.y);
      for (auto elem : guiElems) {
        if (event.button.button == SDL_BUTTON_RIGHT && elem->onClick(MouseButtonId::RIGHT, clickPos))
          break;
        if (event.button.button == SDL_BUTTON_LEFT && elem->onClick(MouseButtonId::LEFT, clickPos))
          break;
        if (event.button.button == SDL_BUTTON_MIDDLE && elem->onClick(MouseButtonId::MIDDLE, clickPos))
          break;
      }
      break;
    }
    case SDL::SDL_TEXTINPUT:
      for (auto elem : guiElems)
        if (elem->onTextInput(event.text.text))
          break;
      break;
    case SDL::SDL_KEYDOWN:
      for (auto elem : guiElems)
        if (elem->onKeyPressed2(event.key.keysym))
          break;
      break;
    case SDL::SDL_MOUSEWHEEL:
      for (auto elem : guiElems)
        if (elem->onClick(event.wheel.y > 0 ? MouseButtonId::WHEEL_UP : MouseButtonId::WHEEL_DOWN, renderer.getMousePos()))
          break;
      break;
    default: break;
  }
  for (auto elem : guiElems)
    elem->onRefreshBounds();
}
