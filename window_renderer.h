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

#ifndef _WINDOW_RENDERER_H
#define _WINDOW_RENDERER_H

#include <SFML/Graphics.hpp>

#include "util.h"
#include "renderer.h"

using sf::Font;
using sf::Color;
using sf::String;
using sf::Image;
using sf::Sprite;
using sf::Texture;
using sf::RenderWindow;
using sf::Event;

class WindowRenderer : public Renderer {
  public: 
  void initialize(int width, int height, string title);
  void drawAndClearBuffer();
  void resize(int width, int height);
  bool pollEvent(Event&, Event::EventType);
  bool pollEvent(Event&);
  void flushEvents(Event::EventType);
  void flushAllEvents();
  void waitEvent(Event&);
  Vec2 getMousePos();
  bool leftButtonPressed();
  bool rightButtonPressed();

  void startMonkey();
  bool isMonkey();
  Event getRandomEvent();

  private:
  RenderWindow* display = nullptr;
  sf::View* sfView;
  bool monkey = false;
  deque<Event> eventQueue;
};

#endif
