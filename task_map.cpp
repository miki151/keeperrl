#include "stdafx.h"
#include "task_map.h"
#include "collective.h"
#include "creature.h"

template <class CostInfo>
template <class Archive>
void TaskMap<CostInfo>::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tasks)
    & SVAR(positionMap)
    & SVAR(creatureMap)
    & SVAR(marked)
    & SVAR(lockedTasks)
    & SVAR(completionCost)
    & SVAR(priorityTasks)
    & SVAR(delayedTasks);
  CHECK_SERIAL;
}

SERIALIZABLE(TaskMap<Collective::CostInfo>);


template <class CostInfo>
Task* TaskMap<CostInfo>::getTaskForWorker(Creature* c) {
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    if (auto pos = getPosition(task.get())) {
      double dist = (*pos - c->getPosition()).length8();
      const Creature* owner = getOwner(task.get());
      if ((!owner || (task->canTransfer() && (*pos - owner->getPosition()).length8() > dist))
          && (!closest || dist < (*getPosition(closest) - c->getPosition()).length8()
              || priorityTasks.contains(task.get()))
          && !isLocked(c, task.get())
          && (!delayedTasks.count(task->getUniqueId()) || delayedTasks.at(task->getUniqueId()) < c->getTime())) {
        bool valid = task->getMove(c);
        if (valid)
          closest = task.get();
        else
          lock(c, task.get());
      }
    }
  }
  return closest;
}

template <class CostInfo>
void TaskMap<CostInfo>::freeTaskDelay(Task* t, double d) {
  freeTask(t);
  delayedTasks[t->getUniqueId()] = d;
}

template <class CostInfo>
void TaskMap<CostInfo>::setPriorityTasks(Vec2 pos) {
  for (Task* t : getTasks(pos))
    priorityTasks.insert(t);
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addTaskCost(PTask task, Vec2 position, CostInfo cost) {
  completionCost[task.get()] = cost;
  return addTask(std::move(task), position);
}

template <class CostInfo>
CostInfo TaskMap<CostInfo>::removeTask(Task* task) {
  CostInfo cost;
  if (completionCost.count(task)) {
    cost = completionCost.at(task);
    completionCost.erase(task);
  }
  if (auto pos = getPosition(task))
    if (marked.count(*pos))
      marked.erase(*pos);
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (creatureMap.contains(task))
    creatureMap.erase(task);
  if (positionMap.count(task))
    positionMap.erase(task);
  return cost;
}

template <class CostInfo>
CostInfo TaskMap<CostInfo>::removeTask(UniqueEntity<Task>::Id id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      return removeTask(task.get());
    }
  return CostInfo();
}

template <class CostInfo>
bool TaskMap<CostInfo>::isLocked(const Creature* c, const Task* t) const {
  return lockedTasks.count({c, t->getUniqueId()});
}

template <class CostInfo>
void TaskMap<CostInfo>::lock(const Creature* c, const Task* t) {
  lockedTasks.insert({c, t->getUniqueId()});
}

template <class CostInfo>
void TaskMap<CostInfo>::clearAllLocked() {
  lockedTasks.clear();
}

template <class CostInfo>
Task* TaskMap<CostInfo>::getMarked(Vec2 pos) const {
  if (marked.count(pos))
    return marked.at(pos);
  else
    return nullptr;
}

template <class CostInfo>
void TaskMap<CostInfo>::markSquare(Vec2 pos, PTask task) {
  marked[pos] = task.get();
  addTask(std::move(task), pos);
}

template <class CostInfo>
void TaskMap<CostInfo>::unmarkSquare(Vec2 pos) {
  Task* task = marked.at(pos);
  marked.erase(pos);
  removeTask(task);
}

template <class CostInfo>
Task* TaskMap<CostInfo>::getTask(const Creature* c) const {
  if (creatureMap.contains(c))
    return creatureMap.get(c);
  else
    return nullptr;
}

template <class CostInfo>
vector<Task*> TaskMap<CostInfo>::getTasks(Vec2 pos) const {
  vector<Task*> ret;
  for (const PTask& task : tasks)
    if (getPosition(task.get()) == pos)
      ret.push_back(task.get());
  return ret;
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addTask(PTask task, const Creature* c) {
  creatureMap.insert(c, task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addTask(PTask task, Vec2 position) {
  positionMap[task.get()] = position;
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

template <class CostInfo>
void TaskMap<CostInfo>::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  creatureMap.insert(c, task);
}

template <class CostInfo>
Optional<Vec2> TaskMap<CostInfo>::getPosition(Task* task) const {
  if (positionMap.count(task))
    return positionMap.at(task);
  else
    return Nothing();
}

template <class CostInfo>
const Creature* TaskMap<CostInfo>::getOwner(Task* task) const {
  if (creatureMap.contains(task))
    return creatureMap.get(task);
  else
    return nullptr;
}

template <class CostInfo>
void TaskMap<CostInfo>::freeTask(Task* task) {
  if (creatureMap.contains(task))
    creatureMap.erase(task);
}

template <class CostInfo>
void TaskMap<CostInfo>::freeFromTask(Creature* c) {
  if (Task* t = getTask(c))
    freeTask(t);
}

template <class CostInfo>
const map<Task*, CostInfo>& TaskMap<CostInfo>::getCompletionCosts() const {
  return completionCost;
}

template class TaskMap<Collective::CostInfo>;
