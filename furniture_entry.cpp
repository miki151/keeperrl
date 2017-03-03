#include "stdafx.h"
#include "furniture_entry.h"
#include "level.h"
#include "position.h"
#include "creature.h"
#include "creature_attributes.h"
#include "furniture.h"
#include "player_message.h"

void FurnitureEntry::handle(FurnitureEntryType type, const Furniture* f, Creature* c) {
  switch (type) {
    case FurnitureEntryType::SOKOBAN:
      if (c->getAttributes().isBoulder()) {
        Position pos = c->getPosition();
        pos.globalMessage(c->getName().the() + " fills the " + f->getName());
        pos.removeFurniture(f);
        c->die(nullptr, false, false);
      } else {
        if (!c->isAffected(LastingEffect::FLYING))
          c->you(MsgType::FALL, "into the " + f->getName() + "!");
        else
          c->you(MsgType::ARE, "sucked into the " + f->getName() + "!");
        auto level = c->getPosition().getLevel();
        level->changeLevel(getOnlyElement(level->getAllStairKeys()), c);
      }
      break;
  }
}
