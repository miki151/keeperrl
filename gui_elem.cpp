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

using sf::Color;

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

static bool keyEventEqual(Event::KeyEvent k1, Event::KeyEvent k2) {
  return k1.code == k2.code && k1.shift == k2.shift && k1.control == k2.control && k1.alt == k2.alt;
}

class ButtonKey : public Button {
  public:
  ButtonKey(function<void(Rectangle)> f, Event::KeyEvent key, bool cap) : Button(f), hotkey(key), capture(cap) {}

  virtual bool onKeyPressed2(Event::KeyEvent key) override {
    if (keyEventEqual(hotkey, key)) {
      fun(getBounds());
      return capture;
    }
    return false;
  }

  private:
  Event::KeyEvent hotkey;
  bool capture;
};

GuiFactory::GuiFactory(Renderer& r, Clock* c) : clock(c), renderer(r) {
}

PGuiElem GuiFactory::button(function<void(Rectangle)> fun, Event::KeyEvent hotkey, bool capture) {
  return PGuiElem(new ButtonKey(fun, hotkey, capture));
}

PGuiElem GuiFactory::button(function<void()> fun, Event::KeyEvent hotkey, bool capture) {
  return PGuiElem(new ButtonKey([=](Rectangle) { fun(); }, hotkey, capture));
}

PGuiElem GuiFactory::button(function<void(Rectangle)> fun) {
  return PGuiElem(new Button(fun));
}

PGuiElem GuiFactory::button(function<void()> fun) {
  return PGuiElem(new Button([=](Rectangle) { fun(); }));
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

PGuiElem GuiFactory::reverseButton(function<void()> fun, vector<Event::KeyEvent> hotkey, bool capture) {
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
          r.drawSprite(bounds.getTopLeft(), bounds.getTopLeft(), Vec2(bounds.getW(), bounds.getH()), tex);
        }));
}

PGuiElem GuiFactory::sprite(Texture& tex, double height) {
  return PGuiElem(new DrawCustom(
        [&tex, height] (Renderer& r, Rectangle bounds) {
          Vec2 size(tex.getSize().x, tex.getSize().y);
          r.drawSprite(bounds.getTopLeft(), Vec2(0, 0), size, tex, Color(255, 255, 255, 255),
              Vec2(height * size.x / size.y, height));
        }));
}

PGuiElem GuiFactory::sprite(Texture& tex, Alignment align, bool vFlip, bool hFlip, Vec2 offset, double alpha) {
  return PGuiElem(new DrawCustom(
        [&tex, align, offset, alpha, vFlip, hFlip] (Renderer& r, Rectangle bounds) {
          Vec2 size(tex.getSize().x, tex.getSize().y);
          optional<Vec2> stretchSize;
          Vec2 origin;
          Vec2 pos;
          switch (align) {
            case Alignment::TOP:
              pos = bounds.getTopLeft() + offset;
              size = Vec2(bounds.getW() - 2 * offset.x, size.y);
              break;
            case Alignment::BOTTOM:
              pos = bounds.getBottomLeft() + Vec2(0, -size.y) + offset;
              size = Vec2(bounds.getW() - 2 * offset.x, size.y);
              break;
            case Alignment::RIGHT:
              pos = bounds.getTopRight() + Vec2(-size.x, 0) + offset;
              size = Vec2(size.x, bounds.getH() - 2 * offset.y);
              break;
            case Alignment::LEFT:
              pos = bounds.getTopLeft() + offset;
              size = Vec2(size.x, bounds.getH() - 2 * offset.y);
              break;
            case Alignment::TOP_LEFT:
              pos = bounds.getTopLeft() + offset;
              break;
            case Alignment::TOP_RIGHT:
              pos = bounds.getTopRight() - Vec2(size.x, 0) + offset;
              break;
            case Alignment::BOTTOM_RIGHT:
              pos = bounds.getBottomRight() - size + offset;
              break;
            case Alignment::BOTTOM_LEFT:
              pos = bounds.getBottomLeft() - Vec2(0, size.y) + offset;
              break;
            case Alignment::CENTER:
              pos = bounds.middle() - Vec2(size.x / 2, size.y / 2) + offset;
              break;
            case Alignment::TOP_CENTER:
              pos = (bounds.getTopLeft() + bounds.getTopRight()) / 2 - Vec2(size.x / 2, 0) + offset;
              break;
            case Alignment::LEFT_CENTER:
              pos = (bounds.getTopLeft() + bounds.getBottomLeft()) / 2 - Vec2(0, size.y / 2) + offset;
              break;
            case Alignment::BOTTOM_CENTER:
              pos = (bounds.getBottomLeft() + bounds.getBottomRight()) / 2 - Vec2(size.x / 2, size.y) + offset;
              break;
            case Alignment::RIGHT_CENTER:
              pos = (bounds.getTopRight() + bounds.getBottomRight()) / 2 - Vec2(size.x, size.y / 2) + offset;
              break;
            case Alignment::VERTICAL_CENTER:
              pos = (bounds.getTopLeft() + bounds.getTopRight()) / 2 - Vec2(size.x / 2, 0) + offset;
              size = Vec2(size.x, bounds.getH() - 2 * offset.y);
              break;
            case Alignment::LEFT_STRETCHED:
              stretchSize = size * (double(bounds.getH()) / size.y);
              pos = (bounds.getTopLeft() + bounds.getBottomLeft()) / 2 - Vec2(0, stretchSize->y / 2) + offset;
              break;
            case Alignment::RIGHT_STRETCHED:
              stretchSize = size * (double(bounds.getH()) / size.y);
              pos = (bounds.getTopRight() + bounds.getBottomRight()) / 2
                  - Vec2(stretchSize->x, stretchSize->y / 2) + offset;
              break;
            case Alignment::CENTER_STRETCHED:
              stretchSize = size * (double(bounds.getH()) / size.y);
              pos = (bounds.getTopRight() + bounds.getTopLeft()) / 2 - Vec2(stretchSize->x / 2, 0) + offset;
          }
          if (vFlip) {
            if (stretchSize)
              stretchSize->y *= -1;
            size = Vec2(size.x, -size.y);
            origin = Vec2(0, -size.y);
          }
          if (hFlip) {
            if (stretchSize)
              stretchSize->x *= -1;
            size = Vec2(-size.x, size.y);
            origin = Vec2(-size.x, origin.y);
          }
          r.drawSprite(pos, origin, size, tex, Color(255, 255, 255, alpha * 255), stretchSize);
        }));
}

