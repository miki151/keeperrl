#pragma once

#include "util.h"
#include "view_id.h"
#include "color.h"
#include "fx_info.h"

struct TileInfo {
  PrimaryId<ViewId> SERIAL(viewId);
  string SERIAL(symbol);
  Color SERIAL(color);
  bool SERIAL(isSymbolFont) = false;
  optional<string> SERIAL(sprite);
  optional<Color> SERIAL(spriteColor);
  bool SERIAL(roundShadow) = false;
  bool SERIAL(wallShadow) = false;
  bool SERIAL(wallConnections) = false;
  bool SERIAL(roadConnections) = false;
  bool SERIAL(moveUp) = false;
  bool SERIAL(animated) = false;
  bool SERIAL(canMirror) = false;
  bool SERIAL(highlightAbove) = false;
  optional<string> SERIAL(mountainSides);
  optional<string> SERIAL(waterSides);
  optional<string> SERIAL(background);
  optional<string> SERIAL(southSide);
  vector<string> SERIAL(extraBorders);
  optional<ViewId> SERIAL(connectionId);
  optional<FXInfo> SERIAL(fx);
  optional<Vec2> SERIAL(weaponOrigin);
  struct Connection {
    string SERIAL(spriteName);
    vector<Dir> SERIAL(dirs);
    SERIALIZE_ALL(spriteName, dirs)
  };
  vector<Connection> SERIAL(connections);
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar(NAMED(viewId), NAMED(symbol), NAMED(color), OPTION(isSymbolFont), NAMED(sprite), NAMED(spriteColor));
    ar(OPTION(roundShadow), OPTION(wallShadow), OPTION(wallConnections), NAMED(mountainSides), NAMED(waterSides));
    ar(NAMED(background), OPTION(roadConnections), OPTION(extraBorders), OPTION(moveUp), OPTION(animated));
    ar(OPTION(canMirror), NAMED(southSide), NAMED(connectionId), OPTION(highlightAbove), NAMED(fx), NAMED(weaponOrigin));
    if (version >= 1)
      ar(OPTION(connections));
  }
};

CEREAL_CLASS_VERSION(TileInfo, 1)