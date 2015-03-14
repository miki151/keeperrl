#include "stdafx.h"
#include "task_map.h"
#include "collective.h"
#include "creature.h"

template <class CostInfo>
template <class Archive>
void TaskMap<CostInfo>::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tasks)
    & SVAR(positionMap)
    & SVAR(reversePositions)
    & SVAR(creatureMap)
    & SVAR(marked);
  if (version == 0) {
    set<pair<const Creature*, UniqueEntity<Creature>::Id>> lockedTasks;
    ar & boost::serialization::make_nvp("lockedTasks", lockedTasks);
  }
  ar & SVAR(completionCost)
    & SVAR(priorityTasks)
    & SVAR(delayedTasks)
    & SVAR(highlight);
  CHECK_SERIAL;
}

SERIALIZABLE(TaskMap<Collective::CostInfo>);

template<class CostInfo>
SERIALIZATION_CONSTRUCTOR_IMPL2(TaskMap<CostInfo>, TaskMap);

template <class CostInfo>
TaskMap<CostInfo>::TaskMap(Rectangle bounds) : reversePositions(bounds), marked(bounds, nullptr), highlight(bounds) {
}

template <class CostInfo>
Task* TaskMap<CostInfo>::getTaskForWorker(Creature* c) {
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    if (auto pos = getPosition(task.get())) {
      double dist = (*pos - c->getPosition()).length8();
      const Creature* owner = getOwner(task.get());
      if (!task->isDone() && (!owner || (task->canTransfer() && (*pos - owner->getPosition()).length8() > dist))
          && (!closest || dist < (*getPosition(closest) - c->getPosition()).length8()
              || isPriorityTask(task.get()))
          && (!delayedTasks.count(task->getUniqueId()) || delayedTasks.at(task->getUniqueId()) < c->getTime())) {
        bool valid = task->getMove(c);
        if (valid) {
          closest = task.get();
          if (isPriorityTask(task.get()))
            return task.get();
        }
      }
    }
  }
  return closest;
}
template <class CostInfo>
vector<const Task*> TaskMap<CostInfo>::getAllTasks() const {
  return transform2<const Task*>(tasks, [] (const PTask& t) { return t.get(); });
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
    marked[*pos] = nullptr;
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (creatureMap.contains(task))
    creatureMap.erase(task);
  if (positionMap.count(task)) {
    removeElement(reversePositions[positionMap.at(task)], task);
    positionMap.erase(task);
  }
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
bool TaskMap<CostInfo>::isPriorityTask(const Task* t) const {
  return priorityTasks.contains(t);
}

template <class CostInfo>
bool TaskMap<CostInfo>::hasPriorityTasks(Vec2 pos) const {
  for (Task* task : reversePositions[pos])
    if (isPriorityTask(task))
      return true;
  return false;
}

template <class CostInfo>
Task* TaskMap<CostInfo>::getMarked(Vec2 pos) const {
  return marked[pos];
}

template <class CostInfo>
void TaskMap<CostInfo>::markSquare(Vec2 pos, HighlightType h, PTask task) {
  marked[pos] = task.get();
  highlight[pos] = h;
  addTask(std::move(task), pos);
}

template <class CostInfo>
HighlightType TaskMap<CostInfo>::getHighlightType(Vec2 pos) const {
  return highlight[pos];
}

template <class CostInfo>
void TaskMap<CostInfo>::unmarkSquare(Vec2 pos) {
  Task* task = marked[pos];
  marked[pos] = nullptr;
  removeTask(task);
}

template <class CostInfo>
bool TaskMap<CostInfo>::hasTask(const Creature* c) const {
  if (creatureMap.contains(c))
    return !creatureMap.get(c)->isDone();
  else
    return nullptr;
}

template <class CostInfo>
Task* TaskMap<CostInfo>::getTask(const Creature* c) {
  if (creatureMap.contains(c)) {
    Task* task = creatureMap.get(c);
    if (task->isDone())
      removeTask(task);
    else
      return task;
  }
  return nullptr;
}

template <class CostInfo>
const vector<Task*>& TaskMap<CostInfo>::getTasks(Vec2 pos) const {
  return reversePositions[pos];
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addTask(PTask task, const Creature* c) {
  creatureMap.insert(c, task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addPriorityTask(PTask task, const Creature* c) {
  Task* t = addTask(std::move(task), c);
  priorityTasks.insert(t);
  return t;
}

template <class CostInfo>
Task* TaskMap<CostInfo>::addTask(PTask task, Vec2 position) {
  positionMap[task.get()] = position;
  reversePositions[position].push_back(task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

template <class CostInfo>
void TaskMap<CostInfo>::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  creatureMap.insert(c, task);
}

template <class CostInfo>
optional<Vec2> TaskMap<CostInfo>::getPosition(Task* task) const {
  if (positionMap.count(task))
    return positionMap.at(task);
  else
    return none;
}

template <class CostInfo>
const Creature* TaskMap<CostInfo>::getOwner(const Task* task) const {
  if (creatureMap.contains(const_cast<Task*>(task)))
    return creatureMap.get(const_cast<Task*>(task));
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

namespace boost { 
namespace serialization {
template<typename CostInfo>
const unsigned int version<TaskMap<CostInfo>>::value;
template struct version<TaskMap<Collective::CostInfo>>;

}
}

