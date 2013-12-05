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
      int marg, vector<ViewLayer> layers) : MapLayout(screenW, screenH, leftM, topM, rightM, bottomM, layers) 
              , squareW(sW), squareH(sH), boxMargin(marg) {}

  virtual double squareHeight(Vec2 mapPos) override {
    return squareH;
  }

  virtual double squareWidth(Vec2 mapPos) override {
    return squareW;
  }

  virtual Vec2 projectOnScreen(Vec2 mapPos, double height) override {
    return getBounds().middle() + (mapPos).mult(Vec2(squareW, squareH)) - center;
  }

  virtual Vec2 projectOnMap(Vec2 screenPos) override {
    Vec2 pos = (screenPos + center - getBounds().middle()).div(Vec2(squareW, squareH));
    return pos;
  }

  virtual void updatePlayerPos(Vec2 pos) override {
    center = pos;
  }

  virtual vector<Vec2> getAllTiles() override {
    vector<Vec2> ret;
    Rectangle grid(getBounds().getW() / squareW, getBounds().getH() / squareH);
    for (Vec2 v : grid)
      ret.push_back(v + center.div(Vec2(squareW, squareH)) - grid.middle());
    return ret;
  }

  private:
  Vec2 center;
  int squareW;
  int squareH;
  int boxMargin;
};

MapLayout* MapLayout::gridLayout(int screenW, int screenH,
    int squareWidth, int squareHeight,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin,
    int boxMargin, vector<ViewLayer> layers) {
  return new GridLayout(
      screenW, screenH, squareWidth, squareHeight, leftMargin, topMargin, rightMargin, bottomMargin, boxMargin,
      layers);
}

class WorldLayout : public GridLayout {
  public:
  WorldLayout(int screenW, int screenH, int leftM, int topM, int rightM, int bottomM) 
      : GridLayout(screenW, screenH, 1, 1, leftM, topM, rightM, bottomM, 1,
          {ViewLayer::FLOOR, ViewLayer::CREATURE}) {}

  virtual Vec2 projectOnScreen(Vec2 mapPos, double height) override {
    return mapPos;
  }
};

MapLayout* MapLayout::worldLayout(int screenW, int screenH,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin) {
  return new WorldLayout(
      screenW, screenH, leftMargin, topMargin, rightMargin, bottomMargin);
}

double multY = 75;
double multX = 36;
double offsetY = 1750;
double offsetX = 0;
double camOffset = 30;
double camHeight = 15;

using sf::Keyboard;

class TppLayout : public MapLayout {
  public:
  TppLayout(int screenW, int screenH,
    int leftM, int topM, int rightM, int bottomM, int sW, int sH) : MapLayout(screenW, screenH,
      leftM, topM, rightM, bottomM, allLayers), squareW(sW), squareH(sH) {}

  virtual double squareWidth(Vec2 mapPos) override {
    VecD dir = translateToScreen(mapPos - center);
    return max(5., min(20., 160. * (atan2(camOffset - dir.y, camHeight) - atan2(camOffset - dir.y - 10, camHeight))));
  }

  virtual double squareHeight(Vec2 mapPos) override {
    return squareWidth(mapPos);
  }

  virtual Vec2 projectOnMap(Vec2 screenPos) override {
    return screenPos;
  }

  virtual Vec2 projectOnScreen(Vec2 mapPos, double height) override {
    VecD dir = translateToScreen(mapPos - center);
    return getBounds().middle() + Vec2(offsetX + multX * atan2(dir.x, camOffset - dir.y) * squareW,
        offsetY - multY * atan2(camOffset - dir.y, camHeight - height * 0) * squareH);
  }

  virtual vector<Vec2> getAllTiles() override {
    int horizon = 50;
    vector<Vec2> ret;
    for (Vec2 v : Rectangle(center.x - horizon, center.y - horizon, center.x + horizon, center.y + horizon))
      if (projectOnScreen(v, 0).inRectangle(getBounds()))
        ret.push_back(v);
    return ret;
  }

  struct VecD {
    VecD(double a, double b) : x(a), y(b) {}
    VecD(Vec2 v) : x(v.x), y(v.y) {}
    double x, y;
  };

