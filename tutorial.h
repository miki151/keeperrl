#pragma once

#include "util.h"

class TutorialInfo;
class Game;
class GameConfig;
class ContentFactory;

class Tutorial {
  public:
  Tutorial();
  void refreshInfo(const Game*, optional<TutorialInfo>&) const;
  void continueTutorial(const Game*);
  void goBack();
  EnumSet<TutorialHighlight> getHighlights(const Game*) const;
  bool blockAutoEquipment() const;

  static void createTutorial(Game&, const ContentFactory* factory);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  using State = TutorialState;
  State getState() const;

  private:
  string getMessage() const;
  bool canContinue(const Game*) const;

  State SERIAL(state);
  vector<Vec2> getHighlightedSquaresHigh(const Game*) const;
  vector<Vec2> getHighlightedSquaresLow(const Game*) const;
  void onNewState(const Game*);
  Vec2 SERIAL(entrance);
  optional<string> getWarning(const Game*) const;
};
