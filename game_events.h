#ifndef _GAME_EVENT_H
#define _GAME_EVENT_H

#include "util.h"

class FileSharing;

class GameEvents {
  public:
  GameEvents(FileSharing&);
  void addRetiredLoaded(const string& retiredId, const string& playerName);
  void addRetiredConquered(const string& retiredId, const string& playerName);

  private:
  void uploadEvent(const map<string, string>&);
  FileSharing& fileSharing;
};

#endif
