#ifndef _GUI_ELEM
#define _GUI_ELEM

#include "renderer.h"

class ViewObject;

class GuiElem {
  public:
  virtual void render(Renderer&) {}
  virtual void onLeftClick(Vec2) {}
  virtual void onRightClick(Vec2) {}
  virtual void onMouseMove(Vec2) {}
  virtual void onMouseRelease() {}
  virtual void onRefreshBounds() {}
  virtual void onKeyPressed(char) {}

  void setBounds(Rectangle);
  Rectangle getBounds();

  virtual ~GuiElem();

  Rectangle bounds;

  static void initialize(const string& texturePath);

  static PGuiElem button(function<void()> fun, char hotkey = 0);
  static PGuiElem button(function<void()> fun, Event::KeyEvent);
  static PGuiElem stack(vector<PGuiElem>);
  static PGuiElem stack(PGuiElem, PGuiElem);
  static PGuiElem rectangle(sf::Color color, Optional<sf::Color> borderColor = Nothing());
  static PGuiElem verticalList(vector<PGuiElem>, int elemHeight, int spacing);
  static PGuiElem verticalList(vector<PGuiElem>, vector<int> elemHeight, int spacing);
  static PGuiElem horizontalList(vector<PGuiElem>, int elemWidth, int spacing, int numAlignRight = 0);
  static PGuiElem horizontalList(vector<PGuiElem>, vector<int> elemWidth, int spacing, int numAlignRight = 0);
  static PGuiElem tabs(vector<PGuiElem> buttons, vector<PGuiElem> content, int tabWidth, int tabHeight,
      int tabSpacing);
  static PGuiElem empty();
  enum MarginType { TOP, LEFT, RIGHT, BOTTOM};
  static PGuiElem margin(PGuiElem top, PGuiElem rest, int height, MarginType);
  static PGuiElem margins(PGuiElem content, int left, int top, int right, int bottom);
  static PGuiElem label(const string&, Color, char hotkey = 0);
  static PGuiElem label(sf::Uint32, Color, int size, Renderer::FontId);
  static PGuiElem viewObject(const ViewObject& object);
  static PGuiElem drawCustom(function<void(Renderer&, Rectangle)>);
  static PGuiElem translate(PGuiElem, Vec2, Rectangle newSize);
  static PGuiElem centerHoriz(PGuiElem, int width);
  static PGuiElem mouseOverAction(function<void()>);
  static PGuiElem mouseHighlight(PGuiElem highlight, int myIndex, int* highlighted);
  static PGuiElem mouseHighlight2(PGuiElem highlight);
  static PGuiElem scrollable(PGuiElem content, int contentHeight, double* scrollPos, double scrollJump);
  static PGuiElem getScrollButton();
  static PGuiElem conditional(PGuiElem elem, function<bool(GuiElem*)> cond);
  enum class Alignment { TOP, LEFT, BOTTOM, RIGHT, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT};
  static PGuiElem sprite(Texture& tex, Alignment align, bool vFlip = false, bool hFlip = false, int cornerOffset = 0,
      double alpha = 1);
  static PGuiElem repeatedPattern(Texture& tex);
  static PGuiElem background(Color);
  static PGuiElem highlight(Color);
  static PGuiElem insideBackground(PGuiElem content);
  static PGuiElem window(PGuiElem content);
  static PGuiElem mapWindow(PGuiElem content);
  static PGuiElem border(PGuiElem content);
  static PGuiElem border2(PGuiElem content);
  static PGuiElem invisible(PGuiElem content);

  static Color background1;
  static Color background2;
  static Color foreground1;
  static Color text;
  static Color titleText;
  static Color inactiveText;
};

#endif
