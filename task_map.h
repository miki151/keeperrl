#ifndef _TASK_MAP_H
#define _TASK_MAP_H

#include "util.h"
#include "entity_set.h"
#include "entity_map.h"
#include "cost_info.h"
#include "position_map.h"
#include "minion_task.h"

class Task;
class Creature;

class TaskMap {
  public:
  Task* addTask(PTask, const Creature*);
  Task* addPriorityTask(PTask, const Creature*);
  Task* addTask(PTask, Position, MinionTrait required = MinionTrait::WORKER);
  Task* getTask(const Creature*);
  bool hasTask(const Creature*) const;
  const vector<Task*>& getTasks(Position) const;
  vector<const Task*> getAllTasks() const;
  const Creature* getOwner(const Task*) const;
  optional<Position> getPosition(Task*) const;
  void takeTask(const Creature*, Task*);
  void freeTask(Task*);
  void freeFromTask(Creature*);

  Task* addTaskCost(PTask, Position, CostInfo);
  void markSquare(Position, HighlightType, PTask);
  void unmarkSquare(Position);
  Task* getMarked(Position) const;
  HighlightType getHighlightType(Position) const;
  CostInfo removeTask(Task*);
  CostInfo removeTask(UniqueEntity<Task>::Id);
  bool isPriorityTask(const Task*) const;
  bool hasPriorityTasks(Position) const;
  void freeTaskDelay(Task*, double delayTime);
  void setPriorityTasks(Position);
  Task* getClosestTask(Creature* c, MinionTrait);
  const map<Task*, CostInfo>& getCompletionCosts() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  BiMap<const Creature*, Task*> SERIAL(creatureMap);
  unordered_map<Task*, Position> SERIAL(positionMap);
  PositionMap<vector<Task*>> SERIAL(reversePositions);
  vector<PTask> SERIAL(tasks);
  PositionMap<Task*> SERIAL(marked);
  PositionMap<HighlightType> SERIAL(highlight);
  map<Task*, CostInfo> SERIAL(completionCost);
  EntityMap<Task, double> SERIAL(delayedTasks);
  EntitySet<Task> SERIAL(priorityTasks);
  unordered_map<Task*, MinionTrait> SERIAL(requiredTraits);
};

#endif
