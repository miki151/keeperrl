#ifndef _MESSAGE_BUFFER_H
#define _MESSAGE_BUFFER_H

#include "util.h"
#include "view.h"

class MessageBuffer {
  public:
  void addMessage(string msg);
  void addImportantMessage(string msg);
  void initialize(View* view);
  void showHistory();

  static string important(const string& msg);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;

  private:
  static bool isImportant(const string& msg);
  static void removeImportant(string& msg);
  vector<string> SERIAL(messages);
  View* view;
};

extern MessageBuffer messageBuffer;

#endif
