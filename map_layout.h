#ifndef _MAP_LAYOUT
#define _MAP_LAYOUT

#include <SFML/Graphics.hpp>

#include "util.h"
#include "enums.h"
#include "action.h"

class MapLayout {
  public:
  MapLayout(int screenWidth, int screenHeight, int leftMargin, int topMargin, int rightMargin, int bottomMargin,
      vector<ViewLayer> layers);
  Rectangle getBounds();
  void updateScreenSize(int width, int height);

  vector<ViewLayer> getLayers() const;

  virtual double squareWidth() = 0;
  virtual double squareHeight() = 0;
  virtual void increaseSize() {}
  virtual void decreaseSize() {}
  virtual Vec2 projectOnScreen(Vec2 mapPos) = 0;
  virtual Vec2 projectOnMap(Vec2 screenPos) = 0;
  virtual Rectangle getAllTiles(Rectangle bounds) = 0;
  virtual void updatePlayerPos(Vec2) = 0;
  virtual Optional<Action> overrideAction(const sf::Event::KeyEvent&) { return Nothing(); };

  static MapLayout* gridLayout(
      int screenW, int screenH,
      int squareWidth, int squareHeight,
      int leftMargin, int topMargin,
      int rightMargin, int bottomMargin,
      int boxMargin, vector<ViewLayer> layers);

  static MapLayout* worldLayout(int screenW, int screenH,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin);

  protected:
  int screenWidth;
  int screenHeight;
  int leftMargin;
  int topMargin;
  int rightMargin;
  int bottomMargin;

  vector<ViewLayer> layers;
};


#endif
