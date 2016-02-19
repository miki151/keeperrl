#ifndef _SUNLIGHT_INFO_H
#define _SUNLIGHT_INFO_H

enum class SunlightState { DAY, NIGHT};

struct SunlightInfo {
  double lightAmount;
  double timeRemaining;
  SunlightState state;
  const char* getText();
};

#endif