PGuiElem GuiFactory::label(const string& s, Color c, char hotkey) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawTextWithHotkey(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s, 0);
          r.drawTextWithHotkey(c, bounds.getTopLeft().x, bounds.getTopLeft().y, s, hotkey);
        }, width));
}

PGuiElem GuiFactory::label(const string& s, function<Color()> colorFun) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s);
          r.drawText(colorFun(), bounds.getTopLeft().x, bounds.getTopLeft().y, s);
        }, width));
}

PGuiElem GuiFactory::label(const string& s, int size, Color c) {
  auto width = [=] { return renderer.getTextLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s, Renderer::NONE, size);
          r.drawText(c, bounds.getTopLeft().x, bounds.getTopLeft().y, s, Renderer::NONE, size);
        }, width));
}

static Vec2 getTextPos(Rectangle bounds, Renderer::CenterType center) {
  switch (center) {
    case Renderer::HOR:
      return Vec2(bounds.middle().x, bounds.getTopLeft().y);
    case Renderer::VER:
      return Vec2(bounds.getTopLeft().x, bounds.middle().y);
    case Renderer::HOR_VER:
      return bounds.middle();
    default:
      return bounds.getTopLeft();
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

PGuiElem GuiFactory::variableLabel(function<string()> fun, Renderer::CenterType center, int size, Color c) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          string s = fun();
          Vec2 pos = getTextPos(bounds, center);
          r.drawText(transparency(colors[ColorId::BLACK], 100), pos.x + 1, pos.y + 2, s, center, size);
          r.drawText(c, pos.x, pos.y, s, center, size);
        }));
}

PGuiElem GuiFactory::labelUnicode(const String& s, Color color, int size, Renderer::FontId fontId) {
  return labelUnicode(s, [color] { return color; }, size, fontId);
}

PGuiElem GuiFactory::labelUnicode(const String& s, function<Color()> color, int size, Renderer::FontId fontId) {
  auto width = [=] { return renderer.getUnicodeLength(s); };
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(fontId, size, color(), bounds.getTopLeft().x, bounds.getTopLeft().y, s);
        }, width));
}

class MainMenuLabel : public GuiElem {
  public:
  MainMenuLabel(const string& s, Color c, double vPad) : text(s), color(c), vPadding(vPad) {}

