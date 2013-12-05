#ifndef _TIMER_H
#define _TIMER_H
 

class TimerVar {
  public:
  void set(double t) {
    timeout = t;
  }

  void unset() {
    timeout = -1;
  }

  bool isFinished(double currentTime) {
    if (timeout >= 0 && currentTime >= timeout) {
      timeout = -1;
      return true;
    }
    return false;
  }

  operator bool() const {
    return timeout >= 0;
  }

  private:
  double timeout = -1;
};

#endif
