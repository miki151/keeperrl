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

#ifndef _MINIMAP_GUI_H
#define _MINIMAP_GUI_H

#include "util.h"
#include "gui_elem.h"

class Level;
class CreatureView;
class WindowRenderer;

class MinimapGui : public GuiElem {
  public:

  MinimapGui(function<void()> clickFun);

  void update(const Level* level, Rectangle levelPart, const CreatureView* creature, bool printLocations = false);
  void update(const Level* level, Rectangle levelPart, Rectangle bounds,
      const CreatureView* creature, bool printLocations = false);
  void presentMap(const CreatureView* creature, Rectangle bounds, WindowRenderer& renderer,
      function<void(double, double)> clickFun);

  virtual void render(Renderer&) override;
  virtual void onLeftClick(Vec2) override;

  static void initialize();

  private:

  void renderMap(Renderer&, Vec2 topLeft);

  struct MinimapInfo {
    Rectangle bounds;
    vector<Vec2> roads;
    vector<Vec2> enemies;
    Vec2 player;
    double scale;
    struct Location {
      Vec2 pos;
      string text;
    };
    vector<Location> locations;
  } info;

  function<void()> clickFun;
};

#endif
