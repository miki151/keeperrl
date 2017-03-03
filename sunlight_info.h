#pragma once

enum class SunlightState { DAY, NIGHT};

class SunlightInfo {
  public:
  const char* getText() const;
  static const char* getText(SunlightState);
  void update(double currentTime);
  SunlightState getState() const;
  double getLightAmount() const;
  double getTimeRemaining() const;

  private:
  double lightAmount;
  double timeRemaining;
  SunlightState state;
};

