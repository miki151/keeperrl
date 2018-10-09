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
#include "view_object.h"
#include "tile.h"
#include "clock.h"
#include "spell.h"
#include "options.h"
#include "scroll_position.h"
#include "keybinding_map.h"
#include "player_role_choice.h"
#include "attr_type.h"

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

GuiElem::~GuiElem() {
}

class ButtonElem : public GuiElem {
  public:
  ButtonElem(function<void(Rectangle, Vec2)> f) : fun(f) {}

  virtual bool onLeftClick(Vec2 pos) override {
    auto bounds = getBounds();
    if (pos.inRectangle(bounds)) {
      fun(bounds, pos - bounds.topLeft());
      return true;
    }
     return false;
  }

  protected:
  function<void(Rectangle, Vec2)> fun;
};

class ReleaseButton : public GuiElem {
  public:
  ReleaseButton(function<void()> f, int but) : fun(f), button(but) {}

  virtual void onMouseRelease(Vec2 pos) override {
    if (clicked && pos.inRectangle(getBounds())) {
      fun();
    }
    clicked = false;
  }

  virtual bool onLeftClick(Vec2 pos) override {
    if (button == 0 && pos.inRectangle(getBounds())) {
      clicked = true;
      return true;
    } else
      return false;
  }

  virtual bool onRightClick(Vec2 pos) override {
    if (button == 1 && pos.inRectangle(getBounds())) {
      clicked = true;
      return true;
    } else
      return false;
  }

