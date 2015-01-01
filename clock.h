#ifndef _CLOCK_H
#define _CLOCK_H

#include <SFML/Graphics.hpp>

class Clock {
  public:
  int getMillis();
  void setMillis(int time);
  void pause();
  void cont();
  bool isPaused();
  int getRealMillis();

  private:
  int pausedTime = 0;
  int lastPause = -1;
  sf::Clock sfClock;
};

class Intervalometer {
  public:
  Intervalometer(int frequencyMillis);
  int getCount(int currentTimeMillis);

  private:
  int frequency;
  int lastUpdate = 0;
};

#endif
