#ifndef _QUEST_H
#define _QUEST_H

#include <map>
#include <string>

#include "util.h"

enum class QuestName { GOLD };

class Quest {
  public:
  Quest();
  void open();
  void finish();
  bool isOpen() const;
  bool isFinished() const;
  bool isClosed() const;
  static void initialize();
  static Quest& get(QuestName);

  private:
  enum class State { CLOSED, OPEN, FINISHED } state;
  static map<QuestName, Quest> quests;
};

#endif
