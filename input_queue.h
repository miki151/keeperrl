#ifndef _INPUT_QUEUE
#define _INPUT_QUEUE

#include "user_input.h"

class InputQueue {
  public:
  void push(UserInput);
  bool pop(UserInput&);

  private:
  queue<UserInput> q;
};


#endif