  virtual bool onMouseMove(Vec2 pos) override {
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
  ButtonKey(function<void(Rectangle)> f, SDL_Keysym key, bool cap) : ButtonElem([f](Rectangle b, Vec2) { f(b);}),
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

static optional<SDL_Keycode> getKey(char c) {
  switch (c) {
    case 'a': return SDL::SDLK_a;
    case 'b': return SDL::SDLK_b;
    case 'c': return SDL::SDLK_c;
    case 'd': return SDL::SDLK_d;
    case 'e': return SDL::SDLK_e;
    case 'f': return SDL::SDLK_f;
    case 'g': return SDL::SDLK_g;
    case 'h': return SDL::SDLK_h;
    case 'i': return SDL::SDLK_i;
    case 'j': return SDL::SDLK_j;
    case 'k': return SDL::SDLK_k;
    case 'l': return SDL::SDLK_l;
    case 'm': return SDL::SDLK_m;
    case 'n': return SDL::SDLK_n;
    case 'o': return SDL::SDLK_o;
    case 'p': return SDL::SDLK_p;
    case 'q': return SDL::SDLK_q;
    case 'r': return SDL::SDLK_r;
    case 's': return SDL::SDLK_s;
    case 't': return SDL::SDLK_t;
    case 'u': return SDL::SDLK_u;
    case 'v': return SDL::SDLK_v;
    case 'w': return SDL::SDLK_w;
    case 'x': return SDL::SDLK_x;
    case 'y': return SDL::SDLK_y;
    case 'z': return SDL::SDLK_z;
    case ' ': return SDL::SDLK_SPACE;
    default: return none;
  }
  return none;
}

GuiFactory::GuiFactory(Renderer& r, Clock* c, Options* o, KeybindingMap* k,
    const DirectoryPath& freeImages, const optional<DirectoryPath>& nonFreeImages)
    : clock(c), renderer(r), options(o), keybindingMap(k), freeImagesPath(freeImages),
      nonFreeImagesPath(nonFreeImages) {
}

DragContainer& GuiFactory::getDragContainer() {
  return dragContainer;
}

SGuiElem GuiFactory::buttonRect(function<void(Rectangle)> fun, SDL_Keysym hotkey, bool capture) {
  return SGuiElem(new ButtonKey(fun, hotkey, capture));
}

SGuiElem GuiFactory::button(function<void()> fun, SDL_Keysym hotkey, bool capture) {
  return SGuiElem(new ButtonKey([=](Rectangle) { fun(); }, hotkey, capture));
}

SGuiElem GuiFactory::buttonRect(function<void(Rectangle)> fun) {
  return SGuiElem(new ButtonElem([=](Rectangle b, Vec2) {fun(b);}));
}

SGuiElem GuiFactory::button(function<void()> fun) {
  return SGuiElem(new ButtonElem([=](Rectangle, Vec2) { fun(); }));
}

SGuiElem GuiFactory::buttonPos(function<void (Rectangle, Vec2)> fun) {
  return make_shared<ButtonElem>(fun);
}

namespace {
class ButtonRightClick : public GuiElem {
  public:
  ButtonRightClick(function<void(Rectangle)> f) : fun(f) {}

  virtual bool onRightClick(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
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

  virtual bool onLeftClick(Vec2 pos) override {
    if (!pos.inRectangle(getBounds())) {
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

  virtual bool onMouseWheel(Vec2 mousePos, bool up) override {
    if (mousePos.inRectangle(getBounds())) {
      fun(up);
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
  virtual bool onMouseMove(Vec2 pos) override {
    return pos.inRectangle(getBounds());
  }

  virtual bool onRightClick(Vec2 pos) override {
    return pos.inRectangle(getBounds());
  }

  virtual bool onLeftClick(Vec2 pos) override {
    return pos.inRectangle(getBounds());
  }

  virtual bool onMiddleClick(Vec2 pos) override {
    return pos.inRectangle(getBounds());
  }

  virtual bool onMouseWheel(Vec2 pos, bool up) override {
    return pos.inRectangle(getBounds());
  }
};

SGuiElem GuiFactory::stopMouseMovement() {
  return SGuiElem(new StopMouseMovement());
}

class DrawCustom : public GuiElem {
  public:
  DrawCustom(GuiFactory::CustomDrawFun draw, function<int()> width = nullptr) : drawFun(draw), preferredWidth(width) {}

  virtual void render(Renderer& renderer) override {
    drawFun(renderer, getBounds());
  }

  virtual optional<int> getPreferredWidth() override {
    if (preferredWidth)
      return preferredWidth();
    else
      return none;
  }

  private:
  GuiFactory::CustomDrawFun drawFun;
  function<int()> preferredWidth;
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

SGuiElem GuiFactory::sprite(Texture& tex, Alignment align, bool vFlip, bool hFlip, Vec2 offset,
    optional<Color> col) {
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
  auto width = [=] { return renderer.getTextLength(s) + 1; };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          //r.setScissor(bounds);
          r.drawTextWithHotkey(Color::BLACK.transparency(100),
            bounds.topLeft() + Vec2(1, 2), s, 0);
          r.drawTextWithHotkey(c, bounds.topLeft(), s, hotkey);
          //r.setScissor(none);
        }, width));
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

static vector<string> breakText(Renderer& renderer, const string& text, int maxWidth, int size = Renderer::textSize,
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

vector<string> GuiFactory::breakText(const string& text, int maxWidth) {
  return ::breakText(renderer, text, maxWidth);
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
  auto width = [=] { return renderer.getTextLength(s); };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = blinkingColor(c1, c2, clock->getRealMillis());
          r.drawTextWithHotkey(Color::BLACK.transparency(100),
            bounds.topLeft() + Vec2(1, 2), s, hotkey);
          Color c1(c);
          if (r.getMousePos().inRectangle(bounds))
            lighten(c1);
          r.drawTextWithHotkey(c1, bounds.topLeft(), s, hotkey);
        }, width));
}

SGuiElem GuiFactory::label(const string& s, function<Color()> colorFun, char hotkey) {
  auto width = [=] { return renderer.getTextLength(s) + 1; };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          auto color = colorFun();
          r.drawText(Color::BLACK.transparency(min<Uint8>(100, color.a)), bounds.topLeft() + Vec2(1, 2), s);
          r.drawTextWithHotkey(colorFun(), bounds.topLeft(), s, hotkey);
        }, width));
}

SGuiElem GuiFactory::labelFun(function<string()> textFun, function<Color()> colorFun) {
  function<int()> width = [this, textFun] { return renderer.getTextLength(textFun()); };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), textFun());
          r.drawText(colorFun(), bounds.topLeft(), textFun());
        }, width));
}

SGuiElem GuiFactory::labelFun(function<string()> textFun, Color color) {
  auto width = [=] { return renderer.getTextLength(textFun()); };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), textFun());
          r.drawText(color, bounds.topLeft(), textFun());
        }, width));
}

SGuiElem GuiFactory::label(const string& s, int size, Color c) {
  auto width = [=] { return renderer.getTextLength(s, size) + 1; };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(Color::BLACK.transparency(100), bounds.topLeft() + Vec2(1, 2), s, Renderer::NONE, size);
          r.drawText(c, bounds.topLeft(), s, Renderer::NONE, size);
        }, width));
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
  auto width = [=] { return renderer.getTextLength(s, size) + 1; };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Vec2 pos = getTextPos(bounds, center);
          r.drawText(Color::BLACK.transparency(100), pos + Vec2(1, 2), s, center, size);
          r.drawText(c, pos, s, center, size);
        }, width));
}

SGuiElem GuiFactory::centeredLabel(Renderer::CenterType center, const string& s, Color c) {
  return centeredLabel(center, s, Renderer::textSize, c);
}

