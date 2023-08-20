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
  Task* addTaskFor(PTask, Creature*);
  Task* addTask(PTask, Position, MinionActivity);
  Task* getTask(const Creature*);
  bool hasTask(const Creature*) const;
  const vector<Task*>& getTasks(Position) const;
  bool hasTask(Position, MinionActivity) const;
  vector<Task*> getTasks(MinionActivity) const;
  vector<const Task*> getAllTasks() const;
  Creature* getOwner(const Task*) const;
  optional<Position> getPosition(const Task*) const;
  CostInfo takeTask(Creature*, Task*);
  void freeTask(Task*);

  Task* addTaskCost(PTask, Position, CostInfo, MinionActivity);
  void markSquare(Position, HighlightType, PTask, MinionActivity);
  Task* getMarked(Position) const;
  optional<HighlightType> getHighlightType(Position) const;
  CostInfo removeTask(Task*);
  CostInfo removeTask(UniqueEntity<Task>::Id);
  CostInfo freeFromTask(const Creature*);
  bool isPriorityTask(const Task*) const;
  bool hasPriorityTasks(Position) const;
  void setPriorityTasks(Position);
  Task* getClosestTask(const Creature*, MinionActivity, bool priorityOnly, const Collective*) const;
  const EntityMap<Task, CostInfo>& getCompletionCosts() const;
  Task* getTask(UniqueEntity<Task>::Id) const;
  void tick();
  optional<MinionActivity> getTaskActivity(Task*) const;

  SERIALIZATION_DECL(TaskMap)

  private:
  EntityMap<Creature, Task*> SERIAL(taskByCreature);
  EntityMap<Task, Creature*> SERIAL(creatureByTask);
  EntityMap<Task, Position> SERIAL(positionMap);
  unordered_map<Position, vector<Task*>, CustomHash<Position>> SERIAL(reversePositions);
  vector<PTask> SERIAL(tasks);
  EntityMap<Task, Task*> SERIAL(taskById);
  unordered_map<Position, Task*, CustomHash<Position>> SERIAL(marked);
  unordered_map<Position, HighlightType, CustomHash<Position>> SERIAL(highlight);
  EntityMap<Task, CostInfo> SERIAL(completionCost);
  EntityMap<Task, LocalTime> SERIAL(delayedTasks);
  EntitySet<Task> SERIAL(priorityTasks);
  EnumMap<MinionActivity, vector<Task*>> SERIAL(taskByActivity);
  EnumMap<MinionActivity, IndexedVector<Task*, UniqueEntity<Task>::Id>> priorityTaskByActivity;
  EnumMap<MinionActivity, vector<Task*>> cantPerformByAnyone;
  EntityMap<Task, MinionActivity> SERIAL(activityByTask);
  void releaseOnHoldTask(Task*);
  void setPosition(Task*, Position);
  void addToTaskByActivity(Task*, MinionActivity);
};

