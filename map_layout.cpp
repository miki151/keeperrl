#include "stdafx.h"

#include "map_layout.h"

MapLayout::MapLayout(
    int screenW, int screenH,
    int leftM, int topM, int rightM, int bottomM,
    vector<ViewLayer> l) :screenWidth(screenW), screenHeight(screenH),
          leftMargin(leftM), topMargin(topM), rightMargin(rightM), bottomMargin(bottomM), layers(l) {}

Rectangle MapLayout::getBounds() {
  return Rectangle(leftMargin, topMargin, screenWidth - rightMargin, screenHeight - bottomMargin);
}

void MapLayout::updateScreenSize(int width, int height) {
  screenWidth = width;
  screenHeight = height;
}

vector<ViewLayer> MapLayout::getLayers() const {
  return layers;
}

class GridLayout : public MapLayout {
  public:
  GridLayout(
      int screenW, int screenH,
      int sW, int sH,
      int leftM, int topM,
      int rightM, int bottomM,
      vector<ViewLayer> layers)
      : MapLayout(screenW, screenH, leftM - 2 * sW, topM - 2 * sH, rightM - 2 * sW, bottomM - 2 * sH, layers),
        squareW(sW), squareH(sH) {}

  virtual double squareHeight() override {
    return squareH;
  }

  virtual double squareWidth() override {
    return squareW;
  }

  virtual void increaseSize() override {
    ++squareW;
    ++squareH;
  }

  virtual void decreaseSize() {
    --squareW;
    --squareH;
  }

  virtual Vec2 projectOnScreen(Vec2 mapPos) override {
    return getBounds().middle() + (mapPos).mult(Vec2(squareW, squareH)) - center;
  }

  virtual Vec2 projectOnMap(Vec2 screenPos) override {
    Vec2 pos = (screenPos + center - getBounds().middle()).div(Vec2(squareW, squareH));
    return pos;
  }

  virtual void updatePlayerPos(Vec2 pos) override {
    center = pos;
  }

  virtual Rectangle getAllTiles(Rectangle bounds) override {
    vector<Vec2> ret;
    Rectangle grid(getBounds().getW() / squareW, getBounds().getH() / squareH);
    Vec2 offset = center.div(Vec2(squareW, squareH)) - grid.middle();
    return Rectangle(max(bounds.getPX(), grid.getPX() + offset.x), max(bounds.getPY(), grid.getPY() + offset.y),
        min(bounds.getKX(), grid.getKX() + offset.x), min(bounds.getKY(), grid.getKY() + offset.y));
  }

  private:
  Vec2 center;
  int squareW;
  int squareH;
};

MapLayout* MapLayout::gridLayout(int screenW, int screenH,
    int squareWidth, int squareHeight,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin,
    vector<ViewLayer> layers) {
  return new GridLayout(
      screenW, screenH, squareWidth, squareHeight, leftMargin, topMargin, rightMargin, bottomMargin, layers);
}

class WorldLayout : public GridLayout {
  public:
  WorldLayout(int screenW, int screenH, int leftM, int topM, int rightM, int bottomM) 
      : GridLayout(screenW, screenH, 1, 1, leftM, topM, rightM, bottomM,
          {ViewLayer::FLOOR, ViewLayer::CREATURE}) {}

  virtual Vec2 projectOnScreen(Vec2 mapPos) override {
    return mapPos;
  }
};

MapLayout* MapLayout::worldLayout(int screenW, int screenH,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin) {
  return new WorldLayout(
      screenW, screenH, leftMargin, topMargin, rightMargin, bottomMargin);
}

