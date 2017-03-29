#pragma once

#include "util.h"

class TutorialInfo;
class Game;

class Tutorial {
  public:
  Tutorial();
  void refreshInfo(const Game*, optional<TutorialInfo>&) const;
  void continueTutorial(const Game*);
  void goBack();

  static void createTutorial(Game&);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  string getMessage() const;
  bool canContinue(const Game*) const;

  enum class State;
  State SERIAL(state);
  EnumSet<TutorialHighlight> getHighlights(const Game*) const;
  vector<Vec2> getHighlightedSquares(const Game*) const;
};
