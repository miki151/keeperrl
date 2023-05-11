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
#include "keybinding.h"
#include "steam_input.h"

class ViewObject;
class Clock;
class Options;
class ScrollPosition;
class KeybindingMap;
struct ScriptedUI;
struct ScriptedUIData;
struct ScriptedUIState;

void dumpGuiLineNumbers(ostream&);

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  virtual bool onClick(MouseButtonId, Vec2) { return false; }
  virtual bool onMouseMove(Vec2, Vec2) { return false;}
  virtual bool onScrollEvent(Vec2 mousePos, double x, double y, milliseconds timeDiff) { return false;}
  virtual void onMouseGone() {}
  virtual void onRefreshBounds() {}
  virtual void renderPart(Renderer& r, Rectangle) { render(r); }
  virtual bool onKeyPressed2(SDL::SDL_Keysym) { return false;}
  virtual bool onTextInput(const char*) { return false; }
  virtual optional<int> getPreferredWidth() { return none; }
  virtual optional<int> getPreferredHeight() { return none; }
  optional<Vec2> getPreferredSize();

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
  GuiFactory(Renderer&, Clock*, Options*, const DirectoryPath& freeImages);
  void loadImages();
  ~GuiFactory();

  vector<string> breakText(const string& text, int maxWidth, int fontSize);

  DragContainer& getDragContainer();
  void propagateEvent(const Event&, const vector<SGuiElem>&);
  void propagateScrollEvent(const vector<SGuiElem>&);

  static bool isShift(const SDL::SDL_Keysym&);
  static bool isAlt(const SDL::SDL_Keysym&);
  static bool isCtrl(const SDL::SDL_Keysym&);
  static bool keyEventEqual(const SDL::SDL_Keysym&, const SDL::SDL_Keysym&);

  SDL::SDL_Keysym getKey(SDL::SDL_Keycode);
  SGuiElem button(function<void()>, SDL::SDL_Keysym, bool capture = false);
  SGuiElem button(function<void()>, bool capture = false);
  SGuiElem textField(int maxLength, function<string()> text, function<void(string)> callback,
      function<bool()> controllerFocus);
  SGuiElem textFieldFocused(int maxLength, function<string()> text, function<void(string)> callback);
  SGuiElem buttonPos(function<void(Rectangle, Vec2)>);
  SGuiElem buttonRightClick(function<void()>);
  SGuiElem reverseButton(function<void()>, Keybinding, bool capture = false);
  SGuiElem buttonRect(function<void(Rectangle buttonBounds)>);
  SGuiElem releaseLeftButton(function<void()>, optional<Keybinding> = none);
  SGuiElem releaseRightButton(function<void()>);
  SGuiElem focusable(SGuiElem content, Keybinding focusKey, Keybinding defocusKey, bool& focused);
  SGuiElem mouseWheel(function<void(bool)>);
  SGuiElem keyHandler(function<void(SDL::SDL_Keysym)>, bool capture = false);
  SGuiElem keyHandler(function<void()>, Keybinding, bool capture = false);
  SGuiElem keyHandlerBool(function<bool()>, vector<SDL::SDL_Keysym>);
  SGuiElem keyHandlerBool(function<bool()>, Keybinding);
  SGuiElem keyHandler(function<void()>, vector<SDL::SDL_Keysym>, bool capture = false);
  SGuiElem keyHandlerRect(function<void(Rectangle)>, vector<SDL::SDL_Keysym>, bool capture = false);
  SGuiElem keyHandlerRect(function<void(Rectangle)>, Keybinding, bool capture = false);
  SGuiElem stack(vector<SGuiElem>);
  SGuiElem stack(SGuiElem, SGuiElem);
  SGuiElem stack(SGuiElem, SGuiElem, SGuiElem);
  SGuiElem stack(SGuiElem, SGuiElem, SGuiElem, SGuiElem);
  SGuiElem rectangle(Color color, optional<Color> borderColor = none);
  SGuiElem scripted(function<void()> endCallback, ScriptedUIId, const ScriptedUIData&, ScriptedUIState&);
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
  SGuiElem standardButtonHighlight();
  SGuiElem buttonLabel(const string&, SGuiElem button, bool matchTextWidth = true,
      bool centerHorizontally = false, bool unicode = false);
  SGuiElem buttonLabel(const string&, function<void()> button, bool matchTextWidth = true,
      bool centerHorizontally = false, bool unicode = false);

  SGuiElem buttonLabelFocusable(SGuiElem content, function<void()> button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false);
  SGuiElem buttonLabelFocusable(SGuiElem content, function<void(Rectangle)> button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false);
  SGuiElem buttonLabelFocusable(const string&, function<void()> button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false, bool unicode = false);
  SGuiElem buttonLabelFocusable(const string&, function<void(Rectangle)> button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false, bool unicode = false);
  SGuiElem buttonLabelBlink(const string&, function<void()> button, function<bool()> focused = []{return false;},
      bool matchTextWidth = true, bool centerHorizontally = false);
  SGuiElem buttonLabelWithMargin(const string&, function<bool()> focused);
  SGuiElem buttonLabelSelected(const string&, function<void()> button, bool matchTextWidth = true,
      bool centerHorizontally = false);
  SGuiElem buttonLabelSelectedFocusable(const string&, function<void()> button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false);
  SGuiElem buttonLabelInactive(const string&, bool matchTextWidth = true, bool centerText = false);
  SGuiElem labelHighlight(const string&, Color = Color::WHITE, char hotkey = 0);
  SGuiElem labelHighlightBlink(const string& s, Color, Color, char hotkey = 0);
  SGuiElem label(const string&, int size, Color = Color::WHITE);
  SGuiElem label(const string&, function<Color()>, char hotkey = 0);
  SGuiElem labelFun(function<string()>, function<Color()>);
  SGuiElem labelFun(function<string()>, Color = Color::WHITE);
  SGuiElem labelMultiLine(const string&, int lineHeight, int size = Renderer::textSize(),
      Color = Color::WHITE);
  SGuiElem labelMultiLineWidth(const string&, int lineHeight, int width, int size = Renderer::textSize(),
      Color = Color::WHITE, char delim = ' ');
  SGuiElem centeredLabel(Renderer::CenterType, const string&, int size, Color = Color::WHITE);
  SGuiElem centeredLabel(Renderer::CenterType, const string&, Color = Color::WHITE);
  SGuiElem mainMenuLabel(const string&, double vPadding, Color = Color::MAIN_MENU_ON);
  SGuiElem mainMenuLabelBg(const string&, double vPadding, Color = Color::MAIN_MENU_OFF);
  SGuiElem labelUnicode(const string&, Color = Color::WHITE, int size = Renderer::textSize(),
      FontId = FontId::SYMBOL_FONT);
  SGuiElem labelUnicodeHighlight(const string&, Color color = Color::WHITE, int size = Renderer::textSize(),
      FontId = FontId::SYMBOL_FONT);
  SGuiElem crossOutText(Color);
  SGuiElem viewObject(const ViewObject&, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(function<ViewObject()>, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(ViewId, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(ViewIdList, double scale = 1, Color = Color::WHITE);
  SGuiElem viewObject(function<ViewId()>, double scale = 1, Color = Color::WHITE);
  SGuiElem asciiBackground(ViewId);
  enum class TranslateCorner {
    TOP_LEFT_SHIFTED,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER
  };
  SGuiElem translate(SGuiElem, Vec2 pos, optional<Vec2> size = none, TranslateCorner = TranslateCorner::TOP_LEFT);
  SGuiElem translate(function<Vec2()>, SGuiElem);
  SGuiElem translateAbsolute(function<Vec2()>, SGuiElem);
  SGuiElem centerHoriz(SGuiElem, optional<int> width = none);
  SGuiElem centerVert(SGuiElem, optional<int> height = none);
  SGuiElem onRenderedAction(function<void()>);
  SGuiElem mouseOverAction(function<void()> callback, function<void()> onLeaveCallback = nullptr);
  SGuiElem onMouseLeftButtonHeld(SGuiElem);
  SGuiElem onMouseRightButtonHeld(SGuiElem);
  SGuiElem mouseHighlight(SGuiElem highlight, int myIndex, optional<int>* highlighted);
  SGuiElem mouseHighlight2(SGuiElem highlight, SGuiElem noHighlight = nullptr, bool capture = true);
  static int getHeldInitValue();
  SGuiElem scrollArea(SGuiElem, pair<double, double>& scrollPos);
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
  SGuiElem stopScrollEvent(SGuiElem content, function<bool()>);
  SGuiElem stopKeyEvents();
  SGuiElem fullScreen(SGuiElem);
  SGuiElem absolutePosition(SGuiElem content, Vec2 pos);
  SGuiElem alignment(GuiFactory::Alignment, SGuiElem, optional<Vec2> size = none);
  SGuiElem dragSource(DragContent, function<SGuiElem()>);
  SGuiElem dragListener(function<void(DragContent)>);
  SGuiElem renderInBounds(SGuiElem);
  using CustomDrawFun = function<void(Renderer&, Rectangle)>;
  SGuiElem drawCustom(CustomDrawFun);

  using TexId = TextureId;
  SGuiElem sprite(TexId, Alignment, optional<Color> = none);
  SGuiElem steamInputGlyph(ControllerKey, int size = 24);
  Texture& steamInputTexture(FilePath);
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
  Texture& get(TexId);
  static Color highlightColor();
  static Color highlightColor(int transparency);
  SGuiElem uiHighlightMouseOver(Color = highlightColor());
  SGuiElem uiHighlightLineConditional(function<bool()>, Color = highlightColor());
  SGuiElem uiHighlightLine(Color = highlightColor());
  SGuiElem uiHighlight(Color = highlightColor());
  SGuiElem uiHighlightConditional(function<bool()>, Color = highlightColor());
  SGuiElem uiHighlightFrame(function<bool()>);
  SGuiElem uiHighlightFrame();
  SGuiElem uiHighlightFrameFilled(function<bool()>);
  SGuiElem uiHighlightFrameFilled();
  SGuiElem blink(SGuiElem);
  SGuiElem blink(SGuiElem, SGuiElem);
  SGuiElem tutorialHighlight();
  SGuiElem rectangleBorder(Color);
  SGuiElem renderTopLayer(SGuiElem content);

  KeybindingMap* getKeybindingMap();
  MySteamInput* getSteamInput();
  Clock* clock;

  private:

  SGuiElem sprite(Texture&, Alignment, bool vFlip = false, bool hFlip = false,
      Vec2 offset = Vec2(0, 0), optional<Color> = none);
  SGuiElem sprite(Texture&, Alignment, Color);
  SGuiElem sprite(Texture&, double scale);
  SGuiElem getScrollbar();
  Vec2 getScrollButtonSize();
  SGuiElem buttonLabelFocusableImpl(SGuiElem content, SGuiElem button, function<bool()> focused,
      bool matchTextWidth = true, bool centerHorizontally = false);

  EnumMap<TexId, optional<Texture>> textures;
  unordered_map<string, Texture> steamInputTextures;
  vector<Texture> iconTextures;
  Renderer& renderer;
  Options* options;
  DragContainer dragContainer;
  DirectoryPath imagesPath;
  optional<milliseconds> lastJoyScrollUpdate;
};
