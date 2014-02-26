#ifndef _TILE_H
#define _TILE_H

#include "view_object.h"
#include "renderer.h"

class Tile {
  public:
  static Tile getTile(const ViewObject& obj, bool sprite);
  static Color getColor(const ViewObject& object);

  Color color;
  String text;
  bool symFont = false;
  double translucent = 0;
  bool stickingOut = false;
  Tile(sf::Uint32 ch, Color col, bool sym = false) : color(col), text(ch), symFont(sym) {
  }
  Tile(int x, int y, int num = 0, bool _stickingOut = false) : stickingOut(_stickingOut),tileCoord(Vec2(x, y)), 
      texNum(num) {}

  Tile& addConnection(set<Dir> c, int x, int y) {
    connections.insert({c, Vec2(x, y)});
    return *this;
  }

  Tile& setTranslucent(double v) {
    translucent = v;
    return *this;
  }

  bool hasSpriteCoord() {
    return tileCoord;
  }

  Vec2 getSpriteCoord() {
    return *tileCoord;
  }

  Vec2 getSpriteCoord(set<Dir> c) {
    if (connections.count(c))
      return connections.at(c);
    else return *tileCoord;
  }

  int getTexNum() {
    CHECK(tileCoord) << "Not a sprite tile";
    return texNum;
  }

  private:
  Optional<Vec2> tileCoord;
  int texNum = 0;
  unordered_map<set<Dir>, Vec2> connections;
};


#endif
