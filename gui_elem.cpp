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

#include "sdl.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

DEF_UNIQUE_PTR(VerticalList);

Rectangle GuiElem::getBounds() {
  return bounds;
}

void GuiElem::setBounds(Rectangle b) {
  bounds = b;
  onRefreshBounds();
}

void GuiElem::setPreferredBounds(Vec2 origin) {
  setBounds(Rectangle(origin, origin + Vec2(*getPreferredWidth(), *getPreferredHeight())));
}

GuiElem::~GuiElem() {
}

class Button : public GuiElem {
  public:
  Button(function<void(Rectangle)> f) : fun(f) {}

  virtual bool onLeftClick(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
      fun(getBounds());
      return true;
    }
    return false;
  }

  protected:
  function<void(Rectangle)> fun;
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

class ButtonKey : public Button {
  public:
  ButtonKey(function<void(Rectangle)> f, SDL_Keysym key, bool cap) : Button(f), hotkey(key), capture(cap) {}

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    if (GuiFactory::keyEventEqual(hotkey, key)) {
      fun(getBounds());
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

GuiFactory::GuiFactory(Renderer& r, Clock* c, Options* o) : clock(c), renderer(r), options(o) {
}

DragContainer& GuiFactory::getDragContainer() {
  return dragContainer;
}

PGuiElem GuiFactory::buttonRect(function<void(Rectangle)> fun, SDL_Keysym hotkey, bool capture) {
  return PGuiElem(new ButtonKey(fun, hotkey, capture));
}

PGuiElem GuiFactory::button(function<void()> fun, SDL_Keysym hotkey, bool capture) {
  return PGuiElem(new ButtonKey([=](Rectangle) { fun(); }, hotkey, capture));
}

PGuiElem GuiFactory::buttonRect(function<void(Rectangle)> fun) {
  return PGuiElem(new Button(fun));
}

PGuiElem GuiFactory::button(function<void()> fun) {
  return PGuiElem(new Button([=](Rectangle) { fun(); }));
}

class ReleaseButton : public GuiElem {
  public:
  ReleaseButton(function<void(Rectangle)> f) : fun(f) {}

  virtual bool onLeftClick(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      pressed = true;
    return false;
  }

  virtual void onMouseRelease(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      fun(getBounds());
  }

  protected:
  function<void(Rectangle)> fun;
  bool pressed = false;
};

PGuiElem GuiFactory::releaseButton(function<void(Rectangle)> fun) {
  return PGuiElem(new ReleaseButton(fun));
}

PGuiElem GuiFactory::releaseButton(function<void()> fun) {
  return PGuiElem(new ReleaseButton([=](Rectangle) { fun(); }));
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

PGuiElem GuiFactory::reverseButton(function<void()> fun, vector<SDL_Keysym> hotkey, bool capture) {
  return stack(
      keyHandler(fun, hotkey, true),
      PGuiElem(new ReverseButton(fun, capture)));
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

PGuiElem GuiFactory::mouseWheel(function<void(bool)> fun) {
  return PGuiElem(new MouseWheel(fun));
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

  virtual bool onMouseWheel(Vec2 pos, bool up) override {
    return pos.inRectangle(getBounds());
  }
};

PGuiElem GuiFactory::stopMouseMovement() {
  return PGuiElem(new StopMouseMovement());
}

class DrawCustom : public GuiElem {
  public:
  typedef function<void(Renderer&, Rectangle)> DrawFun;
  DrawCustom(DrawFun draw, function<int()> width = nullptr) : drawFun(draw), preferredWidth(width) {}

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
  DrawFun drawFun;
  function<int()> preferredWidth;
};

PGuiElem GuiFactory::rectangle(Color color, optional<Color> borderColor) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawFilledRectangle(bounds, color, borderColor);
        }));
}

PGuiElem GuiFactory::repeatedPattern(Texture& tex) {
  return PGuiElem(new DrawCustom(
        [&tex] (Renderer& r, Rectangle bounds) {
          r.drawSprite(bounds.topLeft(), Vec2(0, 0), Vec2(bounds.width(), bounds.height()), tex);
        }));
}

PGuiElem GuiFactory::sprite(Texture& tex, double height) {
  return PGuiElem(new DrawCustom(
        [&tex, height] (Renderer& r, Rectangle bounds) {
          Vec2 size = tex.getSize();
          r.drawSprite(bounds.topLeft(), Vec2(0, 0), size, tex,
              Vec2(height * size.x / size.y, height));
        }));
}

PGuiElem GuiFactory::sprite(Texture& tex, Alignment align, bool vFlip, bool hFlip, Vec2 offset,
    function<Color()> col) {
  return PGuiElem(new DrawCustom(
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
          r.drawSprite(pos, origin, size, tex, stretchSize, !!col ? col() : colors[ColorId::WHITE], vFlip, hFlip);
        }));
}

PGuiElem GuiFactory::label(const string& s, Color c, char hotkey) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawTextWithHotkey(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, s, 0);
          r.drawTextWithHotkey(c, bounds.topLeft().x, bounds.topLeft().y, s, hotkey);
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

static vector<string> breakText(Renderer& renderer, const string& text, int maxWidth, int size = Renderer::textSize) {
  if (text.empty())
    return {""};
  vector<string> rows;
  for (string line : split(text, {'\n'})) {
    rows.push_back("");
    for (string word : split(line, {' '}))
      for (string subword : breakWord(renderer, word, maxWidth, size))
        if (renderer.getTextLength(rows.back() + ' ' + subword, size) <= maxWidth)
          rows.back().append((rows.back().size() > 0 ? " " : "") + subword);
        else
          rows.push_back(subword);
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
      renderer.drawText(color, getBounds().left(), height, lines[i],
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

PGuiElem GuiFactory::labelMultiLine(const string& s, int lineHeight, int size, Color c) {
  return PGuiElem(new VariableLabel([=]{ return s;}, lineHeight, size, c));
}

static void lighten(Color& c) {
  if (3 * 255 - c.r - c.g - c.b < 75)
    c = colors[ColorId::YELLOW];
  int a = 160;
  int b = 200;
  auto fun = [=] (int x) { return x * (255 - a) / b + a;};
  c.r = min(255, fun(c.r));
  c.g = min(255, fun(c.g));
  c.b = min(255, fun(c.b));
}

PGuiElem GuiFactory::labelHighlight(const string& s, Color c, char hotkey) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawTextWithHotkey(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, s, 0);
          Color c1(c);
          if (r.getMousePos().inRectangle(bounds))
            lighten(c1);
          r.drawTextWithHotkey(c1, bounds.topLeft().x, bounds.topLeft().y, s, hotkey);
        }, width));
}

/*static Color blinking(Color c1, Color c2, int period, int state) {
  double s = (state % period) / (double) period;
  double c = (cos(s * 2 * 3.14159) + 1) / 2;
  return Color(c1.r * c + c2.r * (1 - c), c1.g * c + c2.g * (1 - c), c1.b * c + c2.b * (1 - c));
}

PGuiElem GuiFactory::labelHighlightBlink(const string& s, Color c1, Color c2) {
  int period = 700;
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = blinking(c1, c2, period, clock->getRealMillis());
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, s);
          Color c1(c);
          if (r.getMousePos().inRectangle(bounds))
            lighten(c1);
          r.drawText(c1, bounds.topLeft().x, bounds.topLeft().y, s);
        }, width));
}*/

