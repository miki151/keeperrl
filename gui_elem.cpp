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

using sf::Color;

DEF_UNIQUE_PTR(VerticalList);

Rectangle GuiElem::getBounds() {
  return bounds;
}

void GuiElem::setBounds(Rectangle b) {
  bounds = b;
  onRefreshBounds();
}

GuiElem::~GuiElem() {
}

class Button : public GuiElem {
  public:
  Button(function<void()> f, char key) : fun(f), hotkey(key) {}
 // Button(function<void()> f, Event::KeyEvent key) : fun(f), hotkey2(key) {}

  virtual void onLeftClick(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      fun();
  }

  virtual void onKeyPressed(char c) override {
    if (hotkey == c)
      fun();
  }

/*  virtual void onKeyPressed(Event::KeyEvent key) override {
    if (hotkey2.code == key.code && hotkey2.shift == key.shift)
      fun();
  }*/

  private:
  function<void()> fun;
  char hotkey;
//  Event::KeyEvent hotkey2;
};

PGuiElem GuiElem::button(function<void()> fun, char hotkey) {
  return PGuiElem(new Button(fun, hotkey));
}

/*PGuiElem GuiElem::button(function<void()> fun, Event::KeyEvent hotkey) {
  return PGuiElem(new Button(fun, hotkey));
}*/

class DrawCustom : public GuiElem {
  public:
  DrawCustom(function<void(Renderer&, Rectangle)> fun) : drawFun(fun) {}

  virtual void render(Renderer& renderer) override {
    drawFun(renderer, getBounds());
  }

  private:
  function<void(Renderer&, Rectangle)> drawFun;
};

PGuiElem GuiElem::rectangle(Color color, Optional<Color> borderColor) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawFilledRectangle(bounds, color, borderColor);
        }));
}

PGuiElem GuiElem::repeatedPattern(Texture& tex) {
  return PGuiElem(new DrawCustom(
        [&tex] (Renderer& r, Rectangle bounds) {
          r.drawSprite(bounds.getTopLeft(), bounds.getTopLeft(), Vec2(bounds.getW(), bounds.getH()), tex);
        }));
}

PGuiElem GuiElem::sprite(Texture& tex, Alignment align, bool vFlip, bool hFlip, Vec2 offset, double alpha) {
  return PGuiElem(new DrawCustom(
        [&tex, align, offset, alpha, vFlip, hFlip] (Renderer& r, Rectangle bounds) {
          Vec2 size(tex.getSize().x, tex.getSize().y);
          Optional<Vec2> stretchSize;
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

PGuiElem GuiElem::spriteFitVert(Texture& tex, double topMargin, double bottomMargin) {
  return PGuiElem(new DrawCustom(
        [=, &tex] (Renderer& r, Rectangle bounds) {
          double height = double(bounds.getH()) * (1.0 - bottomMargin - topMargin);
          double scale = height / tex.getSize().y;
          int width = scale * tex.getSize().x;
          r.drawSprite(Vec2(bounds.middle().x - width / 2, bounds.getPY() + topMargin * bounds.getH()),
              Vec2(width, height), tex);
        }));
}

PGuiElem GuiElem::label(const string& s, Color c, char hotkey) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawTextWithHotkey(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s, 0);
          r.drawTextWithHotkey(c, bounds.getTopLeft().x, bounds.getTopLeft().y, s, hotkey);
        }));
}

PGuiElem GuiElem::label(const string& s, int size, Color c) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s, false, size);
          r.drawText(c, bounds.getTopLeft().x, bounds.getTopLeft().y, s, false, size);
        }));
}

PGuiElem GuiElem::variableLabel(function<string()> fun, int size, Color c) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          string s = fun();
          r.drawText(transparency(colors[ColorId::BLACK], 100),
            bounds.getTopLeft().x + 1, bounds.getTopLeft().y + 2, s, false, size);
          r.drawText(c, bounds.getTopLeft().x, bounds.getTopLeft().y, s, false, size);
        }));
}

PGuiElem GuiElem::labelUnicode(const String& s, Color color, int size, Renderer::FontId fontId) {
  return PGuiElem(new DrawCustom(
        [=] (Renderer& r, Rectangle bounds) {
          r.drawText(fontId, size, color, bounds.getTopLeft().x, bounds.getTopLeft().y, s, false);
        }));
}

class MainMenuLabel : public GuiElem {
  public:
  MainMenuLabel(const string& s, Color c, double vPad) : text(s), color(c), vPadding(vPad) {}

