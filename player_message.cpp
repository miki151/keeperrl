#include "stdafx.h"
#include "player_message.h"
#include "view.h"
#include "scripted_ui_data.h"
#include "scripted_ui.h"
#include "gui_elem.h"

PlayerMessage::PlayerMessage(const TString& t, MessagePriority p)
    : text(TSentence("MAKE_SENTENCE", std::move(t))), priority(p),
    freshness(1) {}

PlayerMessage::PlayerMessage(const TSentence& s, MessagePriority p) : PlayerMessage(TString(std::move(s)), p) {
}

PlayerMessage::PlayerMessage(const TStringId& s, MessagePriority p) : PlayerMessage(TString(std::move(s)), p) {
}

void PlayerMessage::presentMessages(View* view, const vector<PlayerMessage>& messages) {
  auto list = ScriptedUIDataElems::List {};
  for (int i : All(messages))
    list.push_back(ScriptedUIDataElems::Record {{
      {EnumInfo<MessagePriority>::getString(messages[i].priority), TString(messages[i].getText(view))}
    }});
  if (messages.empty())
    list.push_back(ScriptedUIDataElems::Record {{
      {"NORMAL", TString(TStringId("NO_MESSAGES_YET"))}
    }});
  ScriptedUIState state;
  state.scrollPos[0].set(10000000, milliseconds{0});
  view->scriptedUI("message_history", list, state);
}

string PlayerMessage::getText(View* view) const {
  return view->translate(text);
}

string PlayerMessage::getText(GuiFactory* view) const {
  return view->translate(text);
}

double PlayerMessage::getFreshness() const {
  return freshness;
}

void PlayerMessage::setFreshness(double val) {
  freshness = val;
}

MessagePriority PlayerMessage::getPriority() const {
  return priority;
}

PlayerMessage& PlayerMessage::setPosition(Position pos) {
  position = pos;
  return *this;
}

optional<Position> PlayerMessage::getPosition() const {
  return position;
}

PlayerMessage& PlayerMessage::setCreature(UniqueEntity<Creature>::Id id) {
  creature = id;
  return *this;
}

optional<UniqueEntity<Creature>::Id> PlayerMessage::getCreature() const {
  return creature;
}

bool PlayerMessage::isClickable() const {
  return position || creature;
}

int PlayerMessage::getHash() const {
  return combineHash(text, priority, freshness);
}

template <class Archive>
void PlayerMessage::serialize(Archive& ar, const unsigned int version) {
  ar(SUBCLASS(UniqueEntity), text, priority, freshness, announcementTitle, position, creature);
}

SERIALIZABLE(PlayerMessage);
SERIALIZATION_CONSTRUCTOR_IMPL(PlayerMessage);
