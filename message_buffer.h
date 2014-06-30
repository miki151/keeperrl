/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _MESSAGE_BUFFER_H
#define _MESSAGE_BUFFER_H

#include "util.h"

class View;

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