PGuiElem GuiFactory::label(const string& s, function<Color()> colorFun, char hotkey) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, s);
          r.drawTextWithHotkey(colorFun(), bounds.topLeft().x, bounds.topLeft().y, s, hotkey);
        }, width));
}

PGuiElem GuiFactory::labelFun(function<string()> textFun, function<Color()> colorFun) {
  function<int()> width = [this, textFun] { return renderer.getTextLength(textFun()); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, textFun());
          r.drawText(colorFun(), bounds.topLeft().x, bounds.topLeft().y, textFun());
        }, width));
}

PGuiElem GuiFactory::labelFun(function<string()> textFun, Color color) {
  auto width = [=] { return renderer.getTextLength(textFun()); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, textFun());
          r.drawText(color, bounds.topLeft().x, bounds.topLeft().y, textFun());
        }, width));
}

PGuiElem GuiFactory::label(const string& s, int size, Color c) {
  auto width = [=] { return renderer.getTextLength(s, size); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.topLeft().x + 1, bounds.topLeft().y + 2, s, Renderer::NONE, size);
          r.drawText(c, bounds.topLeft().x, bounds.topLeft().y, s, Renderer::NONE, size);
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

PGuiElem GuiFactory::centeredLabel(Renderer::CenterType center, const string& s, int size, Color c) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Vec2 pos = getTextPos(bounds, center);
          r.drawText(transparency(colors[ColorId::BLACK], 100), pos.x + 1, pos.y + 2, s, center, size);
          r.drawText(c, pos.x, pos.y, s, center, size);
        }));
}

PGuiElem GuiFactory::centeredLabel(Renderer::CenterType center, const string& s, Color c) {
  return centeredLabel(center, s, Renderer::textSize, c);
}

PGuiElem GuiFactory::variableLabel(function<string()> fun, int lineHeight, int size, Color color) {
  return PGuiElem(new VariableLabel(fun, lineHeight, size, color));
}

PGuiElem GuiFactory::labelUnicode(const string& s, Color color, int size, Renderer::FontId fontId) {
  return labelUnicode(s, [color] { return color; }, size, fontId);
}

PGuiElem GuiFactory::labelUnicode(const string& s, function<Color()> color, int size, Renderer::FontId fontId) {
  auto width = [=] { return renderer.getTextLength(s, size, fontId); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          Color c = color();
          if (r.getMousePos().inRectangle(bounds))
            lighten(c);
          r.drawText(fontId, size, c, bounds.topLeft().x, bounds.topLeft().y, s);
        }, width));
}

class MainMenuLabel : public GuiElem {
  public:
  MainMenuLabel(const string& s, Color c, double vPad) : text(s), color(c), vPadding(vPad) {}

  virtual void render(Renderer& renderer) override {
    int size = (0.9 - 2 * vPadding) * getBounds().height();
    double height = getBounds().top() + vPadding * getBounds().height();
    renderer.drawText(color, getBounds().middle().x, height - size / 11, text, Renderer::HOR, size);
  }

  private:
  string text;
  Color color;
  double vPadding;
};

PGuiElem GuiFactory::mainMenuLabel(const string& s, double vPadding, Color c) {
  return PGuiElem(new MainMenuLabel(s, c, vPadding));
}

class GuiLayout : public GuiElem {
  public:
  GuiLayout(vector<PGuiElem> e) : elems(std::move(e)) {}
  GuiLayout(PGuiElem e) { elems.push_back(std::move(e)); }

  virtual bool onLeftClick(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i))
        if (elems[i]->onLeftClick(pos))
          return true;
    return false;
  }

  virtual bool onRightClick(Vec2 pos) override {
    for (int i : AllReverse(elems))
      if (isVisible(i))
        if (elems[i]->onRightClick(pos))
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
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->render(r);
  }

  virtual void onRefreshBounds() override {
    for (int i : All(elems))
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

  virtual Rectangle getElemBounds(int num) {
    return getBounds();
  }

  virtual bool isVisible(int num) {
    return true;
  }

  virtual optional<int> getPreferredWidth() override {
    for (int i : All(elems))
      if (auto width = elems[i]->getPreferredWidth())
        if (getElemBounds(i) == getBounds())
          return *width;
    return none;
  }

  virtual optional<int> getPreferredHeight() override {
    for (int i : All(elems))
      if (auto height = elems[i]->getPreferredHeight())
        if (getElemBounds(i) == getBounds())
          return *height;
    return none;
  }

  protected:
  vector<PGuiElem> elems;
};

PGuiElem GuiFactory::stack(vector<PGuiElem> elems) {
  return PGuiElem(new GuiLayout(std::move(elems)));
}

PGuiElem GuiFactory::stack(PGuiElem g1, PGuiElem g2) {
  return stack(makeVec<PGuiElem>(std::move(g1), std::move(g2)));
}

PGuiElem GuiFactory::stack(PGuiElem g1, PGuiElem g2, PGuiElem g3) {
  return stack(makeVec<PGuiElem>(std::move(g1), std::move(g2), std::move(g3)));
}

