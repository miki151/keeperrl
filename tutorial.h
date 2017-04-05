#pragma once

#include "util.h"

class TutorialInfo;
class Game;

class Tutorial {
  public:
  Tutorial();
  void refreshInfo(const WGame, optional<TutorialInfo>&) const;
  void continueTutorial(const WGame);
  void goBack();
  bool showImmigrant() const;

  static void createTutorial(Game&);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  string getMessage() const;
  bool canContinue(const WGame) const;

  enum class State;
  State SERIAL(state);
  EnumSet<TutorialHighlight> getHighlights(const WGame) const;
  vector<Vec2> getHighlightedSquaresHigh(const WGame) const;
  vector<Vec2> getHighlightedSquaresLow(const WGame) const;
};
