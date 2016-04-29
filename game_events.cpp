#include "stdafx.h"
#include "game_events.h"
#include "file_sharing.h"


GameEvents::GameEvents(FileSharing& f) : fileSharing(f) {
}

void GameEvents::uploadEvent(const map<string, string>& data) {
  thread t([=] {
      fileSharing.uploadGameEvent(data);
      });
  t.detach();
}

void GameEvents::addRetiredLoaded(const string& retiredId, const string& playerName) {
  uploadEvent({
    {"eventType", "retiredLoaded"},
    {"retiredId", retiredId},
    {"playerName", playerName},
  });
}

void GameEvents::addRetiredConquered(const string& retiredId, const string& playerName) {
  uploadEvent({
    {"eventType", "retiredConquered"},
    {"retiredId", retiredId},
    {"playerName", playerName},
  });
}

