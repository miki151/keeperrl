#ifndef _TIMER_H
#define _TIMER_H
 

class TimerVar {
  public:
  void set(double t);

  void unset();

  bool isFinished(double currentTime);

  operator bool() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  double timeout = -1;
};

#endif
