#include "stdafx.h"
#include "buff_info.h"
#include "creature.h"
#include "pretty_archive.h"

void applyMessage(const BuffMessageInfo& msg, const Creature* c) {
  msg.visit(
    [c](const YouMessage& msg) {
      c->you(msg.type, msg.message);
    },
    [c](const VerbMessage& msg) {
      c->verb(msg.secondPerson, msg.thirdPerson, msg.message);
    }
  );
}

void serialize(PrettyInputArchive& ar1, BuffMessageInfo& info, const unsigned int) {
  string s = ar1.peek();
  if (auto l = EnumInfo<MsgType>::fromStringSafe(s)) {
    YouMessage msg;
    ar1(msg);
    info = msg;
  } else {
    VerbMessage msg;
    ar1(msg);
    info = msg;
  }
}
