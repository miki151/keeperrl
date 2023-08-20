#pragma once

#include "util.h"
#include "entity_set.h"
#include "entity_map.h"
#include "cost_info.h"
#include "position_map.h"
#include "minion_trait.h"
#include "game_time.h"
#include "minion_activity.h"
#include "indexed_vector.h"

class Task;
class Creature;

class TaskMap {
  public:
  WTask addTaskFor(PTask, Creature*);
  WTask addTask(PTask, Position, MinionActivity);
  WTask getTask(const Creature*);
  bool hasTask(const Creature*) const;
  const vector<WTask>& getTasks(Position) const;
  bool hasTask(Position, MinionActivity) const;
  vector<WTask> getTasks(MinionActivity) const;
  vector<WConstTask> getAllTasks() const;
  Creature* getOwner(WConstTask) const;
  optional<Position> getPosition(const Task*) const;
  CostInfo takeTask(Creature*, WTask);
  void freeTask(WTask);

  WTask addTaskCost(PTask, Position, CostInfo, MinionActivity);
  void markSquare(Position, HighlightType, PTask, MinionActivity);
  WTask getMarked(Position) const;
  optional<HighlightType> getHighlightType(Position) const;
  CostInfo removeTask(WTask);
  CostInfo removeTask(UniqueEntity<Task>::Id);
  CostInfo freeFromTask(const Creature*);
  bool isPriorityTask(WConstTask) const;
  bool hasPriorityTasks(Position) const;
  void setPriorityTasks(Position);
  WTask getClosestTask(const Creature*, MinionActivity, bool priorityOnly, const Collective*) const;
  const EntityMap<Task, CostInfo>& getCompletionCosts() const;
  WTask getTask(UniqueEntity<Task>::Id) const;
  void tick();
  optional<MinionActivity> getTaskActivity(Task*) const;

  SERIALIZATION_DECL(TaskMap)

  private:
  EntityMap<Creature, WTask> SERIAL(taskByCreature);
  EntityMap<Task, Creature*> SERIAL(creatureByTask);
  EntityMap<Task, Position> SERIAL(positionMap);
  unordered_map<Position, vector<WTask>, CustomHash<Position>> SERIAL(reversePositions);
  vector<PTask> SERIAL(tasks);
  EntityMap<Task, WTask> SERIAL(taskById);
  unordered_map<Position, WTask, CustomHash<Position>> SERIAL(marked);
  unordered_map<Position, HighlightType, CustomHash<Position>> SERIAL(highlight);
  EntityMap<Task, CostInfo> SERIAL(completionCost);
  EntityMap<Task, LocalTime> SERIAL(delayedTasks);
  EntitySet<Task> SERIAL(priorityTasks);
  EnumMap<MinionActivity, vector<Task*>> SERIAL(taskByActivity);
  EnumMap<MinionActivity, IndexedVector<Task*, UniqueEntity<Task>::Id>> priorityTaskByActivity;
  EnumMap<MinionActivity, vector<Task*>> cantPerformByAnyone;
  EntityMap<Task, MinionActivity> SERIAL(activityByTask);
  void releaseOnHoldTask(Task*);
  void setPosition(WTask, Position);
  void addToTaskByActivity(Task*, MinionActivity);
};

