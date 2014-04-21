#ifndef _MAP_LAYOUT
#define _MAP_LAYOUT

#include <SFML/Graphics.hpp>

#include "util.h"
#include "enums.h"

class MapLayout {
  public:
  MapLayout() {}
  MapLayout(int squareW, int squareH, vector<ViewLayer> layers);

  vector<ViewLayer> getLayers() const;

  int squareWidth();
  int squareHeight();
  Vec2 projectOnScreen(Rectangle bounds, Vec2 mapPos);
  Vec2 projectOnMap(Rectangle bounds, Vec2 screenPos);
  Rectangle getAllTiles(Rectangle screenBounds, Rectangle tableBounds);
  void updatePlayerPos(Vec2);
  Vec2 getPlayerPos();

  private:
  vector<ViewLayer> layers;
  Vec2 center;
  int squareW;
  int squareH;
};


#endif
