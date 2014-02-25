#ifndef _MESSAGE_BUFFER_H
#define _MESSAGE_BUFFER_H

#include "util.h"
#include "view.h"

class MessageBuffer {
  public:
  void addMessage(string msg);
  void initialize(View* view);
  void showHistory();

  static string important(const string& msg);

  private:
  static bool isImportant(const string& msg);
  static void removeImportant(string& msg);
  vector<string> messages;
  View* view;
};

extern MessageBuffer messageBuffer;

#endif
