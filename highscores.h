#pragma once

#include "util.h"
#include "file_path.h"

class View;
class FileSharing;
class Options;

class Highscores {
  public:
  Highscores(const FilePath& localPath, FileSharing&, Options*);

  struct Score {
    static optional<Score> parse(const string& buf);
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
    bool isPublic() const;
  };

  void present(View*, optional<Score> lastAdded = none) const;
  void add(Score);
  vector<Score> downloadHighscores(View*) const;

  static vector<Score> fromFile(const FilePath&);

  private:
  static vector<Score> fromStream(istream&);
  static vector<Score> fromSync(FileSharing&);
  static vector<Score> fromString(const string&);
  static void saveToFile(const vector<Score>&, const FilePath&);

  FilePath localPath;
  FileSharing& fileSharing;
  vector<Score> localScores;
  Options* options;
};

