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

#include "util.h"
#include "gui_elem.h"
#include "hashing.h"

class Level;
class CreatureView;
class Renderer;

class MinimapGui : public GuiElem {
  public:

  MinimapGui(Renderer&, function<void()> clickFun);

  void update(WConstLevel, Rectangle bounds, const CreatureView*);
  void presentMap(const CreatureView*, Rectangle bounds, Renderer&, function<void(double, double)> clickFun);
  void clear();

  virtual void render(Renderer&) override;
  virtual bool onLeftClick(Vec2) override;

  private:

  void renderMap(Renderer&, Rectangle target);
  void putMapPixel(Vec2 pos, Color col);

  struct MinimapInfo {
    Rectangle bounds;
    unordered_set<Vec2, CustomHash<Vec2>> roads;
    vector<Vec2> enemies;
    Vec2 player;
    vector<Vec2> locations;
  } info;

  function<void()> clickFun;

  SDL::SDL_Surface* mapBuffer;
  optional<Texture> mapBufferTex;
  WConstLevel currentLevel = nullptr;
  Renderer& renderer;
};

