#pragma once

#include "util.h"

class TutorialInfo;
class Game;
class GameConfig;

class Tutorial {
  public:
  Tutorial();
  void refreshInfo(WConstGame, optional<TutorialInfo>&) const;
  void continueTutorial(WConstGame);
  void goBack();
  EnumSet<TutorialHighlight> getHighlights(WConstGame) const;
  bool blockAutoEquipment() const;

  static void createTutorial(Game&, const GameConfig*);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  using State = TutorialState;
  State getState() const;

  private:
  string getMessage() const;
  bool canContinue(WConstGame) const;

  State SERIAL(state);
  vector<Vec2> getHighlightedSquaresHigh(WConstGame) const;
  vector<Vec2> getHighlightedSquaresLow(WConstGame) const;
  void onNewState(WConstGame);
  Vec2 SERIAL(entrance);
  optional<string> getWarning(WConstGame) const;
};
