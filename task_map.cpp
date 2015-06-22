#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"

template <class Archive>
void TaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tasks)
    & SVAR(positionMap)
    & SVAR(reversePositions)
    & SVAR(creatureMap)
    & SVAR(marked)
    & SVAR(completionCost)
    & SVAR(priorityTasks)
    & SVAR(delayedTasks)
    & SVAR(highlight);
}

SERIALIZABLE(TaskMap);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

TaskMap::TaskMap(Rectangle bounds) : reversePositions(bounds), marked(bounds, nullptr), highlight(bounds) {
}

Task* TaskMap::getTaskForWorker(Creature* c) {
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
vector<const Task*> TaskMap::getAllTasks() const {
  return transform2<const Task*>(tasks, [] (const PTask& t) { return t.get(); });
}

void TaskMap::freeTaskDelay(Task* t, double d) {
  freeTask(t);
  delayedTasks[t->getUniqueId()] = d;
}

void TaskMap::setPriorityTasks(Vec2 pos) {
  for (Task* t : getTasks(pos))
    priorityTasks.insert(t);
}

Task* TaskMap::addTaskCost(PTask task, Vec2 position, CostInfo cost) {
  completionCost[task.get()] = cost;
  return addTask(std::move(task), position);
}

CostInfo TaskMap::removeTask(Task* task) {
  task->cancel();
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

CostInfo TaskMap::removeTask(UniqueEntity<Task>::Id id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      return removeTask(task.get());
    }
  return CostInfo();
}

bool TaskMap::isPriorityTask(const Task* t) const {
  return priorityTasks.contains(t);
}

bool TaskMap::hasPriorityTasks(Vec2 pos) const {
  for (Task* task : reversePositions[pos])
    if (isPriorityTask(task))
      return true;
  return false;
}

Task* TaskMap::getMarked(Vec2 pos) const {
  return marked[pos];
}

void TaskMap::markSquare(Vec2 pos, HighlightType h, PTask task) {
  marked[pos] = task.get();
  highlight[pos] = h;
  addTask(std::move(task), pos);
}

HighlightType TaskMap::getHighlightType(Vec2 pos) const {
  return highlight[pos];
}

void TaskMap::unmarkSquare(Vec2 pos) {
  Task* task = marked[pos];
  marked[pos] = nullptr;
  removeTask(task);
}

bool TaskMap::hasTask(const Creature* c) const {
  if (creatureMap.contains(c))
    return !creatureMap.get(c)->isDone();
  else
    return nullptr;
}

Task* TaskMap::getTask(const Creature* c) {
  if (creatureMap.contains(c)) {
    Task* task = creatureMap.get(c);
    if (task->isDone())
      removeTask(task);
    else
      return task;
  }
  return nullptr;
}

const vector<Task*>& TaskMap::getTasks(Vec2 pos) const {
  return reversePositions[pos];
}

Task* TaskMap::addTask(PTask task, const Creature* c) {
  creatureMap.insert(c, task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

Task* TaskMap::addPriorityTask(PTask task, const Creature* c) {
  Task* t = addTask(std::move(task), c);
  priorityTasks.insert(t);
  return t;
}

Task* TaskMap::addTask(PTask task, Vec2 position) {
  positionMap[task.get()] = position;
  reversePositions[position].push_back(task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void TaskMap::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  creatureMap.insert(c, task);
}

optional<Vec2> TaskMap::getPosition(Task* task) const {
  if (positionMap.count(task))
    return positionMap.at(task);
  else
    return none;
}

const Creature* TaskMap::getOwner(const Task* task) const {
  if (creatureMap.contains(const_cast<Task*>(task)))
    return creatureMap.get(const_cast<Task*>(task));
  else
    return nullptr;
}

void TaskMap::freeTask(Task* task) {
  if (creatureMap.contains(task))
    creatureMap.erase(task);
}

void TaskMap::freeFromTask(Creature* c) {
  if (Task* t = getTask(c))
    freeTask(t);
}

const map<Task*, CostInfo>& TaskMap::getCompletionCosts() const {
  return completionCost;
}