  virtual void render(Renderer& renderer) override {
    int size = (0.9 - 2 * vPadding) * getBounds().getH();
    double height = getBounds().getPY() + vPadding * getBounds().getH();
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

  virtual bool onKeyPressed2(Event::KeyEvent key) override {
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

  virtual optional<int> getPreferredWidth() {
    for (int i : All(elems))
      if (auto width = elems[i]->getPreferredWidth())
        if (getElemBounds(i) == getBounds())
          return *width;
    return none;
  }

  virtual optional<int> getPreferredHeight() {
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

class Focusable : public GuiLayout {
  public:
  Focusable(PGuiElem content, vector<Event::KeyEvent> focus, vector<Event::KeyEvent> defocus, bool& foc) :
      GuiLayout(makeVec<PGuiElem>(std::move(content))), focusEvent(focus), defocusEvent(defocus), focused(foc) {}

  virtual bool onLeftClick(Vec2 pos) override {
    if (focused && !pos.inRectangle(getBounds())) {
      focused = false;
      return true;
    }
    return GuiLayout::onLeftClick(pos);
  }

  virtual bool onKeyPressed2(Event::KeyEvent key) override {
    if (!focused)
      for (auto& elem : focusEvent)
        if (keyEventEqual(elem, key)) {
          focused = true;
          return true;
        }
    if (focused)
      for (auto& elem : defocusEvent)
        if (keyEventEqual(elem, key)) {
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
  vector<Event::KeyEvent> focusEvent;
  vector<Event::KeyEvent> defocusEvent;
  bool& focused;
};

PGuiElem GuiFactory::focusable(PGuiElem content, vector<Event::KeyEvent> focusEvent,
    vector<Event::KeyEvent> defocusEvent, bool& focused) {
  return PGuiElem(new Focusable(std::move(content), focusEvent, defocusEvent, focused));
}

class KeyHandler : public GuiElem {
  public:
  KeyHandler(function<void(Event::KeyEvent)> f, bool cap) : fun(f), capture(cap) {}

  virtual bool onKeyPressed2(Event::KeyEvent key) override {
    fun(key);
    return capture;
  }

  private:
  function<void(Event::KeyEvent)> fun;
  bool capture;
};

class AlignmentGui : public GuiLayout {
  public:
  AlignmentGui(PGuiElem e, Vec2 sz, GuiFactory::Alignment align)
      : GuiLayout(makeVec<PGuiElem>(std::move(e))), alignment(align), size(sz) {}

  virtual Rectangle getElemBounds(int num) override {
    switch (alignment) {
      case GuiFactory::Alignment::TOP_RIGHT:
        return Rectangle(getBounds().getTopRight() + size.mult(Vec2(-1, 0)),
            getBounds().getTopRight() + size.mult(Vec2(0, 1)));
      default: FAIL << "Unhandled";
    }
    return Rectangle();
  }

  private:
  GuiFactory::Alignment alignment;
  Vec2 size;
};

PGuiElem GuiFactory::alignment(PGuiElem content, Vec2 size, GuiFactory::Alignment alignment) {
  return PGuiElem(new AlignmentGui(std::move(content), size, alignment));
}
 
PGuiElem GuiFactory::keyHandler(function<void(Event::KeyEvent)> fun, bool capture) {
  return PGuiElem(new KeyHandler(fun, capture));
}

class KeyHandler2 : public GuiElem {
  public:
  KeyHandler2(function<void()> f, vector<Event::KeyEvent> k, bool cap) : fun(f), key(k), capture(cap) {}

  virtual bool onKeyPressed2(Event::KeyEvent k) override {
    for (auto& elem : key)
      if (keyEventEqual(k, elem)) {
        fun();
        return capture;
      }
    return false;
  }

  private:
  function<void()> fun;
  vector<Event::KeyEvent> key;
  bool capture;
};

PGuiElem GuiFactory::keyHandler(function<void()> fun, vector<Event::KeyEvent> key, bool capture) {
  return PGuiElem(new KeyHandler2(fun, key, capture));
}

class VerticalList : public GuiLayout {
  public:
  VerticalList(vector<PGuiElem> e, vector<int> h, int alignBack = 0)
    : GuiLayout(std::move(e)), heights(h), numAlignBack(alignBack) {
    //CHECK(heights.size() > 0);
    CHECK(heights.size() == elems.size());
  }

  Rectangle getBackPosition(int num) {
    int height = std::accumulate(heights.begin() + num + 1, heights.end(), 0);
    return Rectangle(getBounds().getBottomLeft() - Vec2(0, height + heights[num]), getBounds().getBottomRight() 
        - Vec2(0, height));
  }

  virtual Rectangle getElemBounds(int num) override {
    if (num >= heights.size() - numAlignBack)
      return getBackPosition(num);
    int height = std::accumulate(heights.begin(), heights.begin() + num, 0);
    return Rectangle(getBounds().getTopLeft() + Vec2(0, height), getBounds().getTopRight() 
        + Vec2(0, height + heights[num]));
  }

  void renderPart(Renderer& r, int scrollPos) {
    int totHeight = 0;
    for (int i : Range(scrollPos, heights.size())) {
      if (totHeight + elems[i]->getBounds().getH() > getBounds().getH())
        break;
      elems[i]->render(r);
      totHeight += elems[i]->getBounds().getH();
    }
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

  optional<int> getPreferredWidth() override {
    for (auto& elem : elems)
      if (auto width = elem->getPreferredWidth())
        return width;
    return none;
  }

  optional<int> getPreferredHeight() override {
    return getTotalHeight();
  }

  int getSize() {
    return heights.size();
  }

  int getTotalHeight() {
    return getElemsHeight(getSize());
  }

  protected:
  vector<int> heights;
  int numAlignBack;
};

GuiFactory::ListBuilder GuiFactory::getListBuilder(int defaultSize) {
  return ListBuilder(*this, defaultSize);
}

GuiFactory::ListBuilder::ListBuilder(GuiFactory& g, int defSz) : gui(g), defaultSize(defSz) {}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElem(PGuiElem elem, int size) {
  CHECK(!backElems);
  if (size == 0) {
    CHECK(defaultSize > 0);
    size = defaultSize;
  }
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addElemAuto(PGuiElem elem) {
  CHECK(!backElems);
  int size = *elem->getPreferredWidth();
  elems.push_back(std::move(elem));
  sizes.push_back(size);
  return *this;
}

GuiFactory::ListBuilder& GuiFactory::ListBuilder::addBackElemAuto(PGuiElem elem) {
  ++backElems;
  int size = *elem->getPreferredWidth();
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

bool GuiFactory::ListBuilder::isEmpty() const {
  return sizes.empty();
}

vector<PGuiElem>& GuiFactory::ListBuilder::getAllElems() {
  return elems;
}

PGuiElem GuiFactory::ListBuilder::buildVerticalList() {
  return gui.verticalList(std::move(elems), sizes, backElems);
}

PGuiElem GuiFactory::ListBuilder::buildHorizontalList() {
  return gui.horizontalList(std::move(elems), sizes, backElems);
}

PGuiElem GuiFactory::ListBuilder::buildHorizontalListFit() {
  return gui.horizontalListFit(std::move(elems), 0);
}

PGuiElem GuiFactory::verticalList(vector<PGuiElem> e, vector<int> heights, int numAlignBottom) {
  return PGuiElem(new VerticalList(std::move(e), heights, numAlignBottom));
}

PGuiElem GuiFactory::verticalList(vector<PGuiElem> e, int height, int numAlignBottom) {
  vector<int> heights(e.size(), height);
  return PGuiElem(new VerticalList(std::move(e), heights, numAlignBottom));
}

class VerticalListFit : public GuiLayout {
  public:
  VerticalListFit(vector<PGuiElem> e, double space) : GuiLayout(std::move(e)), spacing(space) {
    CHECK(!elems.empty());
  }

  virtual Rectangle getElemBounds(int num) override {
    int elemHeight = double(getBounds().getH()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().getTopLeft() + Vec2(0, num * (elemHeight * (1.0 + spacing))), 
        getBounds().getTopRight() + Vec2(0, num * (elemHeight * (1.0 + spacing)) + elemHeight));
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
    int elemHeight = double(getBounds().getW()) / (double(elems.size()) * (1.0 + spacing) - spacing);
    return Rectangle(getBounds().getTopLeft() + Vec2(num * (elemHeight * (1.0 + spacing)), 0), 
        getBounds().getBottomLeft() + Vec2(num * (elemHeight * (1.0 + spacing)) + elemHeight, 0));
  }

  protected:
  double spacing;
};


PGuiElem GuiFactory::horizontalListFit(vector<PGuiElem> e, double spacing) {
  return PGuiElem(new HorizontalListFit(std::move(e), spacing));
}

class HorizontalList : public VerticalList {
  public:
  using VerticalList::VerticalList;

  Rectangle getBackPosition(int num) {
    int height = std::accumulate(heights.begin() + num + 1, heights.end(), 0);
    return Rectangle(getBounds().getTopRight() - Vec2(height + heights[num], 0), getBounds().getBottomRight() 
        - Vec2(height, 0));
  }

  virtual Rectangle getElemBounds(int num) override {
    if (num >= heights.size() - numAlignBack)
      return getBackPosition(num);
    int height = std::accumulate(heights.begin(), heights.begin() + num, 0);
    return Rectangle(getBounds().getTopLeft() + Vec2(height, 0), getBounds().getBottomLeft() 
        + Vec2(height + heights[num], 0));
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

PGuiElem GuiFactory::horizontalList(vector<PGuiElem> e, vector<int> widths, int numAlignRight) {
  return PGuiElem(new HorizontalList(std::move(e), widths, numAlignRight));
}

PGuiElem GuiFactory::horizontalList(vector<PGuiElem> e, int width, int numAlignRight) {
  vector<int> widths(e.size(), width);
  return horizontalList(std::move(e), widths, numAlignRight);
}

class VerticalAspect : public GuiLayout {
  public:
  VerticalAspect(PGuiElem e, double r) : GuiLayout(makeVec<PGuiElem>(std::move(e))), ratio(r) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0);
    double width = ratio * getBounds().getH();
    return Rectangle((getBounds().getTopLeft() + getBounds().getTopRight()) / 2 - Vec2(width / 2, 0),
        (getBounds().getBottomLeft() + getBounds().getBottomRight()) / 2 + Vec2(width / 2, 0));
  }

  private:
  double ratio;
};

PGuiElem GuiFactory::verticalAspect(PGuiElem elem, double ratio) {
  return PGuiElem(new VerticalAspect(std::move(elem), ratio));
}

class CenterHoriz : public GuiLayout {
  public:
  CenterHoriz(PGuiElem elem, int w) : GuiLayout(makeVec<PGuiElem>(std::move(elem))), width(w) {}

  virtual Rectangle getElemBounds(int num) override {
    int center = (getBounds().getPX() + getBounds().getKX()) / 2;
    return Rectangle(center - width / 2, getBounds().getPY(), center + width / 2, getBounds().getKY());
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
          return Rectangle(getBounds().getTopLeft(), getBounds().getTopRight() + Vec2(0, width));
        case GuiFactory::LEFT:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomLeft() + Vec2(width, 0));
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().getTopRight() - Vec2(width, 0), getBounds().getBottomRight());
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().getBottomLeft() - Vec2(0, width), getBounds().getBottomRight());
      }
    else
      switch (type) { // the remainder
        case GuiFactory::TOP:
          return Rectangle(getBounds().getTopLeft() + Vec2(0, width), getBounds().getBottomRight());
        case GuiFactory::LEFT:
          return Rectangle(getBounds().getTopLeft() + Vec2(width, 0), getBounds().getBottomRight());
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(width, 0));
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(0, width));
      }
  }

  private:
  int width;
  GuiFactory::MarginType type;
};

PGuiElem GuiFactory::margin(PGuiElem top, PGuiElem rest, int width, MarginType type) {
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
    int w = getBounds().getW();
    int h = getBounds().getH();
    if (num == 0)
      switch (type) { // the margin
        case GuiFactory::TOP:
          return Rectangle(getBounds().getTopLeft(), getBounds().getTopRight() + Vec2(0, width * h));
        case GuiFactory::LEFT:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomLeft() + Vec2(width * w, 0));
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().getTopRight() - Vec2(width * w, 0), getBounds().getBottomRight());
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().getBottomLeft() - Vec2(0, width * h), getBounds().getBottomRight());
      }
    else
      switch (type) { // the remainder
        case GuiFactory::TOP:
          return Rectangle(getBounds().getTopLeft() + Vec2(0, width * h), getBounds().getBottomRight());
        case GuiFactory::LEFT:
          return Rectangle(getBounds().getTopLeft() + Vec2(width * w, 0), getBounds().getBottomRight());
        case GuiFactory::RIGHT:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(width * w, 0));
        case GuiFactory::BOTTOM:
          return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(0, width * h));
      }
  }

  private:
  double width;
  GuiFactory::MarginType type;
};