  virtual void render(Renderer& renderer) override {
    int size = (1.0 - 2 * vPadding) * getBounds().getH();
    double height = getBounds().getPY() + vPadding * getBounds().getH();
    renderer.drawText(color, getBounds().middle().x, height - size / 4, text, true, size);
  }

  private:
  string text;
  Color color;
  double vPadding;
};

PGuiElem GuiElem::mainMenuLabel(const string& s, double vPadding, Color c) {
  return PGuiElem(new MainMenuLabel(s, c, vPadding));
}

class GuiLayout : public GuiElem {
  public:
  GuiLayout(vector<PGuiElem> e) : elems(std::move(e)) {}

  virtual void onLeftClick(Vec2 pos) override {
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->onLeftClick(pos);
  }

  virtual void onRightClick(Vec2 pos) override {
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->onRightClick(pos);
  }

  virtual void onMouseMove(Vec2 pos) override {
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->onMouseMove(pos);
  }

  virtual void onMouseRelease() override {
    for (int i : All(elems))
      if (isVisible(i))
        elems[i]->onMouseRelease();
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

  virtual void onKeyPressed(char key) override {
    for (int i : All(elems))
      elems[i]->onKeyPressed(key);
  }

  virtual void onKeyPressed(Event::KeyEvent key) override {
    for (int i : All(elems))
      elems[i]->onKeyPressed(key);
  }

  virtual Rectangle getElemBounds(int num) {
    return getBounds();
  }

  virtual bool isVisible(int num) {
    return true;
  }

  protected:
  vector<PGuiElem> elems;
};

PGuiElem GuiElem::stack(vector<PGuiElem> elems) {
  return PGuiElem(new GuiLayout(std::move(elems)));
}

PGuiElem GuiElem::stack(PGuiElem g1, PGuiElem g2) {
  return stack(makeVec<PGuiElem>(std::move(g1), std::move(g2)));
}

PGuiElem GuiElem::stack(PGuiElem g1, PGuiElem g2, PGuiElem g3) {
  return stack(makeVec<PGuiElem>(std::move(g1), std::move(g2), std::move(g3)));
}

class VerticalList : public GuiLayout {
  public:
  VerticalList(vector<PGuiElem> e, vector<int> h, int space, int alignBack = 0)
    : GuiLayout(std::move(e)), heights(h), spacing(space), numAlignBack(alignBack) {
    CHECK(heights.size() > 0);
    CHECK(heights.size() == elems.size());
  }

  Rectangle getBackPosition(int num) {
    int height = std::accumulate(heights.begin() + num + 1, heights.end(), 0) + (heights.size() - num) * spacing;
    return Rectangle(getBounds().getBottomLeft() - Vec2(0, height + heights[num]), getBounds().getBottomRight() 
        - Vec2(0, height));
  }