PGuiElem GuiFactory::stack(PGuiElem g1, PGuiElem g2, PGuiElem g3, PGuiElem g4) {
  return stack(makeVec<PGuiElem>(std::move(g1), std::move(g2), std::move(g3), std::move(g4)));
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

PGuiElem GuiFactory::external(GuiElem* elem) {
  return PGuiElem(new External(elem)); 
}

class Focusable : public GuiLayout {
  public:
  Focusable(PGuiElem content, vector<SDL_Keysym> focus, vector<SDL_Keysym> defocus, bool& foc) :
      GuiLayout(makeVec<PGuiElem>(std::move(content))), focusEvent(focus), defocusEvent(defocus), focused(foc) {}

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

PGuiElem GuiFactory::focusable(PGuiElem content, vector<SDL_Keysym> focusEvent,
    vector<SDL_Keysym> defocusEvent, bool& focused) {
  return PGuiElem(new Focusable(std::move(content), focusEvent, defocusEvent, focused));
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

class RenderInBounds : public GuiLayout {
  public:
  RenderInBounds(PGuiElem e) : GuiLayout(std::move(e)) {}

  virtual void render(Renderer& r) override {
    r.setScissor(getBounds());
    elems[0]->render(r);
    r.setScissor(none);
  }
};

PGuiElem GuiFactory::renderInBounds(PGuiElem elem) {
  return PGuiElem(new RenderInBounds(std::move(elem)));
}

class AlignmentGui : public GuiLayout {
  public:
  AlignmentGui(PGuiElem e, GuiFactory::Alignment align, optional<Vec2> sz)
      : GuiLayout(makeVec<PGuiElem>(std::move(e))), alignment(align), size(sz) {}

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
      default: FAIL << "Unhandled alignment: " << (int)alignment;
    }
    return Rectangle();
  }

  private:
  GuiFactory::Alignment alignment;
  optional<Vec2> size;
};

PGuiElem GuiFactory::alignment(GuiFactory::Alignment alignment, PGuiElem content, optional<Vec2> size) {
  return PGuiElem(new AlignmentGui(std::move(content), alignment, size));
}
 
PGuiElem GuiFactory::keyHandler(function<void(SDL_Keysym)> fun, bool capture) {
  return PGuiElem(new KeyHandler(fun, capture));
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

PGuiElem GuiFactory::keyHandler(function<void()> fun, vector<SDL_Keysym> key, bool capture) {
  return PGuiElem(new KeyHandler2(fun, key, capture));
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

PGuiElem GuiFactory::keyHandlerChar(function<void ()> fun, char hotkey, bool capture, bool useAltIfWasdOn) {
  return PGuiElem(new KeyHandlerChar(fun, hotkey, capture,
       [=] { return useAltIfWasdOn && options->getBoolValue(OptionId::WASD_SCROLLING); }));
}

PGuiElem GuiFactory::buttonChar(function<void()> fun, char hotkey, bool capture, bool useAltIfWasdOn) {
  return stack(
      PGuiElem(new Button([=](Rectangle) { fun(); })),
      PGuiElem(keyHandlerChar(fun, hotkey, capture, useAltIfWasdOn)));
}

class ElemList : public GuiLayout {
  public:
  ElemList(vector<PGuiElem> e, vector<int> h, int alignBack, bool middleEl)
    : GuiLayout(std::move(e)), heights(h), numAlignBack(alignBack), middleElem(middleEl) {
    //CHECK(heights.size() > 0);
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

  int getElemsHeight(int numElems) {
    int ret = 0;
    CHECK(numElems <= getSize());
    for (int i : Range(numElems))
      ret += heights[i];
    return ret;
  }

  int getSize() {
    return heights.size();
  }

  int getTotalHeight() {
    return getElemsHeight(getSize());
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

  void renderPart(Renderer& r, int scrollPos) {
    int totHeight = 0;
    for (int i : Range(scrollPos, heights.size())) {
      if (totHeight + elems[i]->getBounds().height() > getBounds().height())
        break;
      elems[i]->render(r);
      totHeight += elems[i]->getBounds().height();
    }
  }

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().getXRange(), getElemPosition(num, getBounds().getYRange()));
  }

  optional<int> getPreferredWidth() override {
    for (auto& elem : elems)
      if (auto width = elem->getPreferredWidth())
        return width;
    return none;
  }

  optional<int> getPreferredHeight() override {
    return getTotalHeight();
  }
};

PGuiElem GuiFactory::verticalList(vector<PGuiElem> e, int height) {
  vector<int> heights(e.size(), height);
  return PGuiElem(new VerticalList(std::move(e), heights, 0, false));
}

class HorizontalList : public ElemList {
  public:
  using ElemList::ElemList;

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getElemPosition(num, getBounds().getXRange()), getBounds().getYRange());
  }

  optional<int> getPreferredHeight() override {
    for (auto& elem : elems)
      if (auto height = elem->getPreferredHeight())
        return height;
    return none;
  }

  optional<int> getPreferredWidth() override {
    return getTotalHeight();
  }
};

PGuiElem GuiFactory::horizontalList(vector<PGuiElem> e, int height) {
  vector<int> heights(e.size(), height);
  return PGuiElem(new HorizontalList(std::move(e), heights, 0, false));
}

GuiFactory::ListBuilder GuiFactory::getListBuilder(int defaultSize) {
  return ListBuilder(*this, defaultSize);
}

GuiFactory::ListBuilder::ListBuilder(GuiFactory& g, int defSz) : gui(g), defaultSize(defSz) {}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElem(PGuiElem elem, int size) {
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
  CHECK(!backElems);
  CHECK(!middleElem);
  if (size == 0) {
    CHECK(defaultSize > 0);
    size = defaultSize;
  }
  elems.push_back(gui.empty());
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElemAuto(PGuiElem elem) {
  CHECK(!backElems);
  CHECK(!middleElem);
  int size = -1;
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addMiddleElem(PGuiElem elem) {
  addElem(std::move(elem), 1234);
  middleElem = true;
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackElemAuto(PGuiElem elem) {
  ++backElems;
  int size = -1;
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackElem(PGuiElem elem, int size) {
  ++backElems;
  if (size == 0) {
    CHECK(defaultSize > 0);
    size = defaultSize;
  }
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
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

vector<PGuiElem>& GuiFactory::ListBuilder::getAllElems() {
  return elems;
}

void GuiFactory::ListBuilder::clear() {
  elems.clear();
  sizes.clear();
  backElems = 0;
}

PGuiElem GuiFactory::ListBuilder::buildVerticalList() {
  for (int i : All(sizes))
    if (sizes[i] == -1)
      sizes[i] = *elems[i]->getPreferredHeight();
  return PGuiElem(new VerticalList(std::move(elems), sizes, backElems, middleElem));
}

PGuiElem GuiFactory::ListBuilder::buildHorizontalList() {
  for (int i : All(sizes))
    if (sizes[i] == -1)
      sizes[i] = *elems[i]->getPreferredWidth();
  return PGuiElem(new HorizontalList(std::move(elems), sizes, backElems, middleElem));
}

PGuiElem GuiFactory::ListBuilder::buildHorizontalListFit() {
  PGuiElem ret = gui.horizontalListFit(std::move(elems), 0);
  return ret;
}

PGuiElem GuiFactory::ListBuilder::buildVerticalListFit() {
  PGuiElem ret = gui.verticalListFit(std::move(elems), 0);
  return ret;
}

class VerticalListFit : public GuiLayout {
  public:
  VerticalListFit(vector<PGuiElem> e, double space) : GuiLayout(std::move(e)), spacing(space) {
    CHECK(!elems.empty());
  }

  virtual Rectangle getElemBounds(int num) override {
    int elemHeight = double(getBounds().height()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().topLeft() + Vec2(0, num * (elemHeight * (1.0 + spacing))), 
        getBounds().topRight() + Vec2(0, num * (elemHeight * (1.0 + spacing)) + elemHeight));
  }

  protected:
  double spacing;
};


PGuiElem GuiFactory::verticalListFit(vector<PGuiElem> e, double spacing) {
  return PGuiElem(new VerticalListFit(std::move(e), spacing));
}

class HorizontalListFit : public GuiLayout {
  public:
  HorizontalListFit(vector<PGuiElem> e, double space) : GuiLayout(std::move(e)), spacing(space) {
    //CHECK(!elems.empty());
  }

  virtual Rectangle getElemBounds(int num) override {
    int elemHeight = double(getBounds().width()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().topLeft() + Vec2(num * (elemHeight * (1.0 + spacing)), 0), 
        getBounds().bottomLeft() + Vec2(num * (elemHeight * (1.0 + spacing)) + elemHeight, 0));
  }

  protected:
  double spacing;
};


PGuiElem GuiFactory::horizontalListFit(vector<PGuiElem> e, double spacing) {
  return PGuiElem(new HorizontalListFit(std::move(e), spacing));
}

class VerticalAspect : public GuiLayout {
  public:
  VerticalAspect(PGuiElem e, double r) : GuiLayout(makeVec<PGuiElem>(std::move(e))), ratio(r) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0);
    double width = ratio * getBounds().height();
    return Rectangle((getBounds().topLeft() + getBounds().topRight()) / 2 - Vec2(width / 2, 0),
        (getBounds().bottomLeft() + getBounds().bottomRight()) / 2 + Vec2(width / 2, 0));
  }

  private:
  double ratio;
};

PGuiElem GuiFactory::verticalAspect(PGuiElem elem, double ratio) {
  return PGuiElem(new VerticalAspect(std::move(elem), ratio));
}

class CenterHoriz : public GuiLayout {
  public:
  CenterHoriz(PGuiElem elem, int w = -1) : GuiLayout(makeVec<PGuiElem>(std::move(elem))),
      width(w > -1 ? w : *elems[0]->getPreferredWidth()) {}

  optional<int> getPreferredHeight() override {
    if (auto height = elems[0]->getPreferredHeight())
      return *height;
    else
      return none;
  }

  virtual Rectangle getElemBounds(int num) override {
    int center = (getBounds().left() + getBounds().right()) / 2;
    return Rectangle(center - width / 2, getBounds().top(), center + width / 2, getBounds().bottom());
  }

  private:
  int width;
};

PGuiElem GuiFactory::centerHoriz(PGuiElem e, int width) {
  if (width == 0)
    return empty();
  return PGuiElem(new CenterHoriz(std::move(e), width));
}

class MarginGui : public GuiLayout {
  public:
  MarginGui(PGuiElem top, PGuiElem rest, int _width, GuiFactory::MarginType t)
    : GuiLayout(makeVec<PGuiElem>(std::move(top), std::move(rest))), width(_width), type(t) {}

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

PGuiElem GuiFactory::margin(PGuiElem top, PGuiElem rest, int width, MarginType type) {
  return PGuiElem(new MarginGui(std::move(top), std::move(rest), width, type));
}

PGuiElem GuiFactory::marginAuto(PGuiElem top, PGuiElem rest, MarginType type) {
  int width;
  switch (type) {
    case MarginType::LEFT:
    case MarginType::RIGHT: width = *top->getPreferredWidth(); break;
    case MarginType::TOP:
    case MarginType::BOTTOM: width = *top->getPreferredHeight(); break;
  }
  return PGuiElem(new MarginGui(std::move(top), std::move(rest), width, type));
}

class MaybeMargin : public MarginGui {
  public:
  MaybeMargin(PGuiElem top, PGuiElem rest, int width, GuiFactory::MarginType type, function<bool(Rectangle)> p)
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

PGuiElem GuiFactory::maybeMargin(PGuiElem top, PGuiElem rest, int width, MarginType type,
    function<bool(Rectangle)> pred) {
  return PGuiElem(new MaybeMargin(std::move(top), std::move(rest), width, type, pred));
}

class FullScreen : public GuiLayout {
  public:
  FullScreen(PGuiElem content, Renderer& r) : GuiLayout(makeVec<PGuiElem>(std::move(content))), renderer(r) {
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

PGuiElem GuiFactory::fullScreen(PGuiElem content) {
  return PGuiElem(new FullScreen(std::move(content), renderer));
}

class MarginFit : public GuiLayout {
  public:
  MarginFit(PGuiElem top, PGuiElem rest, double _width, GuiFactory::MarginType t)
    : GuiLayout(makeVec<PGuiElem>(std::move(top), std::move(rest))), width(_width), type(t) {}

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

PGuiElem GuiFactory::marginFit(PGuiElem top, PGuiElem rest, double width, MarginType type) {
  return PGuiElem(new MarginFit(std::move(top), std::move(rest), width, type));
}

PGuiElem GuiFactory::progressBar(Color c, double state) {
  return PGuiElem(new DrawCustom([=] (Renderer& r, Rectangle bounds) {
          int width = bounds.width() * state;
          if (width > 0)
            r.drawFilledRectangle(Rectangle(bounds.topLeft(),
                  Vec2(bounds.left() + width, bounds.bottom())), c);
        }));
}

class Margins : public GuiLayout {
  public:
  Margins(PGuiElem content, int l, int t, int r, int b)
      : GuiLayout(makeVec<PGuiElem>(std::move(content))), left(l), top(t), right(r), bottom(b) {}

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

PGuiElem GuiFactory::margins(PGuiElem content, int left, int top, int right, int bottom) {
  return PGuiElem(new Margins(std::move(content), left, top, right, bottom));
}

PGuiElem GuiFactory::margins(PGuiElem content, int all) {
  return PGuiElem(new Margins(std::move(content), all, all, all, all));
}

PGuiElem GuiFactory::leftMargin(int size, PGuiElem content) {
  return PGuiElem(new Margins(std::move(content), size, 0, 0, 0));
}

PGuiElem GuiFactory::rightMargin(int size, PGuiElem content) {
  return PGuiElem(new Margins(std::move(content), 0, 0, size, 0));
}

PGuiElem GuiFactory::topMargin(int size, PGuiElem content) {
  return PGuiElem(new Margins(std::move(content), 0, size, 0, 0));
}

class SetHeight : public GuiLayout {
  public:
  SetHeight(PGuiElem content, int h)
      : GuiLayout(makeVec<PGuiElem>(std::move(content))), height(h) {}

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().left(), getBounds().top(),
        getBounds().right(), getBounds().top() + height);
  }

  private:
  int height;
};

PGuiElem GuiFactory::setHeight(int height, PGuiElem content) {
  return PGuiElem(new SetHeight(std::move(content), height));
}

class SetWidth : public GuiLayout {
  public:
  SetWidth(PGuiElem content, int w)
      : GuiLayout(makeVec<PGuiElem>(std::move(content))), width(w) {}

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().left(), getBounds().top(),
        getBounds().left() + width, getBounds().bottom());
  }

  private:
  int width;
};

PGuiElem GuiFactory::setWidth(int width, PGuiElem content) {
  return PGuiElem(new SetWidth(std::move(content), width));
}

PGuiElem GuiFactory::bottomMargin(int size, PGuiElem content) {
  return PGuiElem(new Margins(std::move(content), 0, 0, 0, size));
}

class Invisible : public GuiLayout {
  public:
  Invisible(PGuiElem content) : GuiLayout(makeVec<PGuiElem>(std::move(content))) {}

  virtual bool isVisible(int num) {
    return false;
  }
};

PGuiElem GuiFactory::invisible(PGuiElem content) {
  return PGuiElem(new Invisible(std::move(content)));
}

class Switchable : public GuiLayout {
  public:
  Switchable(vector<PGuiElem> elems, function<int()> fun) : GuiLayout(std::move(elems)), switchFun(fun) {}

  virtual bool isVisible(int num) {
    return switchFun() == num;
  }

  private:
  function<int()> switchFun;
};

namespace {

class PreferredSize : public GuiElem {
  public:
  PreferredSize(Vec2 sz) : size(sz) {}

  virtual optional<int> getPreferredWidth() override {
    return size.x;
  }
  
  virtual optional<int> getPreferredHeight() override {
    return size.y;
  }

  private:
  Vec2 size;
};

}

PGuiElem GuiFactory::preferredSize(int width, int height) {
  return PGuiElem(new PreferredSize(Vec2(width, height)));
}

PGuiElem GuiFactory::empty() {
  return PGuiElem(new GuiElem());
}

class ViewObjectGui : public GuiElem {
  public:
  ViewObjectGui(const ViewObject& obj, double sc, Color c) : object(obj), scale(sc), color(c) {}
  ViewObjectGui(ViewId id, double sc, Color c) : object(id), scale(sc), color(c) {}
  
  virtual void render(Renderer& renderer) override {
    if (ViewObject* obj = boost::get<ViewObject>(&object))
      renderer.drawViewObject(getBounds().topLeft(), *obj, true, scale, color);
    else
      renderer.drawViewObject(getBounds().topLeft(), boost::get<ViewId>(object), true, scale, color);
  }

  private:
  variant<ViewObject, ViewId> object;
  double scale;
  Color color;
};

PGuiElem GuiFactory::viewObject(const ViewObject& object, double scale, Color color) {
  return PGuiElem(new ViewObjectGui(object, scale, color));
}

PGuiElem GuiFactory::viewObject(ViewId id, double scale, Color color) {
  return PGuiElem(new ViewObjectGui(id, scale, color));
}

PGuiElem GuiFactory::asciiBackground(ViewId id) {
  return PGuiElem(
      new DrawCustom([=] (Renderer& renderer, Rectangle bounds) { renderer.drawAsciiBackground(id, bounds);}));
}

class DragSource : public GuiElem {
  public:
  DragSource(DragContainer& c, DragContent d, function<PGuiElem()> g) : container(c), content(d), gui(g) {}

  virtual bool onLeftClick(Vec2 v) override {
    if (v.inRectangle(getBounds()))
      container.put(content, gui(), v);
    return false;
  }

  private:
  DragContainer& container;
  DragContent content;
  function<PGuiElem()> gui;
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

PGuiElem GuiFactory::dragSource(DragContent content, function<PGuiElem()> gui) {
  return PGuiElem(new DragSource(dragContainer, content, gui));
}

PGuiElem GuiFactory::dragListener(function<void(DragContent)> fun) {
  return PGuiElem(new OnMouseRelease([this, fun] {
        if (auto content = dragContainer.pop())
          fun(*content);
      }));
}

class TranslateGui : public GuiLayout {
  public:
  TranslateGui(PGuiElem e, Vec2 v, Rectangle nSize)
      : GuiLayout(makeVec<PGuiElem>(std::move(e))), vec(v), newSize(nSize) {
    CHECK(newSize.topLeft() == Vec2(0, 0));
  }

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().topLeft(), newSize.bottomRight()).translate(vec);
  }

  private:
  Vec2 vec;
  Rectangle newSize;
};

PGuiElem GuiFactory::translate(PGuiElem e, Vec2 v, Rectangle newSize) {
  return PGuiElem(new TranslateGui(std::move(e), v, newSize));
}

PGuiElem GuiFactory::onRenderedAction(function<void()> fun) {
  return PGuiElem(new DrawCustom([=] (Renderer& r, Rectangle bounds) { fun(); }));
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

PGuiElem GuiFactory::mouseOverAction(function<void()> callback, function<void()> outCallback) {
  return PGuiElem(new MouseOverAction(callback, outCallback));
}

class MouseHighlightBase : public GuiLayout {
  public:
  MouseHighlightBase(PGuiElem h, int ind, int* highlight)
    : GuiLayout(makeVec<PGuiElem>(std::move(h))), myIndex(ind), highlighted(highlight) {}

  virtual void render(Renderer& r) override {
    if (*highlighted == myIndex)
      elems[0]->render(r);
  }

  protected:
  int myIndex;
  int* highlighted;
  bool canTurnOff = false;
};

class MouseHighlight : public MouseHighlightBase {
  public:
  using MouseHighlightBase::MouseHighlightBase;

  virtual void onMouseGone() override {
    if (*highlighted == myIndex && canTurnOff) {
      *highlighted = -1;
      canTurnOff = false;
    }
  }

  virtual bool onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
      *highlighted = myIndex;
      canTurnOff = true;
    } else if (*highlighted == myIndex && canTurnOff) {
      *highlighted = -1;
      canTurnOff = false;
    }
    return false;
  }
};

class MouseHighlightClick : public MouseHighlightBase {
  public:
  using MouseHighlightBase::MouseHighlightBase;

  virtual bool onLeftClick(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
      *highlighted = myIndex;
      canTurnOff = true;
    } else if (*highlighted == myIndex && canTurnOff) {
      *highlighted = -1;
      canTurnOff = false;
    }
    return false;
  }
};

class MouseHighlight2 : public GuiLayout {
  public:
  MouseHighlight2(PGuiElem h)
    : GuiLayout(makeVec<PGuiElem>(std::move(h))) {}

  virtual void render(Renderer& r) override {
    if (r.getMousePos().inRectangle(getBounds()))
      elems[0]->render(r);
  }
};

PGuiElem GuiFactory::mouseHighlight(PGuiElem elem, int myIndex, int* highlighted) {
  return PGuiElem(new MouseHighlight(std::move(elem), myIndex, highlighted));
}

PGuiElem GuiFactory::mouseHighlightClick(PGuiElem elem, int myIndex, int* highlighted) {
  return PGuiElem(new MouseHighlightClick(std::move(elem), myIndex, highlighted));
}

class MouseHighlightGameChoice : public GuiLayout {
  public:
  MouseHighlightGameChoice(PGuiElem h, optional<GameTypeChoice> my, optional<GameTypeChoice>& highlight)
    : GuiLayout(makeVec<PGuiElem>(std::move(h))), myChoice(my), highlighted(highlight) {}

  virtual void onMouseGone() override {
    if (*highlighted == myChoice)
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
  optional<GameTypeChoice> myChoice;
  optional<GameTypeChoice>& highlighted;
};

PGuiElem GuiFactory::mouseHighlightGameChoice(PGuiElem elem,
    optional<GameTypeChoice> my, optional<GameTypeChoice>& highlight) {
  return PGuiElem(new MouseHighlightGameChoice(std::move(elem), my, highlight));
}

PGuiElem GuiFactory::mouseHighlight2(PGuiElem elem) {
  return PGuiElem(new MouseHighlight2(std::move(elem)));
}

const static int tooltipLineHeight = 28;
const static int tooltipHMargin = 15;
const static int tooltipVMargin = 15;
const static Vec2 tooltipOffset = Vec2(10, 10);

class Tooltip : public GuiElem {
  public:
  Tooltip(const vector<string>& t, PGuiElem bg, Clock* c, milliseconds d) : text(t), background(std::move(bg)),
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
          r.drawText(colors[ColorId::WHITE], pos.x + tooltipHMargin, pos.y + tooltipVMargin + i * tooltipLineHeight,
              text[i]);
        r.popLayer();
      }
    } else 
      lastTimeOut = clock->getRealMillis();
  }

  private:
  bool canRender = false;
  vector<string> text;
  PGuiElem background;
  milliseconds lastTimeOut;
  Clock* clock;
  milliseconds delay;
};

PGuiElem GuiFactory::tooltip(const vector<string>& v, milliseconds delayMilli) {
  if (v.empty() || (v.size() == 1 && v[0].empty()))
    return empty();
  return PGuiElem(new Tooltip(v, stack(background(background1), miniBorder()), clock, delayMilli));
}

const static int notHeld = -1000;

int GuiFactory::getHeldInitValue() {
  return notHeld;
}

class ScrollBar : public GuiLayout {
  public:

  ScrollBar(PGuiElem b, VerticalList* _content, Vec2 butSize, int vMarg, double* scrollP, int contentH, int* h)
      : GuiLayout(makeVec<PGuiElem>(std::move(b))), buttonSize(butSize), vMargin(vMarg),
      scrollPos(scrollP), content(_content), contentHeight(contentH) {
    listSize = content->getSize();
    if (h)
      held = h;
    else
      held = &localHeld;
  }

  virtual Rectangle getElemBounds(int num) override {
    Vec2 center(getBounds().middle().x, calcButHeight());
    return Rectangle(center - buttonSize / 2, center + buttonSize / 2);
  }

  double calcPos(int mouseHeight) {
    return max(0.0, min(1.0, 
          double(mouseHeight - getBounds().top() - vMargin - buttonSize.y / 2)
              / (getBounds().height() - 2 * vMargin - buttonSize.y)));
  }

  int scrollLength() {
    return max(1, content->getLastTopElem(getBounds().height()));
  }

  int calcButHeight() {
    int ret = min<double>(getBounds().bottom() - buttonSize.y / 2 - vMargin,
        double(getBounds().top()) + buttonSize.y / 2 + vMargin + *scrollPos
            * double(getBounds().height() - buttonSize.y - 2 * vMargin)
            / double(scrollLength()));
    return ret;
  }

  virtual bool onMouseWheel(Vec2 v, bool up) override {
    if (v.inRectangle(Rectangle(Vec2(content->getBounds().left(), getBounds().top()),
        getBounds().bottomRight()))) {
      if (up)
        *scrollPos = max<double>(0, *scrollPos - 1);
      else
        *scrollPos = min<double>(scrollLength(), *scrollPos + 1);
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
        *scrollPos = max<double>(0, *scrollPos - 1);
      else if (v.y >= getBounds().bottom() - vMargin)
        *scrollPos = min<double>(scrollLength(), *scrollPos + 1);
      else
        *scrollPos = scrollLength() * calcPos(v.y);
      return true;
    }
    return false;
  }

  virtual bool onMouseMove(Vec2 v) override {
    if (*held != notHeld)
      *scrollPos = scrollLength() * calcPos(v.y - *held);
    return false;
  }

  virtual void onMouseRelease(Vec2) override {
    *held = notHeld;
  }

  virtual bool isVisible(int num) override {
    return getBounds().height() < contentHeight;
  }

  private:
  int* held;
  int localHeld = notHeld;
  Vec2 buttonSize;
  int vMargin;
  double* scrollPos;
  VerticalList* content;
  int listSize;
  int contentHeight;
};

class Scrollable : public GuiElem {
  public:
  Scrollable(PVerticalList c, int cHeight, double* scrollP)
      : content(std::move(c)), contentHeight(cHeight) {
    if (scrollP)
      scrollPos = scrollP;
    else {
      defaultPos.reset(new double);
      scrollPos = defaultPos.get();
      *scrollPos = 0;
    }
  }

  double* getScrollPosPtr() {
    return scrollPos;
  }

  int getScrollOffset() {
    return content->getElemsHeight(getScrollPos());
  }

  double getScrollPos() {
    return min<double>(*scrollPos, content->getLastTopElem(getBounds().height()));
  }

  void onRefreshBounds() override {
    int lastTopElem = content->getLastTopElem(getBounds().height());
    int offset = getScrollOffset();
    if (*scrollPos > lastTopElem)
      *scrollPos = lastTopElem;
    content->setBounds(getBounds().translate(Vec2(0, -offset)));
    contentHeight ++;
  }

  virtual void render(Renderer& r) override {
    content->renderPart(r, getScrollPos());
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
//    if (v.inRectangle(getBounds()))
    return content->onMouseMove(v);
  }

  virtual void onMouseRelease(Vec2 pos) override {
    content->onMouseRelease(pos);
  }

  virtual bool onKeyPressed2(SDL_Keysym key) override {
    return content->onKeyPressed2(key);
  }

  private:
  PVerticalList content;
  int contentHeight;
  double* scrollPos;
  unique_ptr<double> defaultPos;
};

const int border2Width = 6;

const int scrollbarWidth = 22;
const int borderWidth = 8;
const int borderHeight = 11;
const int backgroundSize = 128;

Texture& GuiFactory::get(TexId id) {
  return textures.at(id);
}

void GuiFactory::loadFreeImages(const string& path) {
  textures.emplace(TexId::SCROLLBAR, path + "/ui/scrollbar.png");
  textures.emplace(TexId::SCROLL_BUTTON, path + "/ui/scrollmark.png");
  textures.emplace(TexId::BACKGROUND_PATTERN, path + "/window_bg.png");
  text = colors[ColorId::WHITE];
  titleText = colors[ColorId::YELLOW];
  inactiveText = colors[ColorId::LIGHT_GRAY];
  textures.emplace(TexId::HORI_CORNER1, path + "/ui/horicorner1.png");
  textures.emplace(TexId::HORI_CORNER2, path + "/ui/horicorner2.png");
  textures.emplace(TexId::HORI_LINE, path + "/ui/horiline.png");
  textures.emplace(TexId::HORI_MIDDLE, path + "/ui/horimiddle.png");
  textures.emplace(TexId::VERT_BAR, path + "/ui/vertbar.png");
  textures.emplace(TexId::HORI_BAR, path + "/ui/horibar.png");
  textures.emplace(TexId::CORNER_TOP_LEFT, path + "/ui/cornerTOPL.png");
  textures.emplace(TexId::CORNER_TOP_RIGHT, path + "/ui/cornerTOPR.png");
  textures.emplace(TexId::CORNER_BOTTOM_RIGHT, path + "/ui/cornerBOTTOMR.png");

  textures.emplace(TexId::HORI_BAR_MINI, path + "/ui/horibarmini.png");
  textures.emplace(TexId::VERT_BAR_MINI, path + "/ui/vertbarmini.png");
  textures.emplace(TexId::CORNER_MINI, path + "/ui/cornermini.png");

  textures.emplace(TexId::HORI_BAR_MINI2, path + "/ui/horibarmini2.png");
  textures.emplace(TexId::VERT_BAR_MINI2, path + "/ui/vertbarmini2.png");
  textures.emplace(TexId::CORNER_MINI2, path + "/ui/cornermini2.png");
  textures.emplace(TexId::SCROLL_UP, path + "/ui/up.png");
  textures.emplace(TexId::SCROLL_DOWN, path + "/ui/down.png");
  textures.emplace(TexId::WINDOW_CORNER, path + "/ui/corner1.png");
  textures.emplace(TexId::WINDOW_CORNER_EXIT, path + "/ui/corner2X.png");
  textures.emplace(TexId::WINDOW_VERT_BAR, path + "/ui/vertibarmsg1.png");
  textures.emplace(TexId::MAIN_MENU_HIGHLIGHT, path + "/ui/menu_highlight.png");
  textures.emplace(TexId::SPLASH1, path + "/splash2f.png");
  textures.emplace(TexId::SPLASH2, path + "/splash2e.png");
  textures.emplace(TexId::LOADING_SPLASH, path + "/" + Random.choose(
            "splash2a.png"_s,
            "splash2b.png"_s,
            "splash2c.png"_s,
            "splash2d.png"_s));
  textures.emplace(TexId::UI_HIGHLIGHT, path + "/ui/ui_highlight.png");
  const int tabIconWidth = 42;
  for (int i = 0; i < 8; ++i)
    iconTextures.push_back(Texture(path + "/icons.png", 0, i * tabIconWidth, tabIconWidth, tabIconWidth));
  const int statIconWidth = 18;
  for (int i = 0; i < 6; ++i)
    iconTextures.push_back(Texture(path + "/stat_icons.png",
          i * statIconWidth, 0, statIconWidth, statIconWidth));
  const int moraleIconWidth = 16;
  for (int i = 0; i < 4; ++i)
    iconTextures.push_back(Texture(path + "/morale_icons.png",
          0, i * moraleIconWidth, moraleIconWidth, moraleIconWidth));
  const int teamIconWidth = 16;
  for (int i = 0; i < 2; ++i)
    iconTextures.push_back(Texture(path + "/team_icons.png",
          0, i * teamIconWidth, teamIconWidth, teamIconWidth));
  const int spellIconWidth = 40;
  for (SpellId id : ENUM_ALL(SpellId))
    spellTextures.push_back(Texture(path + "/spells.png",
          0, int(id) * spellIconWidth, spellIconWidth, spellIconWidth));
}

void GuiFactory::loadNonFreeImages(const string& path) {
  textures.emplace(TexId::KEEPER_CHOICE, path + "/keeper_choice.png");
  textures.emplace(TexId::ADVENTURER_CHOICE, path + "/adventurer_choice.png");
  textures.emplace(TexId::KEEPER_HIGHLIGHT, path + "/keeper_highlight.png");
  textures.emplace(TexId::ADVENTURER_HIGHLIGHT, path + "/adventurer_highlight.png");
  textures.emplace(TexId::MENU_ITEM, path + "/barmid.png");
  // If menu_core fails to load, try the lower resolution versions
  if (auto tex = Texture::loadMaybe(path + "/menu_core.png")) {
    textures.emplace(TexId::MENU_CORE, std::move(*tex));
    textures.emplace(TexId::MENU_MOUTH, path + "/menu_mouth.png");
  } else {
    textures.emplace(TexId::MENU_CORE, path + "/menu_core_sm.png");
    textures.emplace(TexId::MENU_MOUTH, path + "/menu_mouth_sm.png");
  }
}

Texture& GuiFactory::getIconTex(IconId id) {
  return iconTextures.at(id);
}

PGuiElem GuiFactory::getScrollbar() {
  return stack(makeVec<PGuiElem>(
        sprite(get(TexId::SCROLLBAR), GuiFactory::Alignment::VERTICAL_CENTER),
        sprite(get(TexId::SCROLL_UP), GuiFactory::Alignment::TOP_RIGHT),
        sprite(get(TexId::SCROLL_DOWN), GuiFactory::Alignment::BOTTOM_RIGHT)));
}

PGuiElem GuiFactory::getScrollButton() {
  return sprite(get(TexId::SCROLL_BUTTON), Alignment::CENTER);
}

Vec2 GuiFactory::getScrollButtonSize() {
  return Vec2(get(TexId::SCROLL_BUTTON).getSize().x, get(TexId::SCROLL_BUTTON).getSize().x);
}

class Conditional : public GuiLayout {
  public:
  Conditional(PGuiElem e, function<bool(GuiElem*)> f) : GuiLayout(makeVec<PGuiElem>(std::move(e))), cond(f) {}

  virtual bool isVisible(int num) override {
    return cond(this);
  }

  protected:
  function<bool(GuiElem*)> cond;
};

PGuiElem GuiFactory::conditional(PGuiElem elem, function<bool()> f) {
  return PGuiElem(new Conditional(std::move(elem), [f](GuiElem*) { return f(); }));
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

PGuiElem GuiFactory::conditionalStopKeys(PGuiElem elem, function<bool()> f) {
  return PGuiElem(new ConditionalStopKeys(std::move(elem), [f](GuiElem*) { return f(); }));
}

PGuiElem GuiFactory::conditional2(PGuiElem elem, function<bool(GuiElem*)> f) {
  return PGuiElem(new Conditional(std::move(elem), f));
}

PGuiElem GuiFactory::conditional2(PGuiElem elem, PGuiElem alter, function<bool(GuiElem*)> f) {
  return stack(PGuiElem(new Conditional(std::move(elem), f)),
      PGuiElem(new Conditional(std::move(alter), [=] (GuiElem* e) { return !f(e);})));
}

PGuiElem GuiFactory::conditional(PGuiElem elem, PGuiElem alter, function<bool()> f) {
  return conditional2(std::move(elem), std::move(alter), [=] (GuiElem*) { return f(); });
}

PGuiElem GuiFactory::scrollable(PGuiElem content, double* scrollPos, int* held) {
  VerticalList* list = dynamic_cast<VerticalList*>(content.release());
  int contentHeight = list->getTotalHeight();
  CHECK(list) << "Scrolling only implemented for vertical list";
  PGuiElem scrollable(new Scrollable(PVerticalList(list), contentHeight, scrollPos));
  scrollPos = ((Scrollable*)(scrollable.get()))->getScrollPosPtr();
  int scrollBarMargin = get(TexId::SCROLL_UP).getSize().y;
  PGuiElem bar(new ScrollBar(
        getScrollButton(), list, getScrollButtonSize(), scrollBarMargin, scrollPos, contentHeight, held));
  PGuiElem barButtons = getScrollbar();
  barButtons = conditional2(std::move(barButtons), [=] (GuiElem* e) {
      return e->getBounds().height() < contentHeight;});
  return maybeMargin(stack(std::move(barButtons), std::move(bar)),
        std::move(scrollable), scrollbarWidth, RIGHT,
        [=] (Rectangle bounds) { return bounds.height() < contentHeight; });
}

PGuiElem GuiFactory::background(Color c) {
  return stack(rectangle(c), repeatedPattern(get(TexId::BACKGROUND_PATTERN)));
}

PGuiElem GuiFactory::highlight(double height) {
  return margins(sprite(get(TexId::MAIN_MENU_HIGHLIGHT), height), -25, 0, 0, 0);
}

PGuiElem GuiFactory::highlightDouble() {
  return stack(
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::LEFT_CENTER),
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::RIGHT_CENTER, false, true));
}

