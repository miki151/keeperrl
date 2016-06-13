#include "stdafx.h"
#include "square_interaction.h"
#include "square_apply_type.h"
#include "position.h"
#include "game.h"

SquareApplyType SquareInteractions::getApplyType(SquareInteraction inter) {
  switch (inter) {
    case SquareInteraction::KEEPER_BOARD: return SquareApplyType::NOTICE_BOARD;
  }
}

void SquareInteractions::apply(SquareInteraction inter, Position pos, Creature* c) {
  switch (inter) {
    case SquareInteraction::KEEPER_BOARD:
      pos.getGame()->handleMessageBoard(pos, c);
      break;
  }
}

double SquareInteractions::getApplyTime(SquareInteraction inter) {
  switch (inter) {
    default: return 1;
  }
}