  virtual Rectangle getElemBounds(int num) override {
    if (num >= heights.size() - numAlignBack)
      return getBackPosition(num);
    int height = std::accumulate(heights.begin(), heights.begin() + num, 0) + num * spacing;
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

  int getSize() {
    return heights.size();
  }

  protected:
  vector<int> heights;
  int spacing;
  int numAlignBack;
};

PGuiElem GuiElem::verticalList(vector<PGuiElem> e, vector<int> heights, int spacing) {
  return PGuiElem(new VerticalList(std::move(e), heights, spacing, 0));
}

PGuiElem GuiElem::verticalList(vector<PGuiElem> e, int height, int spacing) {
  vector<int> heights(e.size(), height);
  return PGuiElem(new VerticalList(std::move(e), heights, spacing, 0));
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


PGuiElem GuiElem::verticalListFit(vector<PGuiElem> e, double spacing) {
  return PGuiElem(new VerticalListFit(std::move(e), spacing));
}

class HorizontalList : public VerticalList {
  public:
  using VerticalList::VerticalList;

  Rectangle getBackPosition(int num) {
    int height = std::accumulate(heights.begin() + num + 1, heights.end(), 0) + (heights.size() - num) * spacing;
    return Rectangle(getBounds().getTopRight() - Vec2(height + heights[num], 0), getBounds().getBottomRight() 
        - Vec2(height, 0));
  }

  virtual Rectangle getElemBounds(int num) override {
    if (num >= heights.size() - numAlignBack)
      return getBackPosition(num);
    int height = std::accumulate(heights.begin(), heights.begin() + num, 0) + num * spacing;
    return Rectangle(getBounds().getTopLeft() + Vec2(height, 0), getBounds().getBottomLeft() 
        + Vec2(height + heights[num], 0));
  }
};

PGuiElem GuiElem::horizontalList(vector<PGuiElem> e, vector<int> widths, int spacing, int numAlignRight) {
  return PGuiElem(new HorizontalList(std::move(e), widths, spacing, numAlignRight));
}

PGuiElem GuiElem::horizontalList(vector<PGuiElem> e, int width, int spacing, int numAlignRight) {
  vector<int> widths(e.size(), width);
  return horizontalList(std::move(e), widths, spacing, numAlignRight);
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

PGuiElem GuiElem::centerHoriz(PGuiElem e, int width) {
  return PGuiElem(new CenterHoriz(std::move(e), width));
}

class MarginGui : public GuiLayout {
  public:
  MarginGui(PGuiElem top, PGuiElem rest, int _width, MarginType t)
    : GuiLayout(makeVec<PGuiElem>(std::move(top), std::move(rest))), width(_width), type(t) {}

  virtual Rectangle getElemBounds(int num) override {
    CHECK(num == 0 || num == 1);
    if (num == 0)
      switch (type) { // the margin
        case TOP: return Rectangle(getBounds().getTopLeft(), getBounds().getTopRight() + Vec2(0, width)); break;
        case LEFT: return Rectangle(getBounds().getTopLeft(), getBounds().getBottomLeft() + Vec2(width, 0)); break;
        case RIGHT: return Rectangle(getBounds().getTopRight() - Vec2(width, 0), getBounds().getBottomRight()); break;
        case BOTTOM: return Rectangle(getBounds().getBottomLeft() - Vec2(0, width), getBounds().getBottomRight());
                       break;
      }
    else
      switch (type) { // the remainder
        case TOP: return Rectangle(getBounds().getTopLeft() + Vec2(0, width), getBounds().getBottomRight()); break;
        case LEFT: return Rectangle(getBounds().getTopLeft() + Vec2(width, 0), getBounds().getBottomRight()); break;
        case RIGHT: return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(width, 0)); break;
        case BOTTOM: return Rectangle(getBounds().getTopLeft(), getBounds().getBottomRight() - Vec2(0, width));
                     break;
      }
    FAIL << "Ff";
    return Rectangle(1, 1);
  }

  private:
  int width;
  MarginType type;
};

PGuiElem GuiElem::margin(PGuiElem top, PGuiElem rest, int width, MarginType type) {
  return PGuiElem(new MarginGui(std::move(top), std::move(rest), width, type));
}

class Margins : public GuiLayout {
  public:
  Margins(PGuiElem content, int l, int t, int r, int b)
      : GuiLayout(makeVec<PGuiElem>(std::move(content))), left(l), top(t), right(r), bottom(b) {}

  virtual Rectangle getElemBounds(int num) override {
    return Rectangle(getBounds().getPX() + left, getBounds().getPY() + top,
        getBounds().getKX() - right, getBounds().getKY() - bottom);
  }

  private:
  int left;
  int top;
  int right;
  int bottom;
};

PGuiElem GuiElem::margins(PGuiElem content, int left, int top, int right, int bottom) {
  return PGuiElem(new Margins(std::move(content), left, top, right, bottom));
}

class Invisible : public GuiLayout {
  public:
  Invisible(PGuiElem content) : GuiLayout(makeVec<PGuiElem>(std::move(content))) {}

  virtual bool isVisible(int num) {
    return false;
  }
};

PGuiElem GuiElem::invisible(PGuiElem content) {
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

/*class TabGui : public MarginGui {
  public:
  TabGui(vector<PGuiElem> tabs, vector<PGuiElem> content, int tabWidth, int tabHeight, int tabSpacing)
      : MarginGui(horizontalList(makeTabButtons(std::move(tabs)), tabWidth, tabSpacing), 
          PGuiElem(new Switchable(std::move(content), [this]() { return currentTab; })), tabHeight, MarginGui::TOP) {}

  vector<PGuiElem> makeTabButtons(vector<PGuiElem> tabs) {
    vector<PGuiElem> ret;
    for (int i : All(tabs))
      ret.push_back(GuiElem::stack(std::move(tabs[i]), GuiElem::button([this, i]() { currentTab = i; })));
    return ret;
  }

  private:
  vector<PGuiElem> content;
  int contentMargin;
  int currentTab = 0;
};

PGuiElem GuiElem::tabs(vector<PGuiElem> but, vector<PGuiElem> tabs, int tabWidth, int tabHeight, int tabSpacing) {
  return PGuiElem(new TabGui(std::move(but), std::move(tabs), tabWidth, tabHeight, tabSpacing));
}*/

PGuiElem GuiElem::empty() {
  return PGuiElem(new GuiElem());
}

class ViewObjectGui : public GuiElem {
  public:
  ViewObjectGui(const ViewObject& obj, bool sprites) : object(obj), useSprites(sprites) {}
  ViewObjectGui(ViewId id, bool sprites) : object(id), useSprites(sprites) {}
  
  virtual void render(Renderer& renderer) override {
    int x = getBounds().getTopLeft().x;
    int y = getBounds().getTopLeft().y;
    if (ViewObject* obj = boost::get<ViewObject>(&object))
      renderer.drawViewObject(x, y, *obj, useSprites, 0.6666);
    else
      renderer.drawViewObject(x, y, boost::get<ViewId>(object), useSprites, 0.6666);
  }

  private:
  variant<ViewObject, ViewId> object;
  bool useSprites;
};

PGuiElem GuiElem::viewObject(const ViewObject& object, bool useSprites) {
  return PGuiElem(new ViewObjectGui(object, useSprites));
}

PGuiElem GuiElem::viewObject(ViewId id, bool useSprites) {
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

PGuiElem GuiElem::translate(PGuiElem e, Vec2 v, Rectangle newSize) {
  return PGuiElem(new TranslateGui(std::move(e), v, newSize));
}

class MouseOverAction : public GuiElem {
  public:
  MouseOverAction(function<void()> f) : callback(f) {}

  virtual void onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      callback();
  }

  private:
  function<void()> callback;
};

PGuiElem GuiElem::mouseOverAction(function<void()> callback) {
  return PGuiElem(new MouseOverAction(callback));
}

class MouseHighlight : public GuiLayout {
  public:
  MouseHighlight(PGuiElem h, int ind, int* highlight)
    : GuiLayout(makeVec<PGuiElem>(std::move(h))), myIndex(ind), highlighted(highlight) {}

  virtual void onMouseMove(Vec2 pos) override {
    if (pos.inRectangle(getBounds()))
      *highlighted = myIndex;
    else if (*highlighted == myIndex)
      *highlighted = -1;
  }

  virtual void render(Renderer& r) override {
    if (*highlighted == myIndex)
      elems[0]->render(r);
  }

  private:
  int myIndex;
  int* highlighted;
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

PGuiElem GuiElem::mouseHighlight(PGuiElem elem, int myIndex, int* highlighted) {
  return PGuiElem(new MouseHighlight(std::move(elem), myIndex, highlighted));
}

PGuiElem GuiElem::mouseHighlight2(PGuiElem elem) {
  return PGuiElem(new MouseHighlight2(std::move(elem)));
}

class ScrollBar : public GuiLayout {
  public:
  ScrollBar(PGuiElem b, VerticalList* _content, int butHeight, int vMarg, int* scrollP, int contentH)
      : GuiLayout(makeVec<PGuiElem>(std::move(b))), buttonHeight(butHeight), vMargin(vMarg), scrollPos(scrollP),
      content(_content), contentHeight(contentH) {
    listSize = content->getSize();
  }

  virtual Rectangle getElemBounds(int num) override {
    Vec2 topLeft(9 + getBounds().getPX(), calcButHeight());
    return Rectangle(topLeft, topLeft + Vec2(getBounds().getW(), buttonHeight));
  }

  double calcPos(int mouseHeight) {
    return max(0.0, min(1.0, 
          double(mouseHeight - getBounds().getPY() - vMargin) / (getBounds().getH() - 2 * vMargin - buttonHeight)));
  }

  int scrollLength() {
    return content->getLastTopElem(getBounds().getH());
  }

  int calcButHeight() {
    int ret = min<double>(getBounds().getKY() - buttonHeight - vMargin,
        double(getBounds().getPY()) + vMargin + *scrollPos * double(getBounds().getH() - buttonHeight - 2 * vMargin)
      / double(scrollLength()));
    return ret;
  }

  virtual void onLeftClick(Vec2 v) override {
    if (v.y <= getBounds().getPY() + vMargin)
      *scrollPos = max(0, *scrollPos - 1);
    else if (v.y >= getBounds().getKY() - vMargin)
      *scrollPos = min(scrollLength(), *scrollPos + 1);
    else if (v.inRectangle(getElemBounds(0)))
      held = v.y - calcButHeight();
    else if (v.inRectangle(getBounds())) {
      *scrollPos = scrollLength() * calcPos(v.y);
    }
  }

  virtual void onMouseMove(Vec2 v) override {
    if (held)
      *scrollPos = scrollLength() * calcPos(v.y - *held);
  }

  virtual void onMouseRelease() override {
    held = Nothing();
  }

  virtual bool isVisible(int num) {
    return getBounds().getH() < contentHeight;
  }

  private:
  int buttonHeight;
  int vMargin;
  int* scrollPos;
  Optional<int> held;
  VerticalList* content;
  int listSize;
  int contentHeight;
};

class Scrollable : public GuiElem {
  public:
  Scrollable(PVerticalList c, int cHeight, int* scrollP)
      : content(std::move(c)), contentHeight(cHeight), scrollPos(scrollP) {
  }

  int getScrollOffset() {
    return content->getElemsHeight(getScrollPos());
  }

  int getScrollPos() {
    return min(*scrollPos, content->getLastTopElem(getBounds().getH()));
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

  virtual void onLeftClick(Vec2 v) override {
    if (v.inRectangle(getBounds()))
      content->onLeftClick(v);
  }

  virtual void onRightClick(Vec2 v) override {
    if (v.inRectangle(getBounds()))
      content->onRightClick(v);
  }

  virtual void onMouseMove(Vec2 v) override {
//    if (v.inRectangle(getBounds()))
      content->onMouseMove(v);
  }

  virtual void onMouseRelease() override {
    content->onMouseRelease();
  }

  virtual void onKeyPressed(char c) override {
    content->onKeyPressed(c);
  }

  virtual void onKeyPressed(Event::KeyEvent key) override {
    content->onKeyPressed(key);
  }

  private:
  PVerticalList content;
  int contentHeight;
  int* scrollPos;
};

enum TexId {
  SCROLLBAR,
  SCROLL_BUTTON,
  BACKGROUND_TOP_CORNER,
  BACKGROUND_BOTTOM_CORNER,
  BACKGROUND_TOP,
  BACKGROUND_LEFT,
  BACKGROUND_PATTERN,
  BORDER_TOP_LEFT,
  BORDER_TOP_RIGHT,
  BORDER_BOTTOM_LEFT,
  BORDER_BOTTOM_RIGHT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  BORDER_LEFT,
  HORI_CORNER1,
  HORI_CORNER2,
  HORI_LINE,
  HORI_MIDDLE,
  VERT_BAR,
  HORI_BAR,
  HORI_BAR_MINI,
  CORNER_TOP_LEFT,
  CORNER_TOP_RIGHT,
  CORNER_BOTTOM_RIGHT,
  SCROLL_UP,
  SCROLL_DOWN,
  WINDOW_CORNER,
  WINDOW_VERT_BAR,
  MAIN_MENU_HIGHLIGHT,
};

const int border2Width = 6;

const int scrollbarWidth = 22;
const int borderWidth = 8;
const int borderHeight = 11;
const int backgroundSize = 128;
const int iconWidth = 42;

Color GuiElem::background1;
Color GuiElem::background2;
Color GuiElem::foreground1;
Color GuiElem::text;
Color GuiElem::titleText;
Color GuiElem::inactiveText;
Color GuiElem::translucentBgColor;

Texture& get(TexId id) {
  static map<TexId, Texture> m;
  if (m.empty()) {
    m[BACKGROUND_TOP_CORNER].loadFromFile("frame.png", sf::IntRect(0, 128, backgroundSize, backgroundSize));
    m[BACKGROUND_BOTTOM_CORNER].loadFromFile("frame.png", sf::IntRect(0, 128, backgroundSize, backgroundSize));
    m[BACKGROUND_LEFT].loadFromFile("frame.png", sf::IntRect(0, 128 + backgroundSize - 1, backgroundSize, 1));
    m[BACKGROUND_TOP].loadFromFile("frame.png", sf::IntRect(127, 128, 1, backgroundSize));
    m[SCROLLBAR].loadFromFile("ui/scrollbar.png");
    m[SCROLLBAR].setRepeated(true);
    m[SCROLL_BUTTON].loadFromFile("ui/scrollmark.png");
    int px = 166;
    m[BORDER_TOP_LEFT].loadFromFile("frame.png", sf::IntRect(px, 0, border2Width, border2Width));
    m[BORDER_TOP_RIGHT].loadFromFile("frame.png",
        sf::IntRect(px + 1 + border2Width, 0, border2Width, border2Width));
    m[BORDER_BOTTOM_LEFT].loadFromFile("frame.png",
        sf::IntRect(px, border2Width + 1, border2Width, border2Width));
    m[BORDER_BOTTOM_RIGHT].loadFromFile("frame.png",
        sf::IntRect(px + 1 + border2Width, border2Width + 1, border2Width, border2Width));
    m[BORDER_TOP].loadFromFile("frame.png", sf::IntRect(px + border2Width, 0, 1, border2Width));
    m[BORDER_RIGHT].loadFromFile("frame.png", sf::IntRect(px + 1 + border2Width, border2Width, border2Width, 1));
    m[BORDER_BOTTOM].loadFromFile("frame.png", sf::IntRect(px + border2Width, border2Width + 1, 1, border2Width));
    m[BORDER_LEFT].loadFromFile("frame.png", sf::IntRect(px, border2Width, border2Width, 1));
    m[BACKGROUND_PATTERN].loadFromFile("tekstuur_1.png");
    m[BACKGROUND_PATTERN].setRepeated(true);
    GuiElem::foreground1 = transparency(Color(0x20, 0x5c, 0x4a), 150);
    GuiElem::translucentBgColor = transparency(Color(0, 0, 0), 150);
    GuiElem::text = colors[ColorId::WHITE];
    GuiElem::titleText = colors[ColorId::YELLOW];
    GuiElem::inactiveText = colors[ColorId::LIGHT_GRAY];
    m[HORI_CORNER1].loadFromFile("ui/horicorner1.png");
    m[HORI_CORNER2].loadFromFile("ui/horicorner2.png");
    m[HORI_LINE].loadFromFile("ui/horiline.png");
    m[HORI_LINE].setRepeated(true);
    m[HORI_MIDDLE].loadFromFile("ui/horimiddle.png");
    m[VERT_BAR].loadFromFile("ui/vertbar.png");
    m[VERT_BAR].setRepeated(true);
    m[HORI_BAR].loadFromFile("ui/horibar.png");
    m[HORI_BAR].setRepeated(true);
    m[HORI_BAR_MINI].loadFromFile("ui/horibarmini.png");
    m[HORI_BAR_MINI].setRepeated(true);
    m[CORNER_TOP_LEFT].loadFromFile("ui/cornerTOPL.png");
    m[CORNER_TOP_RIGHT].loadFromFile("ui/cornerTOPR.png");
    m[CORNER_BOTTOM_RIGHT].loadFromFile("ui/cornerBOTTOMR.png");
    m[SCROLL_UP].loadFromFile("ui/up.png");
    m[SCROLL_DOWN].loadFromFile("ui/down.png");
    m[WINDOW_CORNER].loadFromFile("ui/corner1.png");
    m[WINDOW_VERT_BAR].loadFromFile("ui/vertibarmsg1.png");
    m[WINDOW_VERT_BAR].setRepeated(true);
    m[MAIN_MENU_HIGHLIGHT].loadFromFile("ui/menu_highlight.png");
    m[MAIN_MENU_HIGHLIGHT].setSmooth(true);
  }
  return m[id];
}

Texture& getIconTex(GuiElem::IconId id) {
  static vector<Texture> v;
  if (v.empty()) {
    for (int i = 0; i < 8; ++i) {
      v.emplace_back();
      v.back().loadFromFile("icons.png", sf::IntRect(0, i * iconWidth, iconWidth, iconWidth));
    }
  }
  int num = int(id);
  CHECK(num >= 0 && num < v.size());
  return v[num];
}

void GuiElem::changeBackground(int r, int g, int b) {
  background1.r += r;
  background1.g += g;
  background1.b += b;
  background2 = background1;
  Debug() << "New color " << background1.r << " " << background1.g << " " << background1.b << " ";
}

void GuiElem::setBackground(int r, int g, int b) {
  background1.r = r;
  background1.g = g;
  background1.b = b;
  background2 = background1;
  Debug() << "New color " << background1.r << " " << background1.g << " " << background1.b << " ";
}

static PGuiElem getScrollbar() {
  return GuiElem::stack(makeVec<PGuiElem>(
        GuiElem::sprite(get(SCROLLBAR), GuiElem::Alignment::VERTICAL_CENTER),
        GuiElem::sprite(get(SCROLL_UP), GuiElem::Alignment::TOP_RIGHT),
        GuiElem::sprite(get(SCROLL_DOWN), GuiElem::Alignment::BOTTOM_RIGHT)));
}

PGuiElem GuiElem::getScrollButton() {
  return sprite(get(SCROLL_BUTTON), Alignment::TOP_RIGHT);
}

int getScrollButtonSize() {
  return get(SCROLL_BUTTON).getSize().y;
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

PGuiElem GuiElem::conditional(PGuiElem elem, function<bool(GuiElem*)> f) {
  return PGuiElem(new Conditional(std::move(elem), f));
}

PGuiElem GuiElem::conditional(PGuiElem elem, PGuiElem alter, function<bool(GuiElem*)> f) {
  return stack(PGuiElem(new Conditional(std::move(elem), f)),
      PGuiElem(new Conditional(std::move(alter), [=] (GuiElem* e) { return !f(e);})));
}

PGuiElem GuiElem::scrollable(PGuiElem content, int contentHeight, int* scrollPos) {
  VerticalList* list = dynamic_cast<VerticalList*>(content.release());
  CHECK(list) << "Scrolling only implemented for vertical list";
  PGuiElem scrollable(new Scrollable(PVerticalList(list), contentHeight, scrollPos));
  int scrollBarMargin = get(SCROLL_UP).getSize().y;
  PGuiElem bar(new ScrollBar(
        std::move(getScrollButton()), list, getScrollButtonSize(), scrollBarMargin, scrollPos, contentHeight));
  PGuiElem barButtons = GuiElem::stack(makeVec<PGuiElem>(std::move(getScrollbar()),
    GuiElem::margin(GuiElem::empty(),
      GuiElem::button([scrollPos, list] {*scrollPos = min(list->getSize() - 1, *scrollPos + 1); }),
      scrollBarMargin, TOP),
    GuiElem::margin(GuiElem::empty(),
      GuiElem::button([scrollPos] {*scrollPos = max(0, *scrollPos - 1); }),
      scrollBarMargin, BOTTOM)));
  int hMargin = 30;
  int vMargin = 0;
  barButtons = conditional(std::move(barButtons), [=] (GuiElem* e) {
      return e->getBounds().getH() < contentHeight;});
  return margin(stack(std::move(barButtons), std::move(bar)),
        margins(std::move(scrollable), hMargin, vMargin, hMargin, vMargin), scrollbarWidth, RIGHT);
}

PGuiElem GuiElem::background(Color c) {
  return stack(rectangle(c), repeatedPattern(get(BACKGROUND_PATTERN)));
}

PGuiElem GuiElem::highlight(Color c) {
  return margins(sprite(get(MAIN_MENU_HIGHLIGHT), Alignment::LEFT_STRETCHED, false, true), -25, 0, 0, 0);
}

PGuiElem GuiElem::mainMenuHighlight() {
  return stack(
      sprite(get(MAIN_MENU_HIGHLIGHT), Alignment::LEFT_STRETCHED),
      sprite(get(MAIN_MENU_HIGHLIGHT), Alignment::RIGHT_STRETCHED, false, true));
}

PGuiElem GuiElem::border2(PGuiElem content) {
  double alpha = 1;
  return stack(makeVec<PGuiElem>(std::move(content),
        sprite(get(BORDER_TOP), Alignment::TOP, false, false, Vec2(border2Width, 0), alpha),
        sprite(get(BORDER_BOTTOM), Alignment::BOTTOM, false, false, Vec2(border2Width, 0), alpha),
        sprite(get(BORDER_LEFT), Alignment::LEFT, false, false, Vec2(0, border2Width), alpha),
        sprite(get(BORDER_RIGHT), Alignment::RIGHT, false, false, Vec2(0, border2Width), alpha),
        sprite(get(BORDER_TOP_LEFT), Alignment::TOP_LEFT, false, false, Vec2(0, 0), alpha),
        sprite(get(BORDER_TOP_RIGHT), Alignment::TOP_RIGHT, false, false, Vec2(0, 0), alpha),
        sprite(get(BORDER_BOTTOM_LEFT), Alignment::BOTTOM_LEFT, false, false, Vec2(0, 0), alpha),
        sprite(get(BORDER_BOTTOM_RIGHT), Alignment::BOTTOM_RIGHT, false, false, Vec2(0, 0), alpha)));
}

PGuiElem GuiElem::insideBackground(PGuiElem content) {
  int cornerSide = get(BACKGROUND_TOP_CORNER).getSize().x;
  int cornerDown = get(BACKGROUND_TOP_CORNER).getSize().y;
  return stack(makeVec<PGuiElem>(
        std::move(content),
        margins(sprite(get(BACKGROUND_TOP), Alignment::TOP), cornerSide, 0, cornerSide, 0),
        margins(sprite(get(BACKGROUND_TOP), Alignment::BOTTOM, true), cornerSide, 0, cornerSide, 0),
        margins(sprite(get(BACKGROUND_LEFT), Alignment::LEFT), 0, cornerSide, 0, cornerSide),
        margins(sprite(get(BACKGROUND_LEFT), Alignment::RIGHT, false, true), 0, cornerSide, 0, cornerSide),
        sprite(get(BACKGROUND_TOP_CORNER), Alignment::TOP_LEFT),
        sprite(get(BACKGROUND_TOP_CORNER), Alignment::TOP_RIGHT, false, true),
        sprite(get(BACKGROUND_BOTTOM_CORNER), Alignment::BOTTOM_LEFT, true),
        sprite(get(BACKGROUND_BOTTOM_CORNER), Alignment::BOTTOM_RIGHT, true, true)));
}

PGuiElem GuiElem::border(PGuiElem content) {
  return stack(makeVec<PGuiElem>(std::move(content),
        sprite(get(HORI_BAR), Alignment::BOTTOM, true, false),
        sprite(get(HORI_BAR), Alignment::TOP, false, false),
        sprite(get(WINDOW_VERT_BAR), Alignment::RIGHT, false, false),
        sprite(get(WINDOW_VERT_BAR), Alignment::LEFT, false, true),
        sprite(get(WINDOW_CORNER), Alignment::BOTTOM_RIGHT, true, true, Vec2(6, 2)),
        sprite(get(WINDOW_CORNER), Alignment::BOTTOM_LEFT, true, false, Vec2(-6, 2)),
        sprite(get(WINDOW_CORNER), Alignment::TOP_RIGHT, false, true, Vec2(6, -2)),
        sprite(get(WINDOW_CORNER), Alignment::TOP_LEFT, false, false, Vec2(-6, -2))));
}

PGuiElem GuiElem::window(PGuiElem content) {
  return border(stack(makeVec<PGuiElem>(
        rectangle(colors[ColorId::BLACK]),
        insideBackground(stack(background(background1),
        margins(std::move(content), 20, 35, 30, 30))))));
}

PGuiElem GuiElem::mainDecoration(int rightBarWidth, int bottomBarHeight) {
  return margin(
      stack(makeVec<PGuiElem>(
          background(background2),
          sprite(get(HORI_BAR), Alignment::TOP),
          sprite(get(HORI_BAR), Alignment::BOTTOM, true),
          margin(sprite(get(HORI_BAR_MINI), Alignment::BOTTOM), empty(), 85, TOP),
          sprite(get(VERT_BAR), Alignment::RIGHT, false, true),
          sprite(get(VERT_BAR), Alignment::LEFT),
          sprite(get(CORNER_TOP_LEFT), Alignment::TOP_LEFT, false, false, Vec2(-8, 0)),
          sprite(get(CORNER_TOP_RIGHT), Alignment::TOP_RIGHT),
          sprite(get(CORNER_BOTTOM_RIGHT), Alignment::BOTTOM_RIGHT)
      )),
      stack(makeVec<PGuiElem>(
          margin(background(background2), empty(), bottomBarHeight, BOTTOM),
          sprite(get(HORI_LINE), Alignment::BOTTOM),
 //         sprite(get(HORI_MIDDLE), Alignment::BOTTOM_CENTER),
          sprite(get(HORI_CORNER1), Alignment::BOTTOM_LEFT),
          sprite(get(HORI_CORNER2), Alignment::BOTTOM_RIGHT, false, false, Vec2(93, 0)))),
      rightBarWidth,
      RIGHT);
}

PGuiElem GuiElem::translucentBackground(PGuiElem content) {
  return stack(GuiElem::rectangle(translucentBgColor), std::move(content));
}

PGuiElem GuiElem::icon(IconId id) {
  return sprite(getIconTex(id), Alignment::CENTER);
}