PGuiElem GuiFactory::mainMenuHighlight() {
  return stack(
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::LEFT_STRETCHED),
      sprite(get(TexId::MAIN_MENU_HIGHLIGHT), Alignment::RIGHT_STRETCHED, false, true));
}

PGuiElem GuiFactory::miniBorder() {
  return stack(makeVec<PGuiElem>(
        sprite(get(TexId::HORI_BAR_MINI), Alignment::BOTTOM, true, false),
        sprite(get(TexId::HORI_BAR_MINI), Alignment::TOP, false, false),
        sprite(get(TexId::VERT_BAR_MINI), Alignment::RIGHT, false, true),
        sprite(get(TexId::VERT_BAR_MINI), Alignment::LEFT, false, false),
        sprite(get(TexId::CORNER_MINI), Alignment::BOTTOM_RIGHT, true, true),
        sprite(get(TexId::CORNER_MINI), Alignment::BOTTOM_LEFT, true, false),
        sprite(get(TexId::CORNER_MINI), Alignment::TOP_RIGHT, false, true),
        sprite(get(TexId::CORNER_MINI), Alignment::TOP_LEFT, false, false)));
}

PGuiElem GuiFactory::miniBorder2() {
  return stack(makeVec<PGuiElem>(
        sprite(get(TexId::HORI_BAR_MINI2), Alignment::BOTTOM, false, false),
        sprite(get(TexId::HORI_BAR_MINI2), Alignment::TOP, true, false),
        sprite(get(TexId::VERT_BAR_MINI2), Alignment::RIGHT, false, false),
        sprite(get(TexId::VERT_BAR_MINI2), Alignment::LEFT, false, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::BOTTOM_RIGHT, true, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::BOTTOM_LEFT, true, false),
        sprite(get(TexId::CORNER_MINI2), Alignment::TOP_RIGHT, false, true),
        sprite(get(TexId::CORNER_MINI2), Alignment::TOP_LEFT, false, false)));
}