  VecD translateToScreen(Vec2 dir) {
    CHECK(contains(Vec2::directions8(), facing)) << facing;
    double diagDist = 1 / sqrt(2);
    if (facing == Vec2(0, -1))
      return dir;
    if (facing == Vec2(0, 1))
      return VecD(-dir.x, -dir.y);
    if (facing == Vec2(1, 0))
      return VecD(dir.y, -dir.x);
    if (facing == Vec2(-1, 0))
      return VecD(-dir.y, dir.x);
    if (facing == Vec2(-1, -1))
      return VecD((-dir.y + dir.x) * diagDist, (dir.y + dir.x) * diagDist);
    if (facing == Vec2(1, -1))
      return VecD((dir.y + dir.x) * diagDist, (dir.y - dir.x) * diagDist);
    if (facing == Vec2(1, 1))
      return VecD((dir.y - dir.x) * diagDist, (-dir.y - dir.x) * diagDist);
    return VecD((-dir.y - dir.x) * diagDist, (-dir.y + dir.x) * diagDist);
  }

  void printTrans() {
    Debug() << "xMult = " << multX << " yMult = " << multY << " xOffset = " << offsetX << " yOffset = " << offsetY << " camOffset = " << camOffset << " camHeight = " << camHeight;
  }

  vector<Vec2> dirs {
    Vec2(0, -1), Vec2(1, -1), Vec2(1, 0), Vec2(1, 1), Vec2(0, 1), Vec2(-1, 1), Vec2(-1, 0), Vec2(-1, -1)};

  Vec2 facingMove(Vec2 f, int dir) {
    auto elem = findElement(dirs, f);
    CHECK(elem);
    return dirs[(*elem + dir + dirs.size()) % dirs.size()];
  }


  virtual Optional<Action> overrideAction(const sf::Event::KeyEvent& key) override {
    switch (key.code) {
      case Keyboard::Up:
      case Keyboard::Numpad8: return Action(
          key.control ? ActionId::TRAVEL : key.alt ? ActionId::FIRE : ActionId::MOVE, facing);
      case Keyboard::Right: 
      case Keyboard::Numpad6: facing = facingMove(facing, 1); return Action(ActionId::IDLE);
      case Keyboard::Down:
      case Keyboard::Numpad2: facing = facingMove(facing, 4); return Action(ActionId::IDLE);
      case Keyboard::Left:
      case Keyboard::Numpad4: facing = facingMove(facing, -1); return Action(ActionId::IDLE);
      case Keyboard::Numpad9:
      case Keyboard::Numpad7:
      case Keyboard::Numpad3:
      case Keyboard::Numpad1: return Action(ActionId::IDLE);
      case Keyboard::T: return Action(ActionId::THROW_DIR, facing);
 /*     case Keyboard::F1: multX *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F2: multX *= 0.9; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F3: multY *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F4: multY *= 0.9; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F5: offsetX *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F6: offsetX *= 0.9; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F7: offsetY *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F8: offsetY *= 0.9; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F9: camOffset *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F10: camOffset *= 0.9; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F11: camHeight *= 1.1; printTrans(); return Action(ActionId::IDLE);
      case Keyboard::F12: camHeight *= 0.9; printTrans(); return Action(ActionId::IDLE);*/
      default: return Nothing();
    }
}

  virtual void updatePlayerPos(Vec2 pos) override {
    if (contains(Vec2::directions8(), pos - center)) {
      Vec2 movement = pos - center;
      if (movement == lastMovement) {
        ++cnt;
        if (cnt > 10) {
          facing = movement;
          cnt = 0;
        }
      } else {
        lastMovement = movement;
        cnt = 0;
      }
    }
    center = pos;
  }

  private:
  int squareW;
  int squareH;
  Vec2 center;
  Vec2 facing = Vec2(0, -1);
  int cnt = 0;
  Vec2 lastMovement;
};

MapLayout* MapLayout::tppLayout(int screenW, int screenH,
    int squareWidth, int squareHeight,
    int leftMargin, int topMargin,
    int rightMargin, int bottomMargin) {
  return new TppLayout(screenW, screenH, leftMargin, topMargin, rightMargin, bottomMargin, squareWidth, squareHeight);
}