PGuiElem GuiFactory::marginFit(PGuiElem top, PGuiElem rest, double width, MarginType type) {
  return PGuiElem(new MarginFit(std::move(top), std::move(rest), width, type));
}

class Margins : public GuiLayout {
  public:
  Margins(PGuiElem content, int l, int t, int r, int b)
      : GuiLayout(makeVec<PGuiElem>(std::move(content))), left(l), top(t), right(r), bottom(b) {}

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().getPX() + left, getBounds().getPY() + top,
        getBounds().getKX() - right, getBounds().getKY() - bottom);
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
  ViewObjectGui(const ViewObject& obj, bool sprites) : object(obj), useSprites(sprites) {}
  ViewObjectGui(ViewId id, bool sprites) : object(id), useSprites(sprites) {}
  
  virtual void render(Renderer& renderer) override {
    if (ViewObject* obj = boost::get<ViewObject>(&object))
      renderer.drawViewObject(getBounds().getTopLeft(), *obj, useSprites);
    else
      renderer.drawViewObject(getBounds().getTopLeft(), boost::get<ViewId>(object), useSprites);
  }

  private:
  variant<ViewObject, ViewId> object;
  bool useSprites;
};

PGuiElem GuiFactory::viewObject(const ViewObject& object, bool useSprites) {
  return PGuiElem(new ViewObjectGui(object, useSprites));
}

