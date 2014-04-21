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
