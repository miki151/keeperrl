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

#pragma once

#include "renderer.h"
#include "drag_and_drop.h"

class ViewObject;
class Clock;
class Options;
enum class SpellId;
class ScrollPosition;

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  virtual bool onLeftClick(Vec2) { return false; }
  virtual bool onRightClick(Vec2) { return false; }
  virtual bool onMouseMove(Vec2) { return false;}
  virtual void onMouseGone() {}
  virtual void onMouseRelease(Vec2) {}
  virtual void onRefreshBounds() {}
  virtual bool onKeyPressed2(SDL::SDL_Keysym) { return false;}
  virtual bool onMouseWheel(Vec2 mousePos, bool up) { return false;}
  virtual optional<int> getPreferredWidth() { return none; }
  virtual optional<int> getPreferredHeight() { return none; }

  void setPreferredBounds(Vec2 origin);
  void setBounds(Rectangle);
  Rectangle getBounds();

  virtual ~GuiElem();

  private:
  Rectangle bounds;
};

class GuiFactory {
  public:
  GuiFactory(Renderer&, Clock*, Options*);
  void loadFreeImages(const string& path);
  void loadNonFreeImages(const string& path);

  vector<string> breakText(const string& text, int maxWidth);

  DragContainer& getDragContainer();
  void propagateEvent(const Event&, vector<SGuiElem>);

  static bool isShift(const SDL::SDL_Keysym&);
  static bool isAlt(const SDL::SDL_Keysym&);
  static bool isCtrl(const SDL::SDL_Keysym&);
  static bool keyEventEqual(const SDL::SDL_Keysym&, const SDL::SDL_Keysym&);

  SDL::SDL_Keysym getKey(SDL::SDL_Keycode);
  SGuiElem button(function<void()> fun, SDL::SDL_Keysym, bool capture = false);
  SGuiElem buttonChar(function<void()> fun, char, bool capture = false, bool useAltIfWasdScrolling = false);
  SGuiElem button(function<void()> fun);
  SGuiElem buttonRightClick(function<void()> fun);
  SGuiElem reverseButton(function<void()> fun, vector<SDL::SDL_Keysym> = {}, bool capture = false);
  SGuiElem buttonRect(function<void(Rectangle buttonBounds)> fun, SDL::SDL_Keysym, bool capture = false);
  SGuiElem buttonRect(function<void(Rectangle buttonBounds)> fun);
  SGuiElem releaseLeftButton(function<void()> fun);
  SGuiElem releaseRightButton(function<void()> fun);
  SGuiElem focusable(SGuiElem content, vector<SDL::SDL_Keysym> focusEvent,
      vector<SDL::SDL_Keysym> defocusEvent, bool& focused);
  SGuiElem mouseWheel(function<void(bool)>);
  SGuiElem keyHandler(function<void(SDL::SDL_Keysym)>, bool capture = false);
  SGuiElem keyHandler(function<void()>, vector<SDL::SDL_Keysym>, bool capture = false);
  SGuiElem keyHandlerChar(function<void()>, char, bool capture = false, bool useAltIfWasdScrolling = false);
  SGuiElem stack(vector<SGuiElem>);
  SGuiElem stack(SGuiElem, SGuiElem);
  SGuiElem stack(SGuiElem, SGuiElem, SGuiElem);
  SGuiElem stack(SGuiElem, SGuiElem, SGuiElem, SGuiElem);
  SGuiElem external(GuiElem*);
  SGuiElem rectangle(Color color, optional<Color> borderColor = none);
  class ListBuilder {
    public:
    ListBuilder(GuiFactory&, int defaultSize = 0);
    ListBuilder& addElem(SGuiElem, int size = 0);
    ListBuilder& addSpace(int size = 0);
    ListBuilder& addElemAuto(SGuiElem);
    ListBuilder& addBackElemAuto(SGuiElem);
    ListBuilder& addBackElem(SGuiElem, int size = 0);
    ListBuilder& addMiddleElem(SGuiElem);
    SGuiElem buildVerticalList();
    SGuiElem buildVerticalListFit();
    SGuiElem buildHorizontalList();
    SGuiElem buildHorizontalListFit();
    int getSize() const;
    int getLength() const;
    bool isEmpty() const;
    vector<SGuiElem>& getAllElems();
    void clear();