PGuiElem GuiFactory::viewObject(ViewId id, bool useSprites) {
  return PGuiElem(new ViewObjectGui(id, useSprites));
}

class TranslateGui : public GuiLayout {
  public:
  TranslateGui(PGuiElem e, Vec2 v, Rectangle nSize)
      : GuiLayout(makeVec<PGuiElem>(std::move(e))), vec(v), newSize(nSize) {
    CHECK(newSize.getTopLeft() == Vec2(0, 0));
  }

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().getTopLeft(), newSize.getBottomRight()).translate(vec);
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

  virtual bool onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds())) {
      callback();
      in = true;
    } else if (in && outCallback) {
      in = false;
      outCallback();
    }
    return false;
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
  MouseHighlightGameChoice(PGuiElem h, GameTypeChoice my, optional<GameTypeChoice>& highlight)
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
  GameTypeChoice myChoice;
  optional<GameTypeChoice>& highlighted;
};

PGuiElem GuiFactory::mouseHighlightGameChoice(PGuiElem elem,
    GameTypeChoice my, optional<GameTypeChoice>& highlight) {
  return PGuiElem(new MouseHighlightGameChoice(std::move(elem), my, highlight));
}

PGuiElem GuiFactory::mouseHighlight2(PGuiElem elem) {
  return PGuiElem(new MouseHighlight2(std::move(elem)));
}

const static int tooltipLineHeight = 28;
const static int tooltipHMargin = 15;
const static int tooltipVMargin = 15;
const static Vec2 tooltipOffset = Vec2(10, 10);
const static int tooltipDelay = 700;

