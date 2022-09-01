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

#include "scripted_ui_data.h"
#include "stdafx.h"

#include "view.h"
#include "progress_meter.h"
#include "item.h"
#include "view_object.h"


View::View() {

}

View::~View() {
}

void View::doWithSplash(const string& text, int totalProgress,
    function<void(ProgressMeter&)> fun, function<void()> cancelFun) {
  ProgressMeter meter(1.0 / totalProgress);
  displaySplash(&meter, text, cancelFun);
  thread t = makeThread([fun, &meter, this] {
      try {
        fun(meter);
        clearSplash();
      } catch (Progress::InterruptedException) {}
    });
  try {
    refreshView();
    t.join();
  } catch (GameExitException e) {
    Progress::interrupt();
    t.join();
    throw e;
  }
}

void View::presentText(const string& title, const string& text) {
  auto data = ScriptedUIDataElems::Record {{
    {"text", std::move(text)}
  }};
  if (!title.empty())
    data.elems["title"] = title;
  ScriptedUIState state;
  scriptedUI("present_text", data, state);
}

void View::presentTextBelow(const string& title, const string& text) {
  auto data = ScriptedUIDataElems::Record {{
    {"text", std::move(text)}
  }};
  if (!title.empty())
    data.elems["title"] = title;
  ScriptedUIState state;
  scriptedUI("present_text_below", data, state);
}

optional<int> View::multiChoice(const string& message, const vector<string>& elems) {
  optional<int> ret;
  auto list = ScriptedUIDataElems::List {};
  for (int i : All(elems))
    list.push_back(ScriptedUIDataElems::Record{{
        {"text", elems[i]},
        {"callback", ScriptedUIDataElems::Callback { [i, &ret] {
          ret = i;
          return true;
        }}}
    }});
  auto data = ScriptedUIDataElems::Record {{
    {"title", message},
    {"options", std::move(list)}
  }};
  ScriptedUIState state;
  scriptedUI("multi_choice", data, state);
  return ret;
}

bool View::yesOrNoPrompt(const string& message, bool defaultNo, ScriptedUIId id) {
  bool ret = false;
  auto data = ScriptedUIDataElems::Record {{
    {"callback"_s, ScriptedUIDataElems::Callback{[&ret] { ret = true; return true; }}},
    {"message"_s, message},
  }};
  ScriptedUIState state;
  scriptedUI(id, data, state);
  return ret;
}

void View::windowedMessage(ViewIdList id, const string& message) {
  ScriptedUIState state;
  auto data = ScriptedUIDataElems::Record{};
  data.elems["message"] = message;
  data.elems["view_id"] = id;
  scriptedUI("unlock_message", data, state);
}

bool View::confirmConflictingItems(const ContentFactory* f, const vector<Item*>& items) {
  auto itemsList = ScriptedUIDataElems::List{};
  for (auto it : items) {
    auto data = ScriptedUIDataElems::Record{};
    data.elems["item"] = it->getNameAndModifiers(f);
    data.elems["view_id"] = it->getViewObject().getViewIdList();
    itemsList.push_back(std::move(data));
  }
  if (!items.empty()) {
    bool confirmed = false;
    ScriptedUIState state;
    auto data = ScriptedUIDataElems::Record{};
    data.elems["items"] = std::move(itemsList);
    data.elems["callback"] = ScriptedUIDataElems::Callback{[&confirmed] { confirmed = true; return true; }};
    scriptedUI("conflicting_items", data, state);
    return confirmed;
  }
  return true;
}