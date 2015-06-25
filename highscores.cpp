#include "stdafx.h"
#include "highscores.h"
#include "view.h"
#include "file_sharing.h"
#include "options.h"

Highscores::Highscores(const string& local, FileSharing& sharing, Options* o)
    : localPath(local), fileSharing(sharing), options(o) {
  localScores = fromFile(localPath);
  remoteScores = fromString(fileSharing.downloadHighscores());
  if (options->getBoolValue(OptionId::ONLINE)) {
    thread t([=] { fileSharing.uploadHighscores(localPath); });
    t.detach();
  }
}

void Highscores::add(Score s) {
  localScores.push_back(s);
  remoteScores.push_back(s);
  sortScores(localScores);
  sortScores(remoteScores);
  saveToFile(localScores, localPath);
  if (options->getBoolValue(OptionId::ONLINE)) {
    thread t([=] { fileSharing.uploadHighscores(localPath); });
    t.detach();
  }
}

void Highscores::sortScores(vector<Score>& scores) {
  sort(scores.begin(), scores.end(), [] (const Score& a, const Score& b) -> bool {
      return a.points > b.points;
  });
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
  return gameId + delim + playerName + delim + worldName + delim + gameResult + delim + ::toString(points);
}

void Highscores::saveToFile(const vector<Score>& scores, const string& path) {
  ofstream of(path);
  for (const Score& score : scores) {
    of << score.toString() << std::endl;
  }
}

bool Highscores::Score::operator == (const Score& s) const {
  return gameId == s.gameId && playerName == s.playerName && worldName == s.worldName && gameResult == s.gameResult
    && points == s.points;
}

vector<Highscores::Score> Highscores::fromFile(const string& path) {
  ifstream in(path);
  return fromStream(in);
}

vector<Highscores::Score>  Highscores::fromString(const string& s) {
  stringstream in(s);
  return fromStream(in);
}

optional<Highscores::Score> Highscores::Score::parse(const string& buf) {
  vector<string> p = split(buf, {','});
  if (p.size() != 5 || !fromStringSafe<int>(p[4]))
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
        c.points = ::fromString<int>(p[4]);
    );
  }
}

typedef Highscores::Score Score;
static void fillScores(vector<ListElem>& elems, const vector<Score>& scores, optional<Score> lastElem) {
  for (const Score& score : scores) {
    ListElem::ElemMod highlight = (!lastElem || lastElem == score)
        ? ListElem::NORMAL : ListElem::INACTIVE;
    if (score.gameResult.empty())
      elems.push_back(ListElem(score.playerName + " of " + score.worldName,
          toString(score.points) + " points", highlight));
    else
      elems.push_back(ListElem(score.playerName + " of " + score.worldName +
          ", " + score.gameResult,
          toString(score.points) + " points", highlight));
  }
}

void Highscores::present(View* view, optional<Score> lastAdded) const {
  vector<ListElem> elems { ListElem("Local highscores:", ListElem::TITLE)};
  fillScores(elems, localScores, lastAdded); 
  if (options->getBoolValue(OptionId::ONLINE)) {
    elems.push_back(ListElem("Online highscores:", ListElem::TITLE));
    fillScores(elems, remoteScores, lastAdded); 
  }
  view->presentList("High scores", elems);
}

