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

#ifndef _GUI_ELEM
#define _GUI_ELEM

#include "renderer.h"
#include "view.h"

class ViewObject;
class Clock;

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  virtual bool onLeftClick(Vec2) { return false; }
  virtual bool onRightClick(Vec2) { return false; }
  virtual void onMouseMove(Vec2) {}
  virtual void onMouseRelease() {}
  virtual void onRefreshBounds() {}
  virtual void onKeyPressed(char) {}
  virtual void onKeyPressed2(Event::KeyEvent) {}
  virtual bool onMouseWheel(Vec2 mousePos, bool up) { return false;}

  void setBounds(Rectangle);
  Rectangle getBounds();

  virtual ~GuiElem();

  static void propagateEvent(const Event&, vector<GuiElem*>);

  private:
  Rectangle bounds;
};

class GuiFactory {
  public:
  GuiFactory(Clock*);
  void loadFreeImages(const string& path);
  void loadNonFreeImages(const string& path);

  PGuiElem button(function<void()> fun, char hotkey = 0);
  PGuiElem button(function<void()> fun, Event::KeyEvent);
  PGuiElem button(function<void(Rectangle buttonBounds)> fun, char hotkey = 0);
  PGuiElem button(function<void(Rectangle buttonBounds)> fun, Event::KeyEvent);
  PGuiElem mouseWheel(function<void(bool)>);
  PGuiElem stack(vector<PGuiElem>);
  PGuiElem stack(PGuiElem, PGuiElem);
  PGuiElem stack(PGuiElem, PGuiElem, PGuiElem);
  PGuiElem rectangle(sf::Color color, optional<sf::Color> borderColor = none);
  PGuiElem verticalList(vector<PGuiElem>, int elemHeight, int spacing);
  PGuiElem verticalList(vector<PGuiElem>, vector<int> elemHeight, int spacing);
  PGuiElem verticalListFit(vector<PGuiElem>, double spacing);
  PGuiElem horizontalList(vector<PGuiElem>, int elemWidth, int spacing, int numAlignRight = 0);
  PGuiElem horizontalList(vector<PGuiElem>, vector<int> elemWidth, int spacing, int numAlignRight = 0);
  PGuiElem horizontalListFit(vector<PGuiElem>, double spacing);
  PGuiElem verticalAspect(PGuiElem, double ratio);
  PGuiElem empty();
  enum MarginType { TOP, LEFT, RIGHT, BOTTOM};
  PGuiElem margin(PGuiElem top, PGuiElem rest, int height, MarginType);
  PGuiElem marginFit(PGuiElem top, PGuiElem rest, double height, MarginType);
  PGuiElem margins(PGuiElem content, int left, int top, int right, int bottom);
  PGuiElem label(const string&, Color = colors[ColorId::WHITE], char hotkey = 0);
  PGuiElem label(const string&, int size, Color = colors[ColorId::WHITE]);
  PGuiElem centeredLabel(const string&, int size, Color = colors[ColorId::WHITE]);
  PGuiElem centeredLabel(const string&, Color = colors[ColorId::WHITE]);
  PGuiElem variableLabel(function<string()>,
      bool center = false, int size = Renderer::textSize, Color = colors[ColorId::WHITE]);
  PGuiElem mainMenuLabel(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_ON]);
  PGuiElem mainMenuLabelBg(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_OFF]);
  PGuiElem labelUnicode(const String&, Color, int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  PGuiElem viewObject(const ViewObject& object, bool useSprites);
  PGuiElem viewObject(ViewId, bool useSprites);
  PGuiElem drawCustom(function<void(Renderer&, Rectangle)>);
  PGuiElem translate(PGuiElem, Vec2, Rectangle newSize);
  PGuiElem centerHoriz(PGuiElem, int width);
  PGuiElem mouseOverAction(function<void()> callback, function<void()> onLeaveCallback = nullptr);
  PGuiElem mouseHighlight(PGuiElem highlight, int myIndex, int* highlighted);
  PGuiElem mouseHighlight2(PGuiElem highlight);
  PGuiElem mouseHighlightGameChoice(PGuiElem, View::GameTypeChoice my,
      optional<View::GameTypeChoice>& highlight);
  static int getHeldInitValue();
  PGuiElem scrollable(PGuiElem content, double* scrollPos, int* held = nullptr);
  PGuiElem getScrollButton();
  PGuiElem conditional(PGuiElem elem, function<bool(GuiElem*)> cond);
  PGuiElem conditional(PGuiElem elem, PGuiElem alter, function<bool(GuiElem*)> cond);
  enum class Alignment { TOP, LEFT, BOTTOM, RIGHT, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER,
      TOP_CENTER, LEFT_CENTER, BOTTOM_CENTER, RIGHT_CENTER, VERTICAL_CENTER, LEFT_STRETCHED, RIGHT_STRETCHED,
      CENTER_STRETCHED};
  PGuiElem sprite(Texture&, Alignment, bool vFlip = false, bool hFlip = false,
      Vec2 offset = Vec2(0, 0), double alpha = 1);
  PGuiElem sprite(Texture&, double scale);
  PGuiElem tooltip(const vector<string>&);

  enum class TexId {
    SCROLLBAR,
    SCROLL_BUTTON,
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
    VERT_BAR_MINI,
    CORNER_MINI,
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_RIGHT,
    SCROLL_UP,
    SCROLL_DOWN,
    WINDOW_CORNER,
    WINDOW_VERT_BAR,
    MAIN_MENU_HIGHLIGHT,
    KEEPER_CHOICE,
    ADVENTURER_CHOICE,
    KEEPER_HIGHLIGHT,
    ADVENTURER_HIGHLIGHT,
    MENU_ITEM,
    MENU_CORE,
    MENU_MOUTH,
    SPLASH1,
    SPLASH2,
    LOADING_SPLASH,
  };

  PGuiElem sprite(TexId, Alignment);
  PGuiElem repeatedPattern(Texture& tex);
  PGuiElem background(Color);
  PGuiElem highlight(double height);
  PGuiElem mainMenuHighlight();
  PGuiElem insideBackground(PGuiElem content);
  PGuiElem window(PGuiElem content);
  PGuiElem miniWindow(PGuiElem content);
  PGuiElem mapWindow(PGuiElem content);
  PGuiElem border(PGuiElem content);
  PGuiElem miniBorder(PGuiElem content);
  PGuiElem border2(PGuiElem content);
  PGuiElem mainDecoration(int rightBarWidth, int bottomBarHeight);
  PGuiElem invisible(PGuiElem content);
  PGuiElem background(PGuiElem content, Color);
  PGuiElem translucentBackground(PGuiElem content);
  Color translucentBgColor;
  void changeBackground(int r, int g, int b);
  void setBackground(int r, int g, int b);
  Color background1;
  Color background2;
  Color foreground1;
  Color text;
  Color titleText;
  Color inactiveText;

  enum IconId {
    BUILDING = 5,
    MINION = 4,
    LIBRARY = 1,
    WORKSHOP = 0,
    DIPLOMACY = 2,
    HELP = 3,
    DEITIES = 6,
    TEAM = 7,
    STAT_ATT = 8,
    STAT_DEF = 9,
    STAT_STR = 12,
    STAT_DEX = 13,
    STAT_ACC = 10,
    STAT_SPD = 11,
  };

  PGuiElem icon(IconId);
  Texture& get(TexId);

  private:

  PGuiElem getScrollbar();
  Vec2 getScrollButtonSize();
  Texture& getIconTex(IconId);

  map<TexId, Texture> textures;
  vector<Texture> iconTextures;
  Clock* clock;
};


#endif
