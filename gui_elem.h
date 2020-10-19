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

#include "stdafx.h"
#include "util.h"
#include "renderer.h"
#include "drag_and_drop.h"
#include "texture_id.h"
#include "attr_type.h"
#include "view_id.h"

class ViewObject;
class Clock;
class Options;
class ScrollPosition;
class KeybindingMap;

void dumpGuiLineNumbers(ostream&);

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  enum ClickButton { LEFT, MIDDLE, RIGHT };
  virtual bool onClick(ClickButton, Vec2) { return false; }
  virtual void onClickElsewhere() {}
  virtual bool onMouseMove(Vec2) { return false;}
  virtual void onMouseGone() {}
  virtual void onMouseRelease(Vec2) {}
  virtual void onRefreshBounds() {}
  virtual void renderPart(Renderer& r, Rectangle) { render(r); }
  virtual bool onKeyPressed2(SDL::SDL_Keysym) { return false;}
  virtual bool onMouseWheel(Vec2 mousePos, bool up) { return false;}
  virtual bool onTextInput(const char*) { return false; }
  virtual optional<int> getPreferredWidth() { return none; }
  virtual optional<int> getPreferredHeight() { return none; }

  void setPreferredBounds(Vec2 origin);
  void setBounds(Rectangle);
  Rectangle getBounds();

  virtual ~GuiElem();
  GuiElem();

  void setLineNumber(int);

  private:
  Rectangle bounds;
  optional<int> lineNumber;
};

class GuiFactory {
  public:
  GuiFactory(Renderer&, Clock*, Options*, KeybindingMap*, const DirectoryPath& freeImages,
      const optional<DirectoryPath>& nonFreeImages);
  void loadImages();

  vector<string> breakText(const string& text, int maxWidth, int fontSize);

  DragContainer& getDragContainer();
  void propagateEvent(const Event&, vector<SGuiElem>);

  static bool isShift(const SDL::SDL_Keysym&);
  static bool isAlt(const SDL::SDL_Keysym&);
  static bool isCtrl(const SDL::SDL_Keysym&);
  static bool keyEventEqual(const SDL::SDL_Keysym&, const SDL::SDL_Keysym&);

  SDL::SDL_Keysym getKey(SDL::SDL_Keycode);
  SGuiElem button(function<void()>, SDL::SDL_Keysym, bool capture = false);
  SGuiElem buttonChar(function<void()>, char, bool capture = false, bool useAltIfWasdScrolling = false);
  SGuiElem button(function<void()>, bool capture = false);
  SGuiElem textField(int maxLength, function<string()> text, function<void(string)> callback);
  SGuiElem buttonPos(function<void(Rectangle, Vec2)>);
  SGuiElem buttonRightClick(function<void()>);
  SGuiElem reverseButton(function<void()>, vector<SDL::SDL_Keysym> = {}, bool capture = false);
  SGuiElem buttonRect(function<void(Rectangle buttonBounds)>, SDL::SDL_Keysym, bool capture = false);
  SGuiElem buttonRect(function<void(Rectangle buttonBounds)>);
  SGuiElem releaseLeftButton(function<void()>, optional<Keybinding> = none);
  SGuiElem releaseRightButton(function<void()>);
  SGuiElem focusable(SGuiElem content, vector<SDL::SDL_Keysym> focusEvent,
      vector<SDL::SDL_Keysym> defocusEvent, bool& focused);
  SGuiElem mouseWheel(function<void(bool)>);
  SGuiElem keyHandler(function<void(SDL::SDL_Keysym)>, bool capture = false);
  SGuiElem keyHandler(function<void()>, Keybinding, bool capture = false);
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
    ListBuilder& addElem(SGuiElem, int size = 0);
    ListBuilder& addSpace(int size = 0);
    ListBuilder& addElemAuto(SGuiElem);
    ListBuilder& addBackElemAuto(SGuiElem);
    ListBuilder& addBackElem(SGuiElem, int size = 0);
    ListBuilder& addBackSpace(int size = 0);
    ListBuilder& addMiddleElem(SGuiElem);
    SGuiElem buildVerticalList();
    SGuiElem buildVerticalListFit();
    SGuiElem buildHorizontalList();
    SGuiElem buildHorizontalListFit(double spacing = 0);
    int getSize() const;
    int getLength() const;
    bool isEmpty() const;
    void clear();
    optional<int> lineNumber;

    friend class GuiFactory;

