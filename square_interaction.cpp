#include "stdafx.h"
#include "square_interaction.h"
#include "square_apply_type.h"
#include "position.h"
#include "game.h"
#include "creature.h"
#include "level.h"

SquareApplyType SquareInteractions::getApplyType(SquareInteraction inter) {
  switch (inter) {
    case SquareInteraction::KEEPER_BOARD: return SquareApplyType::NOTICE_BOARD;
    case SquareInteraction::STAIRS: return SquareApplyType::USE_STAIRS;
  }
}

void SquareInteractions::apply(SquareInteraction inter, Position pos, Creature* c) {
  switch (inter) {
    case SquareInteraction::KEEPER_BOARD:
      pos.getGame()->handleMessageBoard(pos, c);
      break;
    case SquareInteraction::STAIRS:
      c->getLevel()->changeLevel(*pos.getLandingLink(), c);
      break;
  }
}

double SquareInteractions::getApplyTime(SquareInteraction inter) {
  switch (inter) {
    default: return 1;
  }
}

