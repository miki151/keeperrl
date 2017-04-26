#pragma once

#include "util.h"

class TutorialInfo;
class Game;
class ImmigrantInfo;

class Tutorial {
  public:
  Tutorial();
  void refreshInfo(WConstGame, optional<TutorialInfo>&) const;
  void continueTutorial(WConstGame);
  void goBack();
  bool showImmigrant(const ImmigrantInfo&) const;
  EnumSet<TutorialHighlight> getHighlights(WConstGame) const;
  bool blockAutoEquipment() const;

  static void createTutorial(Game&);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  string getMessage() const;
  bool canContinue(WConstGame) const;

  enum class State;
  State SERIAL(state);
  vector<Vec2> getHighlightedSquaresHigh(WConstGame) const;
  vector<Vec2> getHighlightedSquaresLow(WConstGame) const;
  void onNewState(WConstGame);
};