    private:
    ListBuilder(GuiFactory&, int defaultSize = 0);
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
  SGuiElem label(const string&, Color = Color::WHITE, char hotkey = 0);
  SGuiElem standardButton();
  SGuiElem standardButton(SGuiElem content, SGuiElem button, bool matchTextWidth = true);
  SGuiElem standardButtonBlink(SGuiElem content, SGuiElem button, bool matchTextWidth);
  SGuiElem standardButtonHighlight();
  SGuiElem buttonLabel(const string&, SGuiElem button, bool matchTextWidth = true,
      bool centerHorizontally = false, bool unicode = false);
  SGuiElem buttonLabel(const string&, function<void()> button, bool matchTextWidth = true,
      bool centerHorizontally = false, bool unicode = false);
  SGuiElem buttonLabelBlink(const string&, function<void()> button);
  SGuiElem buttonLabelWithMargin(const string&, bool matchTextWidth = true);
  SGuiElem buttonLabelSelected(const string&, function<void()> button, bool matchTextWidth = true,
      bool centerHorizontally = false);
  SGuiElem buttonLabelInactive(const string&, bool matchTextWidth = true);
  SGuiElem labelHighlight(const string&, Color = Color::WHITE, char hotkey = 0);
  SGuiElem labelHighlightBlink(const string& s, Color, Color, char hotkey = 0);
  SGuiElem label(const string&, int size, Color = Color::WHITE);
  SGuiElem label(const string&, function<Color()>, char hotkey = 0);
  SGuiElem labelFun(function<string()>, function<Color()>);
  SGuiElem labelFun(function<string()>, Color = Color::WHITE);
  SGuiElem labelMultiLine(const string&, int lineHeight, int size = Renderer::textSize,
      Color = Color::WHITE);
  SGuiElem labelMultiLineWidth(const string&, int lineHeight, int width, int size = Renderer::textSize,
      Color = Color::WHITE, char delim = ' ');
  SGuiElem centeredLabel(Renderer::CenterType, const string&, int size, Color = Color::WHITE);
  SGuiElem centeredLabel(Renderer::CenterType, const string&, Color = Color::WHITE);
  SGuiElem variableLabel(function<string()>, int lineHeight, int size = Renderer::textSize,
      Color = Color::WHITE);
  SGuiElem mainMenuLabel(const string&, double vPadding, Color = Color::MAIN_MENU_ON);
  SGuiElem mainMenuLabelBg(const string&, double vPadding, Color = Color::MAIN_MENU_OFF);
  SGuiElem labelUnicode(const string&, Color = Color::WHITE, int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  SGuiElem labelUnicodeHighlight(const string&, Color color = Color::WHITE, int size = Renderer::textSize,
      Renderer::FontId = Renderer::SYMBOL_FONT);
  SGuiElem crossOutText(Color);
  SGuiElem viewObject(const ViewObject&, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(ViewId, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(ViewIdList, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(function<ViewId()>, double scale = 1, Color = Color::WHITE);
  SGuiElem asciiBackground(ViewId);
  enum class TranslateCorner {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
  };
  SGuiElem translate(SGuiElem, Vec2 pos, optional<Vec2> size = none, TranslateCorner = TranslateCorner::TOP_LEFT);
  SGuiElem translate(function<Vec2()>, SGuiElem);
  SGuiElem centerHoriz(SGuiElem, optional<int> width = none);
  SGuiElem centerVert(SGuiElem, optional<int> height = none);
  SGuiElem onRenderedAction(function<void()>);
  SGuiElem mouseOverAction(function<void()> callback, function<void()> onLeaveCallback = nullptr);
  SGuiElem onMouseLeftButtonHeld(SGuiElem);
  SGuiElem onMouseRightButtonHeld(SGuiElem);
  SGuiElem mouseHighlight(SGuiElem highlight, int myIndex, optional<int>* highlighted);
  SGuiElem mouseHighlight2(SGuiElem highlight, SGuiElem noHighlight = nullptr, bool capture = true);
  static int getHeldInitValue();
  SGuiElem scrollArea(SGuiElem);
  SGuiElem scrollable(SGuiElem content, ScrollPosition* scrollPos = nullptr, int* held = nullptr);
  SGuiElem getScrollButton();
  SGuiElem conditional2(SGuiElem elem, function<bool(GuiElem*)> cond);
  SGuiElem conditional(SGuiElem elem, function<bool()> cond);
  SGuiElem conditional(function<int()> cond, vector<SGuiElem> elem);
  SGuiElem conditionalStopKeys(SGuiElem elem, function<bool()> cond);
  SGuiElem conditional2(SGuiElem elem, SGuiElem alter, function<bool(GuiElem*)> cond);
  SGuiElem conditional(SGuiElem elem, SGuiElem alter, function<bool()> cond);
  enum class Alignment { TOP, LEFT, BOTTOM, RIGHT, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER,
      TOP_CENTER, LEFT_CENTER, BOTTOM_CENTER, RIGHT_CENTER, VERTICAL_CENTER, LEFT_STRETCHED, RIGHT_STRETCHED,
      CENTER_STRETCHED};
  SGuiElem tooltip(const vector<string>&, milliseconds delay = milliseconds{700});
  typedef function<Vec2(const Rectangle&)> PositionFun;
  SGuiElem tooltip2(SGuiElem, PositionFun);
  SGuiElem darken();
  SGuiElem stopMouseMovement();
  SGuiElem fullScreen(SGuiElem);
  SGuiElem absolutePosition(SGuiElem content, Vec2 pos);
  SGuiElem alignment(GuiFactory::Alignment, SGuiElem, optional<Vec2> size = none);
  SGuiElem dragSource(DragContent, function<SGuiElem()>);
  SGuiElem dragListener(function<void(DragContent)>);
  SGuiElem renderInBounds(SGuiElem);
  using CustomDrawFun = function<void(Renderer&, Rectangle)>;
  SGuiElem drawCustom(CustomDrawFun);
  SGuiElem slider(SGuiElem button, shared_ptr<int> position, int max);

  using TexId = TextureId;
  SGuiElem sprite(TexId, Alignment, optional<Color> = none);
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
  SGuiElem translucentBackgroundPassMouse(SGuiElem content);
  SGuiElem translucentBackgroundWithBorder(SGuiElem content);
  SGuiElem translucentBackgroundWithBorderPassMouse(SGuiElem content);
  SGuiElem translucentBackground();
  SGuiElem textInput(int width, int maxLines, shared_ptr<string> text);
  Color translucentBgColor = Color(0, 0, 0, 150);
  Color foreground1 = Color(0x20, 0x5c, 0x4a, 150);
  Color text = Color::WHITE;
  Color titleText = Color::YELLOW;
  Color inactiveText = Color::LIGHT_GRAY;
  Color background1 = Color::BLACK;

  enum IconId {
    WORLD_MAP,
    LIBRARY,
    DIPLOMACY,
    HELP,
    MINION,
    BUILDING,
    DEITIES,
    KEEPER,
    MORALE_1,
    MORALE_2,
    MORALE_3,
    MORALE_4,
    TEAM_BUTTON,
    TEAM_BUTTON_HIGHLIGHT,
    MINIMAP_WORLD1,
    MINIMAP_WORLD2,
    MINIMAP_WORLD3,
    MINIMAP_CENTER1,
    MINIMAP_CENTER2,
    MINIMAP_CENTER3,
    EXPAND_UP,
    SPECIAL_IMMIGRANT
  };

  SGuiElem minimapBar(SGuiElem icon1, SGuiElem icon2);
  SGuiElem icon(IconId, Alignment = Alignment::CENTER, Color = Color::WHITE);
  SGuiElem icon(AttrType);
  Texture& get(TexId);
  SGuiElem uiHighlightMouseOver(Color = Color::GREEN);
  SGuiElem uiHighlightConditional(function<bool()>, Color = Color::GREEN);
  SGuiElem uiHighlightLine(Color = Color::GREEN);
  SGuiElem uiHighlight(Color = Color::GREEN);
  SGuiElem blink(SGuiElem);
  SGuiElem blink(SGuiElem, SGuiElem);
  SGuiElem tutorialHighlight();
  SGuiElem rectangleBorder(Color);
  SGuiElem renderTopLayer(SGuiElem content);

  KeybindingMap* keybindingMap;

  private:

  SGuiElem sprite(Texture&, Alignment, bool vFlip = false, bool hFlip = false,
      Vec2 offset = Vec2(0, 0), optional<Color> = none);
  SGuiElem sprite(Texture&, Alignment, Color);
  SGuiElem sprite(Texture&, double scale);
  SGuiElem getScrollbar();
  Vec2 getScrollButtonSize();
  SDL::SDL_Keysym getHotkeyEvent(char) ;

  EnumMap<TexId, optional<Texture>> textures;
  vector<Texture> iconTextures;
  EnumMap<AttrType, optional<Texture>> attrTextures;
  Clock* clock;
  Renderer& renderer;
  Options* options;
  DragContainer dragContainer;
  DirectoryPath freeImagesPath;
  optional<DirectoryPath> nonFreeImagesPath;
  void loadNonFreeImages(const DirectoryPath&);
  void loadFreeImages(const DirectoryPath&);
};