SGuiElem GuiFactory::variableLabel(function<string()> fun, int lineHeight, int size, Color color) {
  return SGuiElem(new VariableLabel(fun, lineHeight, size, color));
}

SGuiElem GuiFactory::labelUnicode(const string& s, Color color, int size, Renderer::FontId fontId) {
  return labelUnicode(s, [color] { return color; }, size, fontId);
}

SGuiElem GuiFactory::labelUnicode(const string& s, function<Color()> color, int size, Renderer::FontId fontId) {
  auto width = [=] { return renderer.getTextLength(s, size, fontId); };
  return SGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = color();
          if (r.getMousePos().inRectangle(bounds))
            lighten(c);
          r.drawText(fontId, size, c, bounds.topLeft(), s);
  }, width));
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

  virtual bool onLeftClick(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i) && elems[i]->onLeftClick(pos))
        return true;
    return false;
  }

  virtual bool onRightClick(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i) && elems[i]->onRightClick(pos))
          return true;
    return false;
  }

  virtual bool onMiddleClick(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i) && elems[i]->onMiddleClick(pos))
          return true;
    return false;
  }

  virtual bool onMouseMove(Vec2 pos) override {
    bool gone = false;
    for (int i : AllReverse(elems))
      if (isVisible(i)) {
        if (!gone) {
          if (elems[i]->onMouseMove(pos))
            gone = true;
        } else
          elems[i]->onMouseGone();
      }
    return gone;
  }

  virtual void onMouseGone() override {
    for (int i : AllReverse(elems))
      elems[i]->onMouseGone();
  }

  virtual void onMouseRelease(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i))
        elems[i]->onMouseRelease(pos);
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

  virtual bool onMouseWheel(Vec2 v, bool up) override {
    for (int i : AllReverse(elems))
      if (isVisible(i))
        if (elems[i]->onMouseWheel(v, up))
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

  virtual bool onLeftClick(Vec2 v) override {
    return elem->onLeftClick(v);
  }

  virtual bool onRightClick(Vec2 v) override {
    return elem->onRightClick(v); 
  }

  virtual bool onMiddleClick(Vec2 v) override {
    return elem->onMiddleClick(v);
  }

  virtual bool onMouseMove(Vec2 v) override {
    return elem->onMouseMove(v);
  }

  virtual void onMouseGone() override {
    elem->onMouseGone();
  }

  virtual void onMouseRelease(Vec2 v) override {
    elem->onMouseRelease(v);
  }

  virtual void onRefreshBounds() override {
    elem->setBounds(getBounds());
  }

  virtual bool onKeyPressed2(SDL_Keysym ev) override {
    return elem->onKeyPressed2(ev);
  }

  virtual bool onMouseWheel(Vec2 mousePos, bool up) override {
    return elem->onMouseWheel(mousePos, up);
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
  Focusable(SGuiElem content, vector<SDL_Keysym> focus, vector<SDL_Keysym> defocus, bool& foc) :
      GuiStack(makeVec(std::move(content))), focusEvent(focus), defocusEvent(defocus), focused(foc) {}

  virtual bool onLeftClick(Vec2 pos) override {
    if (focused && !pos.inRectangle(getBounds())) {
      focused = false;
      return true;
    }
    return GuiLayout::onLeftClick(pos);
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (!focused)
      for (auto& elem : focusEvent)
        if (GuiFactory::keyEventEqual(elem, key)) {
          focused = true;
          return true;
        }
    if (focused)
      for (auto& elem : defocusEvent)
        if (GuiFactory::keyEventEqual(elem, key)) {
          focused = false;
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
};

SGuiElem GuiFactory::focusable(SGuiElem content, vector<SDL_Keysym> focusEvent,
    vector<SDL_Keysym> defocusEvent, bool& focused) {
  return SGuiElem(new Focusable(std::move(content), focusEvent, defocusEvent, focused));
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

SGuiElem GuiFactory::keyHandler(function<void()> fun, Keybinding keybinding, bool capture) {
  return keyHandler([=] (SDL_Keysym key) { if (keybindingMap->matches(keybinding, key)) fun(); }, capture);
}

class KeyHandler2 : public GuiElem {
  public:
  KeyHandler2(function<void()> f, vector<SDL_Keysym> k, bool cap) : fun(f), key(k), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym k) override {
    for (auto& elem : key)
      if (GuiFactory::keyEventEqual(k, elem)) {
        fun();
        return capture;
      }
    return false;
  }

  private:
  function<void()> fun;
  vector<SDL_Keysym> key;
  bool capture;
};

SGuiElem GuiFactory::keyHandler(function<void()> fun, vector<SDL_Keysym> key, bool capture) {
  return SGuiElem(new KeyHandler2(fun, key, capture));
}

class KeyHandlerChar : public GuiElem {
  public:
  KeyHandlerChar(function<void()> f, char c, bool cap, function<bool()> rAlt) : fun(f), hotkey(c), requireAlt(rAlt),
      capture(cap) {}

  bool isHotkeyEvent(char c, SDL_Keysym key) {
    return requireAlt() == GuiFactory::isAlt(key) &&
      !GuiFactory::isCtrl(key) &&
      ((!GuiFactory::isShift(key) && getKey(c) == key.sym) ||
          (GuiFactory::isShift(key) && getKey(tolower(c)) == key.sym));
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (isHotkeyEvent(hotkey, key)) {
      fun();
      return capture;
    }
    return false;
  }

  private:
  function<void()> fun;
  char hotkey;
  function<bool()> requireAlt;
  bool capture;
};

SGuiElem GuiFactory::keyHandlerChar(function<void ()> fun, char hotkey, bool capture, bool useAltIfWasdOn) {
  return SGuiElem(new KeyHandlerChar(fun, hotkey, capture,
       [=] { return useAltIfWasdOn && options->getBoolValue(OptionId::WASD_SCROLLING); }));
}

SGuiElem GuiFactory::buttonChar(function<void()> fun, char hotkey, bool capture, bool useAltIfWasdOn) {
  return stack(
      SGuiElem(new ButtonElem([=](Rectangle, Vec2) { fun(); })),
      SGuiElem(keyHandlerChar(fun, hotkey, capture, useAltIfWasdOn)));
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
  return addElem(gui.empty(), size);
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
  for (int i : All(sizes))
    if (sizes[i] == -1)
      sizes[i] = *elems[i]->getPreferredHeight();
  return SGuiElem(new VerticalList(std::move(elems), sizes, backElems, middleElem));
}

SGuiElem GuiFactory::ListBuilder::buildHorizontalList() {
  for (int i : All(sizes))
    if (sizes[i] == -1)
      sizes[i] = *elems[i]->getPreferredWidth();
  return SGuiElem(new HorizontalList(std::move(elems), sizes, backElems, middleElem));
}

SGuiElem GuiFactory::ListBuilder::buildHorizontalListFit() {
  SGuiElem ret = gui.horizontalListFit(std::move(elems), 0);
  return ret;
}

SGuiElem GuiFactory::ListBuilder::buildVerticalListFit() {
  SGuiElem ret = gui.verticalListFit(std::move(elems), 0);
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
  ViewObjectGui(ViewId id, Vec2 sz, double sc, Color c) : object(id), size(sz), scale(sc), color(c) {}
  
  virtual void render(Renderer& renderer) override {
    object.match(
          [&](const ViewObject& obj) {
            renderer.drawViewObject(getBounds().topLeft(), obj, true, scale, color);
          },
          [&](ViewId viewId) {
            renderer.drawViewObject(getBounds().topLeft(), viewId, true, scale, color);
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
  variant<ViewObject, ViewId> object;
  Vec2 size;
  double scale;
  Color color;
};

SGuiElem GuiFactory::viewObject(const ViewObject& object, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(object, Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::viewObject(ViewId id, double scale, Color color) {
  return SGuiElem(new ViewObjectGui(id, Vec2(1, 1) * Renderer::nominalSize * scale, scale, color));
}

SGuiElem GuiFactory::asciiBackground(ViewId id) {
  return SGuiElem(
      new DrawCustom([=] (Renderer& renderer, Rectangle bounds) { renderer.drawAsciiBackground(id, bounds);}));
}

class DragSource : public GuiElem {
  public:
  DragSource(DragContainer& c, DragContent d, function<SGuiElem()> g) : container(c), content(d), gui(g) {}

  virtual bool onLeftClick(Vec2 v) override {
    if (v.inRectangle(getBounds()))
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

  virtual void onMouseRelease(Vec2 v) override {
    if (v.inRectangle(getBounds()))
      fun();
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

  virtual bool onMouseMove(Vec2 pos) override {
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
  MouseButtonHeld(SGuiElem elem, int but) : GuiStack(std::move(elem)), button(but) {}

  virtual bool onLeftClick(Vec2 v) override {
    if (button == 0 && v.inRectangle(getBounds()))
      on = true;
    return false;
  }

  virtual bool onRightClick(Vec2 v) override {
    if (button == 1 && v.inRectangle(getBounds()))
      on = true;
    return false;
  }

  virtual void onMouseRelease(Vec2) override {
    on = false;
  }

  virtual void onMouseGone() override {
    on = false;
  }

  virtual bool onMouseMove(Vec2 pos) override {
    if (!pos.inRectangle(getBounds()))
      on = false;
    return false;
  }

  virtual bool isVisible(int num) override {
    return on;
  }

  private:
  const int button;
  bool on = false;
};

SGuiElem GuiFactory::onMouseLeftButtonHeld(SGuiElem elem) {
  return SGuiElem(new MouseButtonHeld(std::move(elem), 0));
}

SGuiElem GuiFactory::onMouseRightButtonHeld(SGuiElem elem) {
  return SGuiElem(new MouseButtonHeld(std::move(elem), 1));
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

  virtual bool onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
      *highlighted = myIndex;
      canTurnOff = true;
    } else if (*highlighted == myIndex && canTurnOff) {
      *highlighted = none;
      canTurnOff = false;
    }
    return false;
  }
};

class MouseHighlight2 : public GuiStack {
  public:
  MouseHighlight2(SGuiElem h, SGuiElem h2 = nullptr)
      : GuiStack(h2 ? makeVec(std::move(h), std::move(h2)) : makeVec(std::move(h))) {}

  virtual void render(Renderer& r) override {
    if (over)
      elems[0]->render(r);
    else if (elems.size() > 1)
      elems[1]->render(r);
  }

  virtual void onMouseGone() override {
    over = false;
  }

  virtual bool onMouseMove(Vec2 pos) override {
    over = pos.inRectangle(getBounds());
    return false;
  }

  bool over = false;
};

SGuiElem GuiFactory::mouseHighlight(SGuiElem elem, int myIndex, optional<int>* highlighted) {
  return SGuiElem(new MouseHighlight(std::move(elem), myIndex, highlighted));
}

class MouseHighlightGameChoice : public GuiStack {
  public:
  MouseHighlightGameChoice(SGuiElem h, optional<PlayerRoleChoice> my, optional<PlayerRoleChoice>& highlight)
    : GuiStack(std::move(h)), myChoice(my), highlighted(highlight) {}

  virtual void onMouseGone() override {
    if (highlighted == myChoice)
      highlighted = none;
  }

  virtual bool onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      highlighted = myChoice;
    else if (highlighted == myChoice)
      highlighted = none;
    return false;
  }

  virtual void render(Renderer& r) override {
    if (highlighted == myChoice)
      elems[0]->render(r);
  }

  private:
  optional<PlayerRoleChoice> myChoice;
  optional<PlayerRoleChoice>& highlighted;
};

SGuiElem GuiFactory::mouseHighlightGameChoice(SGuiElem elem,
    optional<PlayerRoleChoice> my, optional<PlayerRoleChoice>& highlight) {
  return SGuiElem(new MouseHighlightGameChoice(std::move(elem), my, highlight));
}

SGuiElem GuiFactory::mouseHighlight2(SGuiElem elem, SGuiElem noHighlight) {
  return SGuiElem(new MouseHighlight2(std::move(elem), std::move(noHighlight)));
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

  virtual bool onMouseMove(Vec2 pos) override {
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

  virtual bool onMouseMove(Vec2 pos) override {
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
    scrollPos->setBounds(getBounds().height() / 2, scrollLength() + getBounds().height() / 2);
    return scrollPos->get(clock->getRealMillis()) - getBounds().height() / 2;
  }

  void addScrollPos(double v) {
    scrollPos->add(v, clock->getRealMillis());
    scrollPos->setBounds(getBounds().height() / 2, scrollLength() + getBounds().height() / 2);
  }

  virtual bool onMouseWheel(Vec2 v, bool up) override {
    if (v.inRectangle(Rectangle(Vec2(content->getBounds().left(), getBounds().top()),
        getBounds().bottomRight()))) {
      if (up)
        addScrollPos(- wheelScrollUnit);
      else
        addScrollPos(wheelScrollUnit);
      return true;
    }
    return false;
  }

  virtual bool onLeftClick(Vec2 v) override {
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
    return false;
  }

  virtual bool onMouseMove(Vec2 v) override {
    if (*held != notHeld)
      scrollPos->reset(getBounds().height() / 2 + scrollLength() * calcPos(v.y - *held));
    return false;
  }

  virtual void onMouseRelease(Vec2) override {
    *held = notHeld;
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
    return max<double>(getBounds().height() / 2,
        min<double>(scrollPos->get(clock->getRealMillis()), *content->getPreferredHeight() - getBounds().height() / 2));
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

  virtual bool onLeftClick(Vec2 v) override {
    if (v.y >= getBounds().top() && v.y < getBounds().bottom())
      return content->onLeftClick(v);
    return false;
  }

  virtual bool onRightClick(Vec2 v) override {
    if (v.y >= getBounds().top() && v.y < getBounds().bottom())
      return content->onRightClick(v);
    return false;
  }

  virtual void onMouseGone() override {
    content->onMouseGone();
  }

  virtual bool onMouseMove(Vec2 v) override {
    if (v.inRectangle(getBounds()))
      return content->onMouseMove(v);
    else
      content->onMouseGone();
    return false;
  }

  virtual void onMouseRelease(Vec2 pos) override {
    content->onMouseRelease(pos);
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    return content->onKeyPressed2(key);
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

  virtual bool onMouseWheel(Vec2 v, bool up) override {
    if (up)
      addScrollPos(-1);
    else
      addScrollPos(1);
    return true;
  }

  void setPositionFromClick(Vec2 v) {
    setPosition((int) round((double)(v.x - getBounds().left()) * maxValue / scrollLength()));
  }

  virtual bool onLeftClick(Vec2 v) override {
    if (v.inRectangle(getBounds())) {
      held = true;
      setPositionFromClick(v);
      return true;
    }
    return false;
  }

  virtual bool onMouseMove(Vec2 v) override {
    if (held)
      setPositionFromClick(v);
    return false;
  }

  virtual void onMouseRelease(Vec2) override {
    held = false;
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
  return textures[id];
}

void GuiFactory::loadImages() {
  iconTextures.clear();
  loadFreeImages(freeImagesPath);
  if (nonFreeImagesPath)
    loadNonFreeImages(*nonFreeImagesPath);
}

void GuiFactory::loadFreeImages(const DirectoryPath& path) {
  textures[TexId::SCROLLBAR] = Texture(path.file("ui/scrollbar.png"));
  textures[TexId::SCROLL_BUTTON] = Texture(path.file("ui/scrollmark.png"));
  textures[TexId::BACKGROUND_PATTERN] = Texture(path.file("window_bg.png"));
  text = Color::WHITE;
  titleText = Color::YELLOW;
  inactiveText = Color::LIGHT_GRAY;
  textures[TexId::HORI_CORNER1] = Texture(path.file("ui/horicorner1.png"));
  textures[TexId::HORI_CORNER2] = Texture(path.file("ui/horicorner2.png"));
  textures[TexId::HORI_LINE] = Texture(path.file("ui/horiline.png"));
  textures[TexId::HORI_MIDDLE] = Texture(path.file("ui/horimiddle.png"));
  textures[TexId::VERT_BAR] = Texture(path.file("ui/vertbar.png"));
  textures[TexId::HORI_BAR] = Texture(path.file("ui/horibar.png"));
  textures[TexId::CORNER_TOP_LEFT] = Texture(path.file("ui/cornerTOPL.png"));
  textures[TexId::CORNER_TOP_RIGHT] = Texture(path.file("ui/cornerTOPR.png"));
  textures[TexId::CORNER_BOTTOM_RIGHT] = Texture(path.file("ui/cornerBOTTOMR.png"));

  textures[TexId::HORI_BAR_MINI] = Texture(path.file("ui/horibarmini.png"));
  textures[TexId::VERT_BAR_MINI] = Texture(path.file("ui/vertbarmini.png"));
  textures[TexId::CORNER_MINI] = Texture(path.file("ui/cornermini.png"));

  textures[TexId::HORI_BAR_MINI2] = Texture(path.file("ui/horibarmini2.png"));
  textures[TexId::VERT_BAR_MINI2] = Texture(path.file("ui/vertbarmini2.png"));
  textures[TexId::CORNER_MINI2] = Texture(path.file("ui/cornermini2.png"));
  textures[TexId::IMMIGRANT_BG] = Texture(path.file("ui/immigrantbg.png"));
  textures[TexId::IMMIGRANT2_BG] = Texture(path.file("ui/immigrant2bg.png"));
  textures[TexId::SCROLL_UP] = Texture(path.file("ui/up.png"));
  textures[TexId::SCROLL_DOWN] = Texture(path.file("ui/down.png"));
  textures[TexId::WINDOW_CORNER] = Texture(path.file("ui/corner1.png"));
  textures[TexId::WINDOW_CORNER_EXIT] = Texture(path.file("ui/corner2X.png"));
  textures[TexId::WINDOW_CORNER_EXIT_HIGHLIGHT] = Texture(path.file("ui/corner2X_highlight.png"));
  textures[TexId::WINDOW_VERT_BAR] = Texture(path.file("ui/vertibarmsg1.png"));
  textures[TexId::MAIN_MENU_HIGHLIGHT] = Texture(path.file("ui/menu_highlight.png"));
  textures[TexId::SPLASH1] = Texture(path.file("splash2f.png"));
  textures[TexId::SPLASH2] = Texture(path.file("splash2e.png"));
  textures[TexId::LOADING_SPLASH] = Texture(path.file(Random.choose(
            "splash2a.png"_s,
            "splash2b.png"_s,
            "splash2c.png"_s,
            "splash2d.png"_s)));
  textures[TexId::UI_HIGHLIGHT] = Texture(path.file("ui/ui_highlight.png"));
  textures[TexId::MINIMAP_BAR] = Texture(path.file("minimap_bar.png"));
  const int tabIconWidth = 42;
  for (int i = 0; i < 8; ++i)
    iconTextures.push_back(Texture(path.file("icons.png"), 0, i * tabIconWidth, tabIconWidth, tabIconWidth));
  auto addAttr = [&](AttrType attr, Vec2 pos) {
    const int width = 18;
    attrTextures[attr] =
        Texture(path.file("stat_icons.png"), pos.x * width, pos.y * width, width, width);
  };
  auto getAttrCoord = [&] (AttrType attr) {
    switch (attr) {
      case AttrType::DAMAGE:
        return Vec2(0, 0);
      case AttrType::DEFENSE:
        return Vec2(1, 0);
      case AttrType::SPELL_DAMAGE:
        return Vec2(0, 1);
      case AttrType::RANGED_DAMAGE:
        return Vec2(1, 1);
    }
  };
  for (auto attr : ENUM_ALL(AttrType))
    addAttr(attr, getAttrCoord(attr));
  auto loadIcons = [&] (int width, int count, const char* file) {
    for (int i = 0; i < count; ++i)
      iconTextures.push_back(Texture(path.file(file), 0, i * width, width, width));
  };
  loadIcons(16, 4, "morale_icons.png");
  loadIcons(16, 2, "team_icons.png");
  loadIcons(48, 6, "minimap_icons.png");
  loadIcons(32, 1, "expand_up.png");
  loadIcons(32, 1, "special_immigrant.png");
  auto addSpell = [&](SpellId id, Vec2 pos) {
    const int width = 40;
    spellTextures[id] =
        Texture(path.file("spells.png"), pos.x * width, pos.y * width, width, width);
  };
  auto getSpellCoord = [&] (SpellId id) {
    switch (id) {
      case SpellId::HEAL_SELF:
        return Vec2(1, 3);
      case SpellId::SUMMON_INSECTS:
        return Vec2(0, 1);
      case SpellId::DECEPTION:
        return Vec2(0, 2);
      case SpellId::SPEED_SELF:
        return Vec2(0, 3);
      case SpellId::DAM_BONUS:
        return Vec2(1, 5);
      case SpellId::DEF_BONUS:
        return Vec2(1, 6);
      case SpellId::FIRE_SPHERE_PET:
        return Vec2(0, 6);
      case SpellId::TELEPORT:
        return Vec2(0, 7);
      case SpellId::FIREBALL:
        return Vec2(1, 7);
      case SpellId::FIREBALL_DRAGON:
        return Vec2(1, 7);
      case SpellId::INVISIBILITY:
        return Vec2(0, 8);
      case SpellId::CIRCULAR_BLAST:
        return Vec2(1, 2);
      case SpellId::BLAST:
        return Vec2(0, 15);
      case SpellId::PORTAL:
        return Vec2(0, 10);
      case SpellId::SUMMON_SPIRIT:
        return Vec2(0, 11);
      case SpellId::CURE_POISON:
        return Vec2(0, 12);
      case SpellId::METEOR_SHOWER:
        return Vec2(0, 13);
      case SpellId::MAGIC_MISSILE:
        return Vec2(1, 1);
      case SpellId::SUMMON_ELEMENT:
        return Vec2(1, 0);
      case SpellId::HEAL_OTHER:
        return Vec2(1, 4);
    }
  };
  for (SpellId id : ENUM_ALL(SpellId))
    addSpell(id, getSpellCoord(id));
}

void GuiFactory::loadNonFreeImages(const DirectoryPath& path) {
  textures[TexId::KEEPER_CHOICE] = Texture(path.file("keeper_choice.png"));
  textures[TexId::ADVENTURER_CHOICE] = Texture(path.file("adventurer_choice.png"));
  textures[TexId::KEEPER_HIGHLIGHT] = Texture(path.file("keeper_highlight.png"));
  textures[TexId::ADVENTURER_HIGHLIGHT] = Texture(path.file("adventurer_highlight.png"));
  textures[TexId::MENU_ITEM] = Texture(path.file("barmid.png"));
  // If menu_core fails to load, try the lower resolution versions
  if (auto tex = Texture::loadMaybe(path.file("menu_core.png"))) {
    textures[TexId::MENU_CORE] = std::move(*tex);
    textures[TexId::MENU_MOUTH] = Texture(path.file("menu_mouth.png"));
  } else {
    textures[TexId::MENU_CORE] = Texture(path.file("menu_core_sm.png"));
    textures[TexId::MENU_MOUTH] = Texture(path.file("menu_mouth_sm.png"));
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

class Conditional : public GuiStack {
  public:
  Conditional(SGuiElem e, function<bool(GuiElem*)> f) : GuiStack(makeVec(std::move(e))), cond(f) {}

  virtual bool isVisible(int num) override {
    return cond(this);
  }

  protected:
  function<bool(GuiElem*)> cond;
};

SGuiElem GuiFactory::conditional(SGuiElem elem, function<bool()> f) {
  return SGuiElem(new Conditional(std::move(elem), [f](GuiElem*) { return f(); }));
}

class ConditionalStopKeys : public Conditional {
  public:
  using Conditional::Conditional;

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (cond(this))
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
  return stack(SGuiElem(new Conditional(std::move(elem), f)),
      SGuiElem(new Conditional(std::move(alter), [=] (GuiElem* e) { return !f(e);})));
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
        background(background1),
        margins(std::move(content), 1),
        miniBorder()
	);
  if (onExitButton)
    ret.push_back(reverseButton(onExitButton, {getKey(SDL::SDLK_ESCAPE)}, captureExitClick));
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
        stopMouseMovement(),
        alignment(Alignment::TOP_RIGHT, button(onExitButton, getKey(SDL::SDLK_ESCAPE), true), Vec2(38, 38)),
        rectangle(Color::BLACK),
        background(background1),
        margins(std::move(content), 20, 35, 30, 30),
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
      translucentBackground(std::move(content)),
      rectangleBorder(Color::GRAY)
  );
}

SGuiElem GuiFactory::translucentBackground() {
  return stack(
      stopMouseMovement(),
        rectangle(translucentBgColor));
}

namespace {

constexpr int lineHeight = 20;

class TextInput : public GuiElem {
  public:
  TextInput(int width, int maxLines, shared_ptr<string> text)
      : width(width), maxLines(maxLines), text(std::move(text)) {
    SDL::SDL_StartTextInput();
  }

  ~TextInput() {
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
  return make_shared<TextInput>(width, maxLines, text);
}

SGuiElem GuiFactory::minimapBar(SGuiElem icon1, SGuiElem icon2) {
  auto& tex = textures[TexId::MINIMAP_BAR];
  return preferredSize(tex.getSize(),
      stack(
          stopMouseMovement(),
          sprite(tex, Alignment::CENTER, Color::WHITE),
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

SGuiElem GuiFactory::icon(AttrType attr) {
  return sprite(attrTextures[attr], Alignment::CENTER, Color::WHITE);
}

SGuiElem GuiFactory::spellIcon(SpellId id) {
  return sprite(spellTextures[id], Alignment::CENTER);
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
  return margins(uiHighlight(c), -8, -3, -3, 3);
  //return leftMargin(-8, topMargin(-4, sprite(TexId::UI_HIGHLIGHT, Alignment::LEFT_STRETCHED, c)));
}

SGuiElem GuiFactory::blink(SGuiElem elem) {
  return conditional(elem, [this]() { return blinkingState(clock->getRealMillis(), 2, 4) < 0.5; });
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
        elem->onMouseRelease(Vec2(event.button.x, event.button.y));
      dragContainer.pop();
      break;
    case SDL::SDL_MOUSEMOTION: {
      bool captured = false;
      for (auto elem : guiElems)
        if (!captured)
          captured |= elem->onMouseMove(Vec2(event.motion.x, event.motion.y));
        else
          elem->onMouseGone();
      break;}
    case SDL::SDL_MOUSEBUTTONDOWN: {
      Vec2 clickPos(event.button.x, event.button.y);
      for (auto elem : guiElems) {
        if (event.button.button == SDL_BUTTON_RIGHT && elem->onRightClick(clickPos))
          break;
        if (event.button.button == SDL_BUTTON_LEFT && elem->onLeftClick(clickPos))
          break;
        if (event.button.button == SDL_BUTTON_MIDDLE && elem->onMiddleClick(clickPos))
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
        if (elem->onMouseWheel(renderer.getMousePos(), event.wheel.y > 0))
          break;
      break;
    default: break;
  }
  for (auto elem : guiElems)
    elem->onRefreshBounds();
}
