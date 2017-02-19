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

    SERIALIZE_ALL(gameId, playerName, worldName, gameResult, gameWon, points, turns, campaignType, playerRole, version)

    string SERIAL(gameId);
    string SERIAL(playerName);
    string SERIAL(worldName);
    string SERIAL(gameResult);
    bool SERIAL(gameWon);
    int SERIAL(points);
    int SERIAL(turns);
    CampaignType SERIAL(campaignType);
    PlayerRole SERIAL(playerRole);
    int SERIAL(version);
    bool isPublic() const;
  };

  void present(View*, optional<Score> lastAdded = none) const;
  void add(Score);

  static vector<Score> fromFile(const string& path);

  private:
  static vector<Score> fromStream(istream&);
  static vector<Score> fromSync(FileSharing&);
  static vector<Score> fromString(const string&);
  static void saveToFile(const vector<Score>&, const string& path);

  string localPath;
  FileSharing& fileSharing;
  vector<Score> localScores;
  vector<Score> remoteScores;
  Options* options;
};