PGuiElem GuiFactory::miniWindow2(PGuiElem content, function<void()> onExitButton) {
  return stack(fullScreen(darken()),
      miniWindow(margins(std::move(content), 15), onExitButton));
}

PGuiElem GuiFactory::miniWindow(PGuiElem content, function<void()> onExitButton) {
  auto ret = makeVec<PGuiElem>(
        stopMouseMovement(),
        rectangle(colors[ColorId::BLACK]),
        background(background1),
        miniBorder(),
        std::move(content));
  if (onExitButton)
    ret.push_back(reverseButton(onExitButton, {getKey(SDL::SDLK_ESCAPE)}));
  return stack(std::move(ret));
}

PGuiElem GuiFactory::miniWindow() {
  return stack(makeVec<PGuiElem>(
        stopMouseMovement(),
        rectangle(colors[ColorId::BLACK]),
        background(background1),
        miniBorder()));
}

PGuiElem GuiFactory::window(PGuiElem content, function<void()> onExitButton) {
  return stack(makeVec<PGuiElem>(
        stopMouseMovement(),
        alignment(Alignment::TOP_RIGHT, button(onExitButton, getKey(SDL::SDLK_ESCAPE), true), Vec2(38, 38)),
        rectangle(colors[ColorId::BLACK]),
        background(background1),
        margins(std::move(content), 20, 35, 30, 30),
        sprite(get(TexId::HORI_BAR), Alignment::BOTTOM, true, false),
        sprite(get(TexId::HORI_BAR), Alignment::TOP, false, false),
        sprite(get(TexId::WINDOW_VERT_BAR), Alignment::RIGHT, false, false),
        sprite(get(TexId::WINDOW_VERT_BAR), Alignment::LEFT, false, true),
        sprite(get(TexId::WINDOW_CORNER), Alignment::BOTTOM_RIGHT, true, true, Vec2(6, 2)),
        sprite(get(TexId::WINDOW_CORNER), Alignment::BOTTOM_LEFT, true, false, Vec2(-6, 2)),
        sprite(get(TexId::WINDOW_CORNER_EXIT), Alignment::TOP_RIGHT, false, false, Vec2(6, -2)),
        sprite(get(TexId::WINDOW_CORNER), Alignment::TOP_LEFT, false, false, Vec2(-6, -2))
        ));
}

