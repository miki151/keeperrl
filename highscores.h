#pragma once

#include "util.h"
#include "file_path.h"

class View;
class FileSharing;
class Options;

class Highscores {
  public:
  Highscores(const FilePath& localPath);

  struct Score {
    bool operator == (const Score&) const;

    SERIALIZE_ALL(gameId, playerName, worldName, gameResult, gameWon, points, turns, campaignType, version)

    string SERIAL(gameId);
    string SERIAL(playerName);
    string SERIAL(worldName);
    string SERIAL(gameResult);
    bool SERIAL(gameWon);
    int SERIAL(points);
    int SERIAL(turns);
    CampaignType SERIAL(campaignType);
    int SERIAL(version);
  };

  void present(View*, optional<Score> lastAdded = none) const;
  void add(Score);

  static vector<Score> fromFile(const FilePath&);

  private:
  static void saveToFile(const vector<Score>&, const FilePath&);

  FilePath localPath;
  vector<Score> localScores;
};

