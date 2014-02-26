#include "stdafx.h"

#include "map_layout.h"

vector<ViewLayer> MapLayout::getLayers() const {
  return layers;
}

MapLayout::MapLayout(int sW, int sH, vector<ViewLayer> l) : layers(l), squareW(sW), squareH(sH) {}

int MapLayout::squareHeight() {
  return squareH;
}

int MapLayout::squareWidth() {
  return squareW;
}

Vec2 MapLayout::projectOnScreen(Rectangle bounds, Vec2 mapPos) {
  return bounds.middle() + mapPos.mult(Vec2(squareW, squareH)) - center;
}

Vec2 MapLayout::projectOnMap(Rectangle bounds, Vec2 screenPos) {
  return (screenPos + center - bounds.middle()).div(Vec2(squareW, squareH));
}

void MapLayout::updatePlayerPos(Vec2 pos) {
  center = pos;
}

Rectangle MapLayout::getAllTiles(Rectangle screenBounds1, Rectangle tableBounds) {
  vector<Vec2> ret;
  Rectangle screenBounds = screenBounds1.minusMargin(-squareH);
  Rectangle grid(screenBounds.getW() / squareW, screenBounds.getH() / squareH);
  Vec2 offset = center.div(Vec2(squareW, squareH)) - grid.middle();
  return tableBounds.intersection(grid.translate(offset));
}

