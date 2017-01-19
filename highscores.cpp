#include "stdafx.h"
#include "highscores.h"
#include "view.h"
#include "file_sharing.h"
#include "options.h"
#include "campaign_type.h"
#include "player_role.h"

Highscores::Highscores(const string& local, FileSharing& sharing, Options* o)
    : localPath(local), fileSharing(sharing), options(o) {
  localScores = fromFile(localPath);
  remoteScores = fromString(fileSharing.downloadHighscores());
  fileSharing.uploadHighscores(localPath);
}

void Highscores::add(Score s) {
  localScores.push_back(s);
  sortScores(localScores);
  saveToFile(localScores, localPath);
  remoteScores.push_back(s);
  sortScores(remoteScores);
  fileSharing.uploadHighscores(localPath);
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
    EnumInfo<CampaignType>::getString(campaignType) + delim +
    EnumInfo<PlayerRole>::getString(playerRole);
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
  if (p.size() == 9)
    try {
      return CONSTRUCT(Score,
          c.gameId = p[0];
          c.playerName = p[1];
          c.worldName = p[2];
          c.gameResult = p[3];
          c.gameWon = ::fromString<int>(p[4]);
          c.points = ::fromString<int>(p[5]);
          c.turns = ::fromString<int>(p[6]);
          c.campaignType = EnumInfo<CampaignType>::fromString(p[7]);
          c.playerRole = EnumInfo<PlayerRole>::fromString(p[8]);
      );
    } catch (ParsingException&) {}
#ifndef RELEASE
    return CONSTRUCT(Score, c.playerName = "ERROR: " + buf;);
#else
    return none;
#endif
}

typedef Highscores::Score Score;
static HighscoreList fillScores(const string& name, optional<Score> lastElem, vector<Score> scores) {
  HighscoreList list { name, {} };
  sort(scores.begin(), scores.end(), [] (const Score& a, const Score& b) -> bool {
      if (a.gameWon) {
        if (!b.gameWon)
          return true;
        else
          return a.turns < b.turns;
      } else
      if (b.gameWon)
        return false;
      else
        return a.points > b.points || (a.points == b.points && a.gameId < b.gameId);
  });
  for (auto& score : scores) {
    bool highlight = lastElem == score;
    string points = score.gameWon ? toString(score.turns) + " turns" : toString(score.points) + " points";
    if (score.gameResult.empty())
      list.scores.push_back({score.playerName + " of " + score.worldName, points, highlight});
    else
      list.scores.push_back({score.playerName + " of " + score.worldName +
          ", " + score.gameResult, points, highlight});
  }
  return list;
}

void Highscores::present(View* view, optional<Score> lastAdded) const {
  vector<HighscoreList> lists;
  lists.push_back(fillScores("Keepers", lastAdded, filter(localScores,
      [] (const Score& s) { return s.campaignType == CampaignType::CAMPAIGN && s.playerRole == PlayerRole::KEEPER;})));
  lists.push_back(fillScores("Adventurers", lastAdded, filter(localScores,
      [] (const Score& s) { return s.campaignType == CampaignType::CAMPAIGN && s.playerRole == PlayerRole::ADVENTURER;})));
  lists.push_back(fillScores("Single map", lastAdded, filter(localScores,
      [] (const Score& s) { return s.campaignType == CampaignType::SINGLE_KEEPER && s.playerRole == PlayerRole::KEEPER;})));
  lists.push_back(fillScores("Keepers", lastAdded, filter(remoteScores,
      [] (const Score& s) { return s.campaignType == CampaignType::CAMPAIGN && s.playerRole == PlayerRole::KEEPER;})));
  lists.push_back(fillScores("Adventurers", lastAdded, filter(remoteScores,
      [] (const Score& s) { return s.campaignType == CampaignType::CAMPAIGN && s.playerRole == PlayerRole::ADVENTURER;})));
  lists.push_back(fillScores("Single map", lastAdded, filter(remoteScores,
      [] (const Score& s) { return s.campaignType == CampaignType::SINGLE_KEEPER && s.playerRole == PlayerRole::KEEPER;})));
  view->presentHighscores(lists);
}

