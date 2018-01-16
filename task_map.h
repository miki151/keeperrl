#pragma once

#include "util.h"
#include "entity_set.h"
#include "entity_map.h"
#include "cost_info.h"
#include "position_map.h"
#include "minion_trait.h"
#include "game_time.h"
#include "minion_activity.h"

class Task;
class Creature;

class TaskMap {
  public:
  WTask addTaskFor(PTask, WCreature);
  WTask addTask(PTask, Position, MinionActivity);
  WTask getTask(WConstCreature);
  bool hasTask(WConstCreature) const;
  const vector<WTask>& getTasks(Position) const;
  vector<WConstTask> getAllTasks() const;
  WCreature getOwner(WConstTask) const;
  optional<Position> getPosition(WTask) const;
  void takeTask(WCreature, WTask);
  void freeTask(WTask);

  void setPosition(WTask, Position);
  WTask addTaskCost(PTask, Position, CostInfo, MinionActivity);
  void markSquare(Position, HighlightType, PTask, MinionActivity);
  WTask getMarked(Position) const;
  HighlightType getHighlightType(Position) const;
  CostInfo removeTask(WTask);
  CostInfo removeTask(UniqueEntity<Task>::Id);
  CostInfo freeFromTask(WConstCreature);
  bool isPriorityTask(WConstTask) const;
  bool hasPriorityTasks(Position) const;
  void setPriorityTasks(Position);
  WTask getClosestTask(WConstCreature, MinionActivity);
  const EntityMap<Task, CostInfo>& getCompletionCosts() const;
  WTask getTask(UniqueEntity<Task>::Id) const;

  SERIALIZATION_DECL(TaskMap);

  private:
  EntityMap<Creature, WTask> SERIAL(taskByCreature);
  EntityMap<Task, WCreature> SERIAL(creatureByTask);
  EntityMap<Task, Position> SERIAL(positionMap);
  PositionMap<vector<WTask>> SERIAL(reversePositions);
  vector<PTask> SERIAL(tasks);
  EntityMap<Task, WTask> SERIAL(taskById);
  PositionMap<WTask> SERIAL(marked);
  PositionMap<HighlightType> SERIAL(highlight);
  EntityMap<Task, CostInfo> SERIAL(completionCost);
  EntityMap<Task, LocalTime> SERIAL(delayedTasks);
  EntitySet<Task> SERIAL(priorityTasks);
  EnumMap<MinionActivity, vector<WTask>> SERIAL(taskByActivity);
  EntityMap<Task, MinionActivity> SERIAL(activityByTask);
};

