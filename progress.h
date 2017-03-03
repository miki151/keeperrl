#pragma once

// A simple mechanism of interrupting another thread's execution
class Progress {
  public:
  class InterruptedException {};
  static void interrupt();
  static void checkIfInterrupted();
};