    private:
    GuiFactory& gui;
    vector<SGuiElem> elems;
    vector<int> sizes;
    int defaultSize = 0;
    int backElems = 0;
    bool middleElem = false;
  };
  ListBuilder getListBuilder(int defaultSize = 0);
  SGuiElem verticalList(vector<SGuiElem>, int elemHeight);
  SGuiElem verticalListFit(vector<SGuiElem>, double spacing);
  SGuiElem horizontalList(vector<SGuiElem>, int elemHeight);
  SGuiElem horizontalListFit(vector<SGuiElem>, double spacing = 0);
  SGuiElem verticalAspect(SGuiElem, double ratio);
  SGuiElem empty();
  SGuiElem preferredSize(int width, int height, SGuiElem elem);
  SGuiElem preferredSize(Vec2, SGuiElem elem);
  SGuiElem setHeight(int height, SGuiElem);
  SGuiElem setWidth(int width, SGuiElem);
  enum MarginType { TOP, LEFT, RIGHT, BOTTOM};
  SGuiElem margin(SGuiElem top, SGuiElem rest, int height, MarginType);
  SGuiElem marginAuto(SGuiElem top, SGuiElem rest, MarginType);
  SGuiElem margin(SGuiElem top, SGuiElem rest, function<int(Rectangle)> width, MarginType type);
  SGuiElem maybeMargin(SGuiElem top, SGuiElem rest, int width, MarginType, function<bool(Rectangle)>);
  SGuiElem marginFit(SGuiElem top, SGuiElem rest, double height, MarginType);
  SGuiElem margins(SGuiElem content, int left, int top, int right, int bottom);
  SGuiElem margins(SGuiElem content, int all);
  SGuiElem leftMargin(int size, SGuiElem content);
  SGuiElem rightMargin(int size, SGuiElem content);
  SGuiElem topMargin(int size, SGuiElem content);
  SGuiElem bottomMargin(int size, SGuiElem content);
  SGuiElem progressBar(Color, double state);
  SGuiElem label(const string&, Color = colors[ColorId::WHITE], char hotkey = 0);
  SGuiElem labelHighlight(const string&, Color = colors[ColorId::WHITE], char hotkey = 0);
  SGuiElem labelHighlightBlink(const string& s, Color, Color);
  SGuiElem label(const string&, int size, Color = colors[ColorId::WHITE]);
  SGuiElem label(const string&, function<Color()>, char hotkey = 0);
  SGuiElem labelFun(function<string()>, function<Color()>);
  SGuiElem labelFun(function<string()>, Color = colors[ColorId::WHITE]);
  SGuiElem labelMultiLine(const string&, int lineHeight, int size = Renderer::textSize,
      Color = colors[ColorId::WHITE]);
  SGuiElem centeredLabel(Renderer::CenterType, const string&, int size, Color = colors[ColorId::WHITE]);
  SGuiElem centeredLabel(Renderer::CenterType, const string&, Color = colors[ColorId::WHITE]);
  SGuiElem variableLabel(function<string()>, int lineHeight, int size = Renderer::textSize,
      Color = colors[ColorId::WHITE]);
  SGuiElem mainMenuLabel(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_ON]);
  SGuiElem mainMenuLabelBg(const string&, double vPadding, Color = colors[ColorId::MAIN_MENU_OFF]);
  SGuiElem labelUnicode(const string&, Color = colors[ColorId::WHITE], int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  SGuiElem labelUnicode(const string&, function<Color()>, int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  SGuiElem viewObject(const ViewObject&, double scale = 1, Color = colors[ColorId::WHITE]);
  SGuiElem viewObject(ViewId, double scale = 1, Color = colors[ColorId::WHITE]);
  SGuiElem asciiBackground(ViewId);
  SGuiElem translate(SGuiElem, Vec2, Rectangle newSize);
  SGuiElem translate(function<Vec2()>, SGuiElem);
  SGuiElem centerHoriz(SGuiElem, optional<int> width = none);
  SGuiElem centerVert(SGuiElem, optional<int> height = none);
  SGuiElem onRenderedAction(function<void()>);
  SGuiElem mouseOverAction(function<void()> callback, function<void()> onLeaveCallback = nullptr);
  SGuiElem onMouseLeftButtonHeld(SGuiElem);
  SGuiElem onMouseRightButtonHeld(SGuiElem);
  SGuiElem mouseHighlight(SGuiElem highlight, int myIndex, int* highlighted);
  SGuiElem mouseHighlightClick(SGuiElem highlight, int myIndex, int* highlighted);
  SGuiElem mouseHighlight2(SGuiElem highlight);
  SGuiElem mouseHighlightGameChoice(SGuiElem, optional<GameTypeChoice> my, optional<GameTypeChoice>& highlight);
  static int getHeldInitValue();
  SGuiElem scrollable(SGuiElem content, ScrollPosition* scrollPos = nullptr, int* held = nullptr);
  SGuiElem getScrollButton();
  SGuiElem conditional2(SGuiElem elem, function<bool(GuiElem*)> cond);
  SGuiElem conditional(SGuiElem elem, function<bool()> cond);
  SGuiElem conditionalStopKeys(SGuiElem elem, function<bool()> cond);
  SGuiElem conditional2(SGuiElem elem, SGuiElem alter, function<bool(GuiElem*)> cond);
  SGuiElem conditional(SGuiElem elem, SGuiElem alter, function<bool()> cond);
  enum class Alignment { TOP, LEFT, BOTTOM, RIGHT, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER,
      TOP_CENTER, LEFT_CENTER, BOTTOM_CENTER, RIGHT_CENTER, VERTICAL_CENTER, LEFT_STRETCHED, RIGHT_STRETCHED,
      CENTER_STRETCHED};
  SGuiElem sprite(Texture&, Alignment, bool vFlip = false, bool hFlip = false,
      Vec2 offset = Vec2(0, 0), function<Color()> = nullptr);
  SGuiElem sprite(Texture&, Alignment, Color);
  SGuiElem sprite(Texture&, double scale);
  SGuiElem tooltip(const vector<string>&, milliseconds delay = milliseconds{700});
  typedef function<Vec2(const Rectangle&)> PositionFun;
  SGuiElem tooltip2(SGuiElem, PositionFun);
  SGuiElem darken();
  SGuiElem stopMouseMovement();
  SGuiElem fullScreen(SGuiElem);
  SGuiElem alignment(GuiFactory::Alignment, SGuiElem, optional<Vec2> size = none);
  SGuiElem dragSource(DragContent, function<SGuiElem()>);
  SGuiElem dragListener(function<void(DragContent)>);
  SGuiElem renderInBounds(SGuiElem);

  enum class TexId {
    SCROLLBAR,
    SCROLL_BUTTON,
    BACKGROUND_PATTERN,
    HORI_CORNER1,
    HORI_CORNER2,
    HORI_LINE,
    HORI_MIDDLE,
    VERT_BAR,
    HORI_BAR,
    HORI_BAR_MINI,
    VERT_BAR_MINI,
    CORNER_MINI,
    HORI_BAR_MINI2,
    VERT_BAR_MINI2,
    CORNER_MINI2,
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_RIGHT,
    IMMIGRANT_BG,
    IMMIGRANT2_BG,
    SCROLL_UP,
    SCROLL_DOWN,
    WINDOW_CORNER,
    WINDOW_CORNER_EXIT,
    WINDOW_VERT_BAR,
    UI_HIGHLIGHT,
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

  SGuiElem sprite(TexId, Alignment, function<Color()> = nullptr);
  SGuiElem repeatedPattern(Texture& tex);
  SGuiElem background(Color);
  SGuiElem highlight(double height);
  SGuiElem highlightDouble();
  SGuiElem mainMenuHighlight();
  SGuiElem window(SGuiElem content, function<void()> onExitButton);
  SGuiElem miniWindow();
  SGuiElem miniWindow(SGuiElem content, function<void()> onExitButton = nullptr, bool captureExitClick = false);
  SGuiElem miniWindow2(SGuiElem content, function<void()> onExitButton = nullptr, bool captureExitClick = false);
  SGuiElem miniBorder();
  SGuiElem miniBorder2();
  SGuiElem mainDecoration(int rightBarWidth, int bottomBarHeight, optional<int> topBarHeight);
  SGuiElem invisible(SGuiElem content);
  SGuiElem background(SGuiElem content, Color);
  SGuiElem translucentBackground(SGuiElem content);
  SGuiElem translucentBackground();
  Color translucentBgColor = Color(0, 0, 0, 150);
  Color foreground1 = Color(0x20, 0x5c, 0x4a, 150);
  Color text = colors[ColorId::WHITE];
  Color titleText = colors[ColorId::YELLOW];
  Color inactiveText = colors[ColorId::LIGHT_GRAY];
  Color background1 = colors[ColorId::BLACK];

  enum IconId {
    WORLD_MAP,
    LIBRARY,
    DIPLOMACY,
    HELP,
    MINION,
    BUILDING,
    DEITIES,
    HIGHLIGHT,
    STAT_ATT,
    STAT_DEF,
    STAT_ACC,
    STAT_SPD,
    STAT_STR,
    STAT_DEX,
    MORALE_1,
    MORALE_2,
    MORALE_3,
    MORALE_4,
    TEAM_BUTTON,
    TEAM_BUTTON_HIGHLIGHT,
  };

  SGuiElem icon(IconId, Alignment = Alignment::CENTER, Color = colors[ColorId::WHITE]);
  Texture& get(TexId);
  SGuiElem spellIcon(SpellId);
  SGuiElem uiHighlightMouseOver(Color = colors[ColorId::GREEN]);
  SGuiElem uiHighlightConditional(function<bool()>, Color = colors[ColorId::GREEN]);
  SGuiElem uiHighlight(Color = colors[ColorId::GREEN]);
  SGuiElem uiHighlight(function<Color()>);
  SGuiElem rectangleBorder(Color);

  private:

  SGuiElem getScrollbar();
  Vec2 getScrollButtonSize();
  Texture& getIconTex(IconId);
  SDL::SDL_Keysym getHotkeyEvent(char) ;

  map<TexId, Texture> textures;
  vector<Texture> iconTextures;
  vector<Texture> spellTextures;
  Clock* clock;
  Renderer& renderer;
  Options* options;
  DragContainer dragContainer;
};
