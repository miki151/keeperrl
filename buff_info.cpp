#include "stdafx.h"
#include "buff_info.h"
#include "creature.h"
#include "pretty_archive.h"

void applyMessage(const BuffMessageInfo& msg, const Creature* c) {
  msg.visit(
    [&](const YouMessage& msg) {
      c->you(msg.type, msg.message);
    },
    [&](const VerbMessage& msg) {
      if (auto s1 = msg.secondPerson.text.getReferenceMaybe<TSentence>())
        if (auto s2 = msg.thirdPerson.text.getReferenceMaybe<TSentence>())
          c->verb(s1->id, s2->id, msg.message);
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

template <class Archive>
void BuffInfo::serialize(Archive& ar, const unsigned int version) {
  ar(NAMED(modifyDamageAttr), OPTION(inheritsFromSteed), OPTION(canWishFor), OPTION(canAbsorb), OPTION(combatConsumable), OPTION(fx), OPTION(defenseMultiplier), OPTION(defenseMultiplierAttr), NAMED(hatedGroupName), NAMED(name), OPTION(addedMessage), OPTION(removedMessage), NAMED(startEffect), NAMED(tickEffect), NAMED(endEffect), OPTION(stacks), OPTION(consideredBad), NAMED(description), OPTION(price), NAMED(color), NAMED(adjective), OPTION(efficiencyMultiplier));
  if (version >= 1)
    ar(OPTION(specialAttr));
  if (version >= 2)
    ar(OPTION(hiddenPredicate));
}

SERIALIZABLE(BuffInfo)
template void BuffInfo::serialize(PrettyInputArchive&, unsigned);