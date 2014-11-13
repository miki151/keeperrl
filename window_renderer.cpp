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

#include "stdafx.h"
#include "window_renderer.h"

using namespace sf;

void WindowRenderer::drawAndClearBuffer() {
  display->display();
  display->clear(Color(0, 0, 0));
}

void WindowRenderer::resize(int width, int height) {
  display->setView(*(sfView = new sf::View(sf::FloatRect(0, 0, width, height))));
}

void WindowRenderer::initialize(int width, int height, string title) {
  display = new RenderWindow(VideoMode(1024, 600, 32), "KeeperRL");
  sfView = new sf::View(display->getDefaultView());
  Renderer::initialize(display, width, height);
}

Event WindowRenderer::getRandomEvent() {
  Event::EventType type = Event::EventType(Random.get(int(Event::Count)));
  Event ret;
  ret.type = type;
  int modProb = 5;
  switch (type) {
    case Event::KeyPressed:
      ret.key = {Keyboard::Key(Random.get(int(Keyboard::Key::KeyCount))), Random.roll(modProb),
          Random.roll(modProb), Random.roll(modProb), Random.roll(modProb) };
      break;
    case Event::MouseButtonReleased:
    case Event::MouseButtonPressed:
      ret.mouseButton = { chooseRandom({Mouse::Left, Mouse::Right}), Random.get(getWidth()),
        Random.get(getHeight()) };
      break;
    case Event::MouseMoved:
      ret.mouseMove = { Random.get(getWidth()), Random.get(getHeight()) };
      break;
    default: return getRandomEvent();
  }
  return ret;
}

bool WindowRenderer::pollEvent(Event& event, Event::EventType type) {
  for (int i : All(eventQueue))
    if (eventQueue[i].type == type) {
      event = eventQueue[i];
      eventQueue.erase(eventQueue.begin() + i);
      return true;
    }
  Event ev;
  while (display->pollEvent(ev)) {
    if (ev.type != type)
      eventQueue.push_back(ev);
    else {
      event = ev;
      return true;
    }
  }
  return false;
}

bool WindowRenderer::pollEvent(Event& ev) {
  if (monkey) {
    if (Random.roll(2))
      return false;
    ev = getRandomEvent();
    return true;
  } else if (!eventQueue.empty()) {
      ev = eventQueue.front();
      eventQueue.pop_front();
      return true;
  } else
    return display->pollEvent(ev);
}

void WindowRenderer::flushEvents(Event::EventType type) {
  Event ev;
  while (display->pollEvent(ev)) {
    if (ev.type != type)
      eventQueue.push_back(ev);
  }
}

void WindowRenderer::flushAllEvents() {
  Event ev;
  while (display->pollEvent(ev));
}

void WindowRenderer::waitEvent(Event& ev) {
  if (monkey) {
    ev = getRandomEvent();
    return;
  } else {
    if (!eventQueue.empty()) {
      ev = eventQueue.front();
      eventQueue.pop_front();
    } else
      display->waitEvent(ev);
  }
}

Vec2 WindowRenderer::getMousePos() {
  if (monkey)
    return Vec2(Random.get(getWidth()), Random.get(getHeight()));
  auto pos = Mouse::getPosition(*display);
  return Vec2(pos.x, pos.y);
}

void WindowRenderer::startMonkey() {
  monkey = true;
}
  
bool WindowRenderer::isMonkey() {
  return monkey;
}
