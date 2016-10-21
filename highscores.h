#pragma once

#include "util.h"

class View;
class FileSharing;
class Options;

class Highscores {
  public:
  Highscores(const string& localPath, FileSharing&, Options*);

  struct Score {
    static optional<Score> parse(const string& buf);
    bool operator == (const Score&) const;
    string toString() const;
    string gameId;
    string playerName;
    string worldName;
    string gameResult;
    bool gameWon;
    int points;
    int turns;
    enum GameType { KEEPER, ADVENTURER, KEEPER_CAMPAIGN, ADVENTURER_CAMPAIGN } gameType;
  };

  void present(View*, optional<Score> lastAdded = none) const;
  void add(Score);

  private:
  static vector<Score> fromStream(istream&);
  static vector<Score> fromFile(const string& path);
  static vector<Score> fromSync(FileSharing&);
  static vector<Score> fromString(const string&);
  static void saveToFile(const vector<Score>&, const string& path);
  static void sortScores(vector<Score>&);

  string localPath;
  FileSharing& fileSharing;
  vector<Score> localScores;
  vector<Score> remoteScores;
  Options* options;
};

