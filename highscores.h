#ifndef _HIGHSCORES_H
#define _HIGHSCORES_H

#include "util.h"

class View;
class FileSharing;

class Highscores {
  public:
  Highscores(const string& localPath, FileSharing&);

  struct Score {
    static Score parse(const string& buf);
    bool operator == (const Score&) const;
    string toString() const;
    string gameId;
    string playerName;
    string worldName;
    string gameResult;
    int points;
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
};

#endif
