#include "stdafx.h"
#include "input_queue.h"

void InputQueue::push(UserInput input) {
  q.push(input);
}

bool InputQueue::pop(UserInput& input) {
  if (q.empty())
    return false;
  input = q.front();
  q.pop();
  return true;
}

