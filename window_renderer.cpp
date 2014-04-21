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
  Event::EventType type = Event::EventType(Random.getRandom(int(Event::Count)));
  Event ret;
  ret.type = type;
  int modProb = 5;
  switch (type) {
    case Event::KeyPressed:
      ret.key = {Keyboard::Key(Random.getRandom(int(Keyboard::Key::KeyCount))), Random.roll(modProb),
          Random.roll(modProb), Random.roll(modProb), Random.roll(modProb) };
      break;
    case Event::MouseButtonReleased:
    case Event::MouseButtonPressed:
      ret.mouseButton = { chooseRandom({Mouse::Left, Mouse::Right}), Random.getRandom(getWidth()),
        Random.getRandom(getHeight()) };
      break;
    case Event::MouseMoved:
      ret.mouseMove = { Random.getRandom(getWidth()), Random.getRandom(getHeight()) };
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
    return Vec2(Random.getRandom(getWidth()), Random.getRandom(getHeight()));
  auto pos = Mouse::getPosition(*display);
  return Vec2(pos.x, pos.y);
}

void WindowRenderer::startMonkey() {
  monkey = true;
}
  
bool WindowRenderer::isMonkey() {
  return monkey;
}