PGuiElem GuiFactory::mainDecoration(int rightBarWidth, int bottomBarHeight, optional<int> topBarHeight) {
  return margin(
      stack(makeVec<PGuiElem>(
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
      stack(makeVec<PGuiElem>(
          margin(background(background1), empty(), bottomBarHeight, BOTTOM),
          sprite(get(TexId::HORI_LINE), Alignment::BOTTOM),
 //         sprite(get(TexId::HORI_MIDDLE), Alignment::BOTTOM_CENTER),
          sprite(get(TexId::HORI_CORNER1), Alignment::BOTTOM_RIGHT, false, true),
          sprite(get(TexId::HORI_CORNER2), Alignment::BOTTOM_LEFT, false, true, Vec2(-93, 0)))),
      rightBarWidth,
      LEFT);
}

PGuiElem GuiFactory::translucentBackground(PGuiElem content) {
  return stack(
      stopMouseMovement(),
      background(std::move(content), translucentBgColor));
}

PGuiElem GuiFactory::translucentBackground() {
  return stack(
      stopMouseMovement(),
      rectangle(translucentBgColor));
}

PGuiElem GuiFactory::background(PGuiElem content, Color color) {
  return stack(rectangle(color), std::move(content));
}

PGuiElem GuiFactory::icon(IconId id, Alignment alignment, Color color) {
  return sprite(getIconTex(id), alignment, color);
}

PGuiElem GuiFactory::spellIcon(SpellId id) {
  return sprite(spellTextures[int(id)], Alignment::CENTER);
}

PGuiElem GuiFactory::uiHighlightMouseOver(Color c) {
  return mouseHighlight2(uiHighlight(c));
}

PGuiElem GuiFactory::uiHighlight(Color c) {
  return uiHighlight([=] { return c; });
}

PGuiElem GuiFactory::uiHighlight(function<Color()> c) {
  return leftMargin(-8, topMargin(-4, sprite(TexId::UI_HIGHLIGHT, Alignment::LEFT_STRETCHED, c)));
}

PGuiElem GuiFactory::uiHighlightConditional(function<bool()> cond, Color c) {
  return conditional(uiHighlight(c), cond);
}

PGuiElem GuiFactory::rectangleBorder(Color col) {
  return rectangle(colors[ColorId::TRANSPARENT], col);
}

PGuiElem GuiFactory::sprite(TexId id, Alignment a, function<Color()> c) {
  return sprite(get(id), a, false, false, Vec2(0, 0), c);
}

PGuiElem GuiFactory::sprite(Texture& t, Alignment a, Color c) {
  return sprite(t, a, false, false, Vec2(0, 0), [=]{ return c; });
}

PGuiElem GuiFactory::mainMenuLabelBg(const string& s, double vPadding, Color color) {
  return stack(
      marginFit(empty(), sprite(get(TexId::MENU_ITEM), Alignment::CENTER_STRETCHED),
          0.12, BOTTOM),
      mainMenuLabel(s, vPadding, color));
}

PGuiElem GuiFactory::darken() {
  return stack(
      stopMouseMovement(),
      rectangle(Color{0, 0, 0, 150}));
}

void GuiFactory::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
  switch (event.type) {
    case SDL::SDL_MOUSEBUTTONUP:
      for (GuiElem* elem : guiElems)
        elem->onMouseRelease(Vec2(event.button.x, event.button.y));
      dragContainer.pop();
      break;
    case SDL::SDL_MOUSEMOTION: {
      bool captured = false;
      for (GuiElem* elem : guiElems)
        if (!captured)
          captured |= elem->onMouseMove(Vec2(event.motion.x, event.motion.y));
        else
          elem->onMouseGone();
      break;}
    case SDL::SDL_MOUSEBUTTONDOWN: {
      Vec2 clickPos(event.button.x, event.button.y);
      for (GuiElem* elem : guiElems) {
        if (event.button.button == SDL_BUTTON_RIGHT)
          if (elem->onRightClick(clickPos))
            break;
        if (event.button.button == SDL_BUTTON_LEFT)
          if (elem->onLeftClick(clickPos))
            break;
      }
      }
      break;
    case SDL::SDL_KEYDOWN:
      for (GuiElem* elem : guiElems)
        if (elem->onKeyPressed2(event.key.keysym))
          break;
      break;
    case SDL::SDL_MOUSEWHEEL:
      for (GuiElem* elem : guiElems)
        if (elem->onMouseWheel(renderer.getMousePos(), event.wheel.y > 0))
          break;
      break;
    default: break;
  }
  for (GuiElem* elem : guiElems)
    elem->onRefreshBounds();
}
