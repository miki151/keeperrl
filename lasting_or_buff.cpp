#include "lasting_or_buff.h"
#include "content_factory.h"
#include "game_time.h"
#include "color.h"
#include "buff_info.h"
#include "creature.h"
#include "game.h"
#include "creature_attributes.h"

string getName(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [](LastingEffect e) {
      return LastingEffects::getName(e);
    },
    [f](BuffId id) {
      return f->buffs.at(id).name;
    }
  );
}

string getDescription(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [](LastingEffect e) {
      return LastingEffects::getDescription(e);
    },
    [f](BuffId id) {
      return f->buffs.at(id).description;
    }
  );
}

string getAdjective(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [](LastingEffect e) {
      return LastingEffects::getAdjective(e);
    },
    [f](BuffId id) {
      return f->buffs.at(id).adjective;
    }
  );
}

int getPrice(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [](LastingEffect e) {
      return LastingEffects::getPrice(e);
    },
    [f](BuffId id) {
      return f->buffs.at(id).price;
    }
  );
}

bool addEffect(const LastingOrBuff& l, Creature* c, optional<TimeInterval> duration, bool msg) {
  return l.visit(
    [&](LastingEffect e) {
      return c->addEffect(e, duration.value_or(LastingEffects::getDuration(c, e)), msg);
    },
    [&](BuffId id) {
      return c->addEffect(id, *duration, msg);
    }
  );
}

bool addEffect(const LastingOrBuff& l, Creature* c, optional<TimeInterval> duration, GlobalTime time,
    const ContentFactory* factory) {
  return l.visit(
    [&](LastingEffect e) {
      return c->addEffect(e, duration.value_or(LastingEffects::getDuration(c, e)), time, false);
    },
    [&](BuffId id) {
      return c->addEffect(id, *duration, time, factory, false);
    }
  );
}

bool removeEffect(const LastingOrBuff& l, Creature* c, bool msg) {
  return l.visit(
    [&](LastingEffect e) {
      return c->removeEffect(e, msg);
    },
    [&](BuffId id) {
      return c->removeEffect(id, msg);
    }
  );
}

bool addPermanentEffect(const LastingOrBuff& l, Creature* c, bool msg, const ContentFactory* f) {
  return l.visit(
    [&](LastingEffect e) {
      return c->addPermanentEffect(e, 1, msg, f);
    },
    [&](BuffId id) {
      return c->addPermanentEffect(id, 1, msg, f);
    }
  );
}

bool removePermanentEffect(const LastingOrBuff& l, Creature* c, bool msg) {
  return l.visit(
    [&](LastingEffect e) {
      return c->removePermanentEffect(e, 1, msg);
    },
    [&](BuffId id) {
      return c->removePermanentEffect(id, 1, msg);
    }
  );
}

bool isConsideredBad(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [&](LastingEffect e) {
      return LastingEffects::isConsideredBad(e);
    },
    [&](BuffId id) {
      return f->buffs.at(id).consideredBad;
    }
  );
}

bool isAffected(const Creature* c, const LastingOrBuff& l) {
  return l.visit(
    [&](LastingEffect e) {
      return c->isAffected(e);
    },
    [&](BuffId id) {
      return c->isAffected(id);
    }
  );
}

bool isAffected(const Creature* c, const LastingOrBuff& l, GlobalTime time) {
  return l.visit(
    [&](LastingEffect e) {
      return c->isAffected(e, time);
    },
    [&](BuffId id) {
      return c->isAffected(id);
    }
  );
}

bool isAffectedPermanently(const Creature* c, const LastingOrBuff& l) {
  return l.visit(
    [&](LastingEffect e) {
      return c->getAttributes().isAffectedPermanently(e);
    },
    [&](BuffId id) {
      return c->isAffectedPermanently(id);
    }
  );
}

static bool shouldAllyApplyInDanger(const LastingOrBuff& l, const Creature* target, const ContentFactory* factory) {
  return l.visit(
    [&](LastingEffect e) {
      return LastingEffects::shouldAllyApplyInDanger(target, e);
    },
    [&](BuffId id) {
      return factory->buffs.at(id).combatConsumable;
    }
  );
}

static bool shouldEnemyApply(const LastingOrBuff& l, const Creature* target, const ContentFactory* factory) {
  return l.visit(
    [&](LastingEffect e) {
      return LastingEffects::shouldEnemyApply(target, e);
    },
    [&](BuffId id) {
      return factory->buffs.at(id).consideredBad;
    }
  );
}

EffectAIIntent shouldAIApply(const LastingOrBuff& l, bool enemy, const Creature* victim) {
  auto factory = victim->getGame()->getContentFactory();
  if (shouldEnemyApply(l, victim, factory))
    return enemy ? 1 : -1;
  if (enemy)
    return -1;
  bool isDanger = [&] {
    if (auto intent = victim->getLastCombatIntent())
      return intent->time > *victim->getGlobalTime() - 5_visible;
    return false;
  }();
  if (isDanger == shouldAllyApplyInDanger(l, victim, factory))
    return 1;
  else
    return -1;
}

Color getColor(const LastingOrBuff& l, const ContentFactory* f) {
  return l.visit(
    [](LastingEffect e) {
      return LastingEffects::getColor(e);
    },
    [f](BuffId id) {
      return f->buffs.at(id).color;
    }
  );
}

void serialize(PrettyInputArchive& ar1, LastingOrBuff& b, const unsigned int) {
  string s = ar1.peek();
  if (auto l = EnumInfo<LastingEffect>::fromStringSafe(s)) {
    LastingEffect e;
    ar1(e);
    b = e;
  } else {
    BuffId e;
    ar1(e);
    b = e;
  }
}