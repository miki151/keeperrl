#include "stdafx.h"
#include "highscores.h"
#include "view.h"
#include "campaign_type.h"
#include "parse_game.h"
#include "scripted_ui.h"
#include "scripted_ui_data.h"

const static int highscoreVersion = 2;

Highscores::Highscores(const FilePath& local) : localPath(local) {
  localScores = fromFile(localPath);
}

void Highscores::add(Score s) {
  s.version = highscoreVersion;
  localScores.push_back(s);
  saveToFile(localScores, localPath);
}

void Highscores::saveToFile(const vector<Score>& scores, const FilePath& path) {
  CompressedOutput out(path.getPath());
  out.getArchive() << scores;
}

bool Highscores::Score::operator == (const Score& s) const {
  return gameId == s.gameId && playerName == s.playerName && worldName == s.worldName && gameResult == s.gameResult
    && points == s.points && turns == s.turns;
}

vector<Highscores::Score> Highscores::fromFile(const FilePath& path) {
  vector<Highscores::Score> scores;
  try {
    CompressedInput in(path.getPath());
    in.getArchive() >> scores;
  } catch (...) {}
  return scores.filter([](const Score& s) { return s.version == highscoreVersion;});
}

bool sortByMostPoints(const Highscores::Score& a, const Highscores::Score& b) {
  if (a.gameWon) {
    if (!b.gameWon)
      return true;
    else
      return a.points > b.points;
  } else
  if (b.gameWon)
    return false;
  else
    return a.points > b.points;
}

typedef Highscores::Score Score;

string getPointsColumn(const Score& score) {
  return toString(score.points) + " points";
}

namespace {
  struct HighscoreElem {
    string text;
    string score;
    bool highlight;
  };
}

static vector<HighscoreElem> fillScores(optional<Score> lastElem, vector<Score> scores) {
  vector<HighscoreElem> list;
  sort(scores.begin(), scores.end(), sortByMostPoints);
  for (auto& score : scores) {
    bool highlight = lastElem == score;
    string points = getPointsColumn(score);
    if (score.gameResult.empty())
      list.push_back({score.playerName + " of " + score.worldName, points, highlight});
    else
      list.push_back({score.playerName + " of " + score.worldName +
          ", " + score.gameResult, points, highlight});
  }
  return list;
}

void Highscores::present(View* view, optional<Score> lastAdded) const {
  while (1) {
    ScriptedUIDataElems::Record main;
    ScriptedUIDataElems::List list;
    auto scores = fillScores(lastAdded, localScores);
    for (auto& score : scores) {
      ScriptedUIDataElems::Record elemData;
      elemData.elems["score"] = TString(score.score);
      elemData.elems["text"] = TString(score.text);
      if (score.highlight)
        elemData.elems["highlight"] = TString("xyz"_s);
      list.push_back(std::move(elemData));
    }
    main.elems["list"] = std::move(list);
    bool exit = false;
    main.elems["EXIT"] = ScriptedUIDataElems::Callback {
        [&exit] { exit = true; return true; }
    };
    view->scriptedUI("highscores", main);
    if (exit)
      break;
  }
}

