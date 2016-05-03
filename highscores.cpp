#include "stdafx.h"
#include "highscores.h"
#include "view.h"
#include "file_sharing.h"
#include "options.h"

Highscores::Highscores(const string& local, FileSharing& sharing, Options* o)
    : localPath(local), fileSharing(sharing), options(o) {
  localScores = fromFile(localPath);
  remoteScores = fromString(fileSharing.downloadHighscores());
  thread t([=] { fileSharing.uploadHighscores(localPath); });
  t.detach();
}

void Highscores::add(Score s) {
  localScores.push_back(s);
  sortScores(localScores);
  saveToFile(localScores, localPath);
  remoteScores.push_back(s);
  sortScores(remoteScores);
//  thread t([=] {
      fileSharing.uploadHighscores(localPath);
  //});
//  t.detach();
}

void Highscores::sortScores(vector<Score>& scores) {
}

vector<Highscores::Score> Highscores::fromStream(istream& in) {
  vector<Score> ret;
  while (1) {
    char buf[1000];
    in.getline(buf, 1000);
    if (!in)
      break;
    if (auto score = Score::parse(buf))
      ret.push_back(*score);
  }
  sortScores(ret);
  return ret;
}

const char delim = ',';

string Highscores::Score::toString() const {
  return gameId + delim +
    playerName + delim +
    worldName + delim +
    gameResult + delim +
    ::toString<int>(gameWon) + delim +
    ::toString(points) + delim +
    ::toString(turns) + delim +
    ::toString<int>(gameType);
}

void Highscores::saveToFile(const vector<Score>& scores, const string& path) {
  ofstream of(path);
  for (const Score& score : scores) {
    of << score.toString() << std::endl;
  }
}

bool Highscores::Score::operator == (const Score& s) const {
  return gameId == s.gameId && playerName == s.playerName && worldName == s.worldName && gameResult == s.gameResult
    && points == s.points && turns == s.turns;
}

vector<Highscores::Score> Highscores::fromFile(const string& path) {
  ifstream in(path);
  return fromStream(in);
}

vector<Highscores::Score> Highscores::fromString(const string& s) {
  stringstream in(s);
  return fromStream(in);
}

optional<Highscores::Score> Highscores::Score::parse(const string& buf) {
  vector<string> p = split(buf, {','});
  if (p.size() != 8 || !fromStringSafe<int>(p[4]) || !fromStringSafe<int>(p[5]) || !fromStringSafe<int>(p[6])
      || !fromStringSafe<int>(p[7]))
#ifndef RELEASE
    return CONSTRUCT(Score, c.playerName = "ERROR: " + buf;);
#else
    return none;
#endif
  else {
    return CONSTRUCT(Score,
        c.gameId = p[0];
        c.playerName = p[1];
        c.worldName = p[2];
        c.gameResult = p[3];
        c.gameWon = ::fromString<int>(p[4]);
        c.points = ::fromString<int>(p[5]);
        c.turns = ::fromString<int>(p[6]);
        c.gameType = Score::GameType(::fromString<int>(p[7]));
    );
  }
}

typedef Highscores::Score Score;
static HighscoreList fillScores(const string& name, vector<Score> scores, optional<Score> lastElem,
    HighscoreList::SortBy sortBy) {
  HighscoreList list { name, sortBy };
  switch (sortBy) {
    case HighscoreList::SCORE: 
      sort(scores.begin(), scores.end(), [] (const Score& a, const Score& b) -> bool {
          return a.points > b.points;
      });
      break;
    case HighscoreList::TURNS: 
      sort(scores.begin(), scores.end(), [] (const Score& a, const Score& b) -> bool {
          return a.turns < b.turns;
      });
      break;
  }
  for (auto& score : scores) {
    bool highlight = lastElem == score;
    if (score.gameResult.empty())
      list.scores.push_back({score.playerName + " of " + score.worldName, score.points, score.turns, highlight});
    else
      list.scores.push_back({score.playerName + " of " + score.worldName +
          ", " + score.gameResult, score.points, score.turns, highlight});
  }
  return list;
}

void Highscores::present(View* view, optional<Score> lastAdded) const {
  vector<HighscoreList> lists;
  lists.push_back(fillScores("Keepers", filter(localScores, [] (const Score& s) { return s.gameType == s.KEEPER;}),
      lastAdded, lists.back().SCORE)); 
  lists.push_back(fillScores("Adventurers", filter(localScores,
      [] (const Score& s) { return s.gameType == s.ADVENTURER;}), lastAdded, lists.back().SCORE)); 
  lists.push_back(fillScores("Fastest wins", filter(localScores,
      [] (const Score& s) { return s.gameType == s.KEEPER && s.gameWon == true;}), lastAdded, lists.back().TURNS)); 
  lists.push_back(fillScores("Keepers", filter(remoteScores,
      [] (const Score& s) { return s.gameType == s.KEEPER;}), lastAdded, lists.back().SCORE)); 
  lists.push_back(fillScores("Adventurers", filter(remoteScores,
      [] (const Score& s) { return s.gameType == s.ADVENTURER;}), lastAdded, lists.back().SCORE)); 
  lists.push_back(fillScores("Fastest wins", filter(remoteScores,
      [] (const Score& s) { return s.gameType == s.KEEPER && s.gameWon == true;}),
          lastAdded, lists.back().TURNS)); 
  view->presentHighscores(lists);
}