class Tooltip : public GuiElem {
  public:
  Tooltip(const vector<string>& t, PGuiElem bg, Clock* c) : text(t), background(std::move(bg)),
      lastTimeOut(c->getRealMillis()), clock(c) {
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
      if (clock->getRealMillis() > lastTimeOut + tooltipDelay) {
        Vec2 size(0, text.size() * tooltipLineHeight + 2 * tooltipVMargin);
        for (const string& t : text)
          size.x = max(size.x, r.getTextLength(t) + 2 * tooltipHMargin);
        Vec2 pos = getBounds().getBottomLeft() + tooltipOffset;
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
  int lastTimeOut;
  Clock* clock;
};

PGuiElem GuiFactory::tooltip(const vector<string>& v) {
  if (v.empty() || (v.size() == 1 && v[0].empty()))
    return empty();
  return PGuiElem(new Tooltip(v, stack(background(background1), miniBorder()), clock));
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
          double(mouseHeight - getBounds().getPY() - vMargin - buttonSize.y / 2)
              / (getBounds().getH() - 2 * vMargin - buttonSize.y)));
  }

  int scrollLength() {
    return max(1, content->getLastTopElem(getBounds().getH()));
  }

  int calcButHeight() {
    int ret = min<double>(getBounds().getKY() - buttonSize.y / 2 - vMargin,
        double(getBounds().getPY()) + buttonSize.y / 2 + vMargin + *scrollPos
            * double(getBounds().getH() - buttonSize.y - 2 * vMargin)
            / double(scrollLength()));
    return ret;
  }

  virtual bool onMouseWheel(Vec2 v, bool up) override {
    if (v.inRectangle(Rectangle(Vec2(content->getBounds().getPX(), getBounds().getPY()),
        getBounds().getBottomRight()))) {
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
      if (v.y <= getBounds().getPY() + vMargin)
        *scrollPos = max<double>(0, *scrollPos - 1);
      else if (v.y >= getBounds().getKY() - vMargin)
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

  virtual bool isVisible(int num) {
    return getBounds().getH() < contentHeight;
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
    return min<double>(*scrollPos, content->getLastTopElem(getBounds().getH()));
  }

  void onRefreshBounds() override {
    int lastTopElem = content->getLastTopElem(getBounds().getH());
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
    if (v.inRectangle(getBounds()))
      return content->onLeftClick(v);
    return false;
  }

  virtual bool onRightClick(Vec2 v) override {
    if (v.inRectangle(getBounds()))
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

  virtual bool onKeyPressed2(Event::KeyEvent key) override {
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
  CHECK(textures[TexId::SCROLLBAR].loadFromFile(path + "/ui/scrollbar.png"));
  textures[TexId::SCROLLBAR].setRepeated(true);
  CHECK(textures[TexId::SCROLL_BUTTON].loadFromFile(path + "/ui/scrollmark.png"));
  int px = 166;
  CHECK(textures[TexId::BORDER_TOP_LEFT].loadFromFile(path + "/frame.png",
        sf::IntRect(px, 0, border2Width, border2Width)));
  CHECK(textures[TexId::BORDER_TOP_RIGHT].loadFromFile(path + "/frame.png",
        sf::IntRect(px + 1 + border2Width, 0, border2Width, border2Width)));
  CHECK(textures[TexId::BORDER_BOTTOM_LEFT].loadFromFile(path + "/frame.png",
        sf::IntRect(px, border2Width + 1, border2Width, border2Width)));
  CHECK(textures[TexId::BORDER_BOTTOM_RIGHT].loadFromFile(path + "/frame.png",
        sf::IntRect(px + 1 + border2Width, border2Width + 1, border2Width, border2Width)));
  CHECK(textures[TexId::BORDER_TOP].loadFromFile(path + "/frame.png",
        sf::IntRect(px + border2Width, 0, 1, border2Width)));
  CHECK(textures[TexId::BORDER_RIGHT].loadFromFile(path + "/frame.png",
        sf::IntRect(px + 1 + border2Width, border2Width, border2Width, 1)));
  CHECK(textures[TexId::BORDER_BOTTOM].loadFromFile(path + "/frame.png",
        sf::IntRect(px + border2Width, border2Width + 1, 1, border2Width)));
  CHECK(textures[TexId::BORDER_LEFT].loadFromFile(path + "/frame.png", sf::IntRect(px, border2Width, border2Width, 1)));
  CHECK(textures[TexId::BACKGROUND_PATTERN].loadFromFile(path + "/window_bg.png"));
  textures[TexId::BACKGROUND_PATTERN].setRepeated(true);
  foreground1 = transparency(Color(0x20, 0x5c, 0x4a), 150);
  translucentBgColor = transparency(Color(0, 0, 0), 150);
  text = colors[ColorId::WHITE];
  titleText = colors[ColorId::YELLOW];
  inactiveText = colors[ColorId::LIGHT_GRAY];
  CHECK(textures[TexId::HORI_CORNER1].loadFromFile(path + "/ui/horicorner1.png"));
  CHECK(textures[TexId::HORI_CORNER2].loadFromFile(path + "/ui/horicorner2.png"));
  CHECK(textures[TexId::HORI_LINE].loadFromFile(path + "/ui/horiline.png"));
  textures[TexId::HORI_LINE].setRepeated(true);
  CHECK(textures[TexId::HORI_MIDDLE].loadFromFile(path + "/ui/horimiddle.png"));
  CHECK(textures[TexId::VERT_BAR].loadFromFile(path + "/ui/vertbar.png"));
  textures[TexId::VERT_BAR].setRepeated(true);
  CHECK(textures[TexId::HORI_BAR].loadFromFile(path + "/ui/horibar.png"));
  textures[TexId::HORI_BAR].setRepeated(true);
  CHECK(textures[TexId::HORI_BAR_MINI].loadFromFile(path + "/ui/horibarmini.png"));
  textures[TexId::HORI_BAR_MINI].setRepeated(true);
  CHECK(textures[TexId::CORNER_TOP_LEFT].loadFromFile(path + "/ui/cornerTOPL.png"));
  CHECK(textures[TexId::CORNER_TOP_RIGHT].loadFromFile(path + "/ui/cornerTOPR.png"));
  CHECK(textures[TexId::CORNER_BOTTOM_RIGHT].loadFromFile(path + "/ui/cornerBOTTOMR.png"));
  CHECK(textures[TexId::VERT_BAR_MINI].loadFromFile(path + "/ui/vertbarmini.png"));
  textures[TexId::VERT_BAR_MINI].setRepeated(true);
  CHECK(textures[TexId::CORNER_MINI].loadFromFile(path + "/ui/cornersmall.png"));
  CHECK(textures[TexId::SCROLL_UP].loadFromFile(path + "/ui/up.png"));
  CHECK(textures[TexId::SCROLL_DOWN].loadFromFile(path + "/ui/down.png"));
  CHECK(textures[TexId::WINDOW_CORNER].loadFromFile(path + "/ui/corner1.png"));
  CHECK(textures[TexId::WINDOW_CORNER_EXIT].loadFromFile(path + "/ui/corner2X.png"));
  CHECK(textures[TexId::WINDOW_VERT_BAR].loadFromFile(path + "/ui/vertibarmsg1.png"));
  textures[TexId::WINDOW_VERT_BAR].setRepeated(true);
  CHECK(textures[TexId::MAIN_MENU_HIGHLIGHT].loadFromFile(path + "/ui/menu_highlight.png"));
  textures[TexId::MAIN_MENU_HIGHLIGHT].setSmooth(true);
  CHECK(textures[TexId::SPLASH1].loadFromFile(path + "/splash2f.png"));
  CHECK(textures[TexId::SPLASH2].loadFromFile(path + "/splash2e.png"));
  CHECK(textures[TexId::LOADING_SPLASH].loadFromFile(path + "/" + Random.choose(LIST(
            "splash2a.png",
            "splash2b.png",
            "splash2c.png",
            "splash2d.png"))));
  const int tabIconWidth = 42;
  for (int i = 0; i < 8; ++i) {
    iconTextures.emplace_back();
    CHECK(iconTextures.back().loadFromFile(path + "/icons.png",
          sf::IntRect(0, i * tabIconWidth, tabIconWidth, tabIconWidth)));
  }
  const int statIconWidth = 18;
  for (int i = 0; i < 6; ++i) {
    iconTextures.emplace_back();
    CHECK(iconTextures.back().loadFromFile(path + "/stat_icons.png",
          sf::IntRect(i * statIconWidth, 0, statIconWidth, statIconWidth)));
  }
  const int moraleIconWidth = 16;
  for (int i = 0; i < 4; ++i) {
    iconTextures.emplace_back();
    CHECK(iconTextures.back().loadFromFile(path + "/morale_icons.png",
          sf::IntRect(0, i * moraleIconWidth, moraleIconWidth, moraleIconWidth)));
  }
  const int teamIconWidth = 16;
  for (int i = 0; i < 2; ++i) {
    iconTextures.emplace_back();
    CHECK(iconTextures.back().loadFromFile(path + "/team_icons.png",
          sf::IntRect(0, i * teamIconWidth, teamIconWidth, teamIconWidth)));
  }
  const int spellIconWidth = 40;
  for (SpellId id : ENUM_ALL(SpellId)) {
    spellTextures.emplace_back();
    CHECK(spellTextures.back().loadFromFile(path + "/spells.png",
          sf::IntRect(0, int(id) * spellIconWidth, spellIconWidth, spellIconWidth)));
  }
}

void GuiFactory::loadNonFreeImages(const string& path) {
  CHECK(textures[TexId::KEEPER_CHOICE].loadFromFile(path + "/keeper_choice.png"));
  CHECK(textures[TexId::ADVENTURER_CHOICE].loadFromFile(path + "/adventurer_choice.png"));
  CHECK(textures[TexId::KEEPER_HIGHLIGHT].loadFromFile(path + "/keeper_highlight.png"));
  CHECK(textures[TexId::ADVENTURER_HIGHLIGHT].loadFromFile(path + "/adventurer_highlight.png"));
  textures[TexId::KEEPER_CHOICE].setSmooth(true);
  textures[TexId::ADVENTURER_CHOICE].setSmooth(true);
  textures[TexId::KEEPER_HIGHLIGHT].setSmooth(true);
  textures[TexId::ADVENTURER_HIGHLIGHT].setSmooth(true);
  CHECK(textures[TexId::MENU_ITEM].loadFromFile(path + "/barmid.png"));
  textures[TexId::MENU_ITEM].setSmooth(true);
  if (textures[TexId::MENU_CORE].loadFromFile(path + "/menu_core.png")) {
    CHECK(textures[TexId::MENU_MOUTH].loadFromFile(path + "/menu_mouth.png"));
  } else {
    CHECK(textures[TexId::MENU_CORE].loadFromFile(path + "/menu_core_sm.png"));
    CHECK(textures[TexId::MENU_MOUTH].loadFromFile(path + "/menu_mouth_sm.png"));
  }
  textures[TexId::MENU_CORE].setSmooth(true);
  textures[TexId::MENU_MOUTH].setSmooth(true);
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
  return Vec2(get(TexId::SCROLL_BUTTON).getSize().x, get(TexId::SCROLL_BUTTON).getSize().y);
}

class Conditional : public GuiLayout {
  public:
  Conditional(PGuiElem e, function<bool(GuiElem*)> f) : GuiLayout(makeVec<PGuiElem>(std::move(e))), cond(f) {}

  virtual bool isVisible(int num) {
    return cond(this);
  }

  private:
  function<bool(GuiElem*)> cond;
};

PGuiElem GuiFactory::conditional(PGuiElem elem, function<bool(GuiElem*)> f) {
  return PGuiElem(new Conditional(std::move(elem), f));
}

PGuiElem GuiFactory::conditional(PGuiElem elem, PGuiElem alter, function<bool(GuiElem*)> f) {
  return stack(PGuiElem(new Conditional(std::move(elem), f)),
      PGuiElem(new Conditional(std::move(alter), [=] (GuiElem* e) { return !f(e);})));
}

PGuiElem GuiFactory::scrollable(PGuiElem content, double* scrollPos, int* held) {
  VerticalList* list = dynamic_cast<VerticalList*>(content.release());
  int contentHeight = list->getTotalHeight();
  CHECK(list) << "Scrolling only implemented for vertical list";
  PGuiElem scrollable(new Scrollable(PVerticalList(list), contentHeight, scrollPos));
  scrollPos = ((Scrollable*)(scrollable.get()))->getScrollPosPtr();
  int scrollBarMargin = get(TexId::SCROLL_UP).getSize().y;
  PGuiElem bar(new ScrollBar(
        std::move(getScrollButton()), list, getScrollButtonSize(), scrollBarMargin, scrollPos, contentHeight, held));
  PGuiElem barButtons = getScrollbar();
  barButtons = conditional(std::move(barButtons), [=] (GuiElem* e) {
      return e->getBounds().getH() < contentHeight;});
  return maybeMargin(stack(std::move(barButtons), std::move(bar)),
        std::move(scrollable), scrollbarWidth, RIGHT,
        [=] (Rectangle bounds) { return bounds.getH() < contentHeight; });
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

PGuiElem GuiFactory::border2(PGuiElem content) {
  double alpha = 1;
  return stack(makeVec<PGuiElem>(std::move(content),
        sprite(get(TexId::BORDER_TOP), Alignment::TOP, false, false, Vec2(border2Width, 0), alpha),
        sprite(get(TexId::BORDER_BOTTOM), Alignment::BOTTOM, false, false, Vec2(border2Width, 0), alpha),
        sprite(get(TexId::BORDER_LEFT), Alignment::LEFT, false, false, Vec2(0, border2Width), alpha),
        sprite(get(TexId::BORDER_RIGHT), Alignment::RIGHT, false, false, Vec2(0, border2Width), alpha),
        sprite(get(TexId::BORDER_TOP_LEFT), Alignment::TOP_LEFT, false, false, Vec2(0, 0), alpha),
        sprite(get(TexId::BORDER_TOP_RIGHT), Alignment::TOP_RIGHT, false, false, Vec2(0, 0), alpha),
        sprite(get(TexId::BORDER_BOTTOM_LEFT), Alignment::BOTTOM_LEFT, false, false, Vec2(0, 0), alpha),
        sprite(get(TexId::BORDER_BOTTOM_RIGHT), Alignment::BOTTOM_RIGHT, false, false, Vec2(0, 0), alpha)));
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

PGuiElem GuiFactory::miniWindow(PGuiElem content) {
  return stack(makeVec<PGuiElem>(
        stopMouseMovement(),
        rectangle(colors[ColorId::BLACK]),
        background(background1),
        miniBorder(),
        std::move(content)));
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
        alignment(button(onExitButton, {sf::Keyboard::Escape}, true), Vec2(38, 38), Alignment::TOP_RIGHT),
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

PGuiElem GuiFactory::mainDecoration(int rightBarWidth, int bottomBarHeight) {
  return margin(
      stack(makeVec<PGuiElem>(
          background(background2),
          sprite(get(TexId::HORI_BAR), Alignment::TOP),
          sprite(get(TexId::HORI_BAR), Alignment::BOTTOM, true),
          margin(sprite(get(TexId::HORI_BAR_MINI), Alignment::BOTTOM), empty(), 85, TOP),
          sprite(get(TexId::VERT_BAR), Alignment::RIGHT, false, true),
          sprite(get(TexId::VERT_BAR), Alignment::LEFT),
          sprite(get(TexId::CORNER_TOP_LEFT), Alignment::TOP_RIGHT, false, true, Vec2(8, 0)),
          sprite(get(TexId::CORNER_TOP_RIGHT), Alignment::TOP_LEFT, false, true),
          sprite(get(TexId::CORNER_BOTTOM_RIGHT), Alignment::BOTTOM_LEFT, false, true)
      )),
      stack(makeVec<PGuiElem>(
          margin(background(background2), empty(), bottomBarHeight, BOTTOM),
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

PGuiElem GuiFactory::icon(IconId id) {
  return sprite(getIconTex(id), Alignment::CENTER);
}

PGuiElem GuiFactory::spellIcon(SpellId id) {
  return sprite(spellTextures[int(id)], Alignment::CENTER);
}

PGuiElem GuiFactory::sprite(TexId id, Alignment a) {
  return sprite(get(id), a);
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
      rectangle(Color(0, 0, 0, 150)));
}

void GuiElem::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
  switch (event.type) {
    case Event::MouseButtonReleased:
      for (GuiElem* elem : guiElems)
        elem->onMouseRelease(Vec2(event.mouseButton.x, event.mouseButton.y));
      break;
    case Event::MouseMoved: {
      bool captured = false;
      for (GuiElem* elem : guiElems)
        if (!captured)
          captured |= elem->onMouseMove(Vec2(event.mouseMove.x, event.mouseMove.y));
        else
          elem->onMouseGone();
      break;}
    case Event::MouseButtonPressed: {
      Vec2 clickPos(event.mouseButton.x, event.mouseButton.y);
      for (GuiElem* elem : guiElems) {
        if (event.mouseButton.button == sf::Mouse::Right)
          if (elem->onRightClick(clickPos))
            break;
        if (event.mouseButton.button == sf::Mouse::Left)
          if (elem->onLeftClick(clickPos))
            break;
      }
      }
      break;
    case Event::KeyPressed:
      for (GuiElem* elem : guiElems)
        if (elem->onKeyPressed2(event.key))
          break;
      break;
    case Event::MouseWheelMoved:
      for (GuiElem* elem : guiElems)
        if (elem->onMouseWheel(Vec2(event.mouseWheel.x, event.mouseWheel.y), event.mouseWheel.delta > 0))
          break;
      break;
    default: break;
  }
  for (GuiElem* elem : guiElems)
    elem->onRefreshBounds();
}
