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

#include "stdafx.h"

#include "message_buffer.h"
#include "view.h"

template <class Archive> 
void MessageBuffer::serialize(Archive& ar, const unsigned int version) { 
  ar & SVAR(messages);
  CHECK_SERIAL;
}

SERIALIZABLE(MessageBuffer);

MessageBuffer messageBuffer;

void MessageBuffer::addMessage(string msg) {
  Debug() << "MSG " << msg;
  CHECK(view != nullptr) << "Message buffer not initialized.";
  if (msg == "")
    return;
  bool imp = isImportant(msg);
  if (imp)
    removeImportant(msg);
  msg = makeSentence(msg);
  if (imp)
    view->addImportantMessage(msg);
  else {
    view->addMessage(msg);
  }
  messages.push_back(msg);
}

void MessageBuffer::addImportantMessage(string msg) {
  addMessage(important(msg));
}

void MessageBuffer::initialize(View* view) {
  this->view = view;
  messages.clear();
}

void MessageBuffer::showHistory() {
  view->presentList("Messages:", View::getListElem(messages), true); 
}

const static string importantPref = "IMPORTANT";

string MessageBuffer::important(const string& msg) {
  return importantPref + msg;
}

bool MessageBuffer::isImportant(const string& msg) {
  return msg.size() > importantPref.size() && msg.substr(0, importantPref.size()) == importantPref;
}

void MessageBuffer::removeImportant(string& msg) {
  CHECK(isImportant(msg));
  msg = msg.substr(importantPref.size());
}
