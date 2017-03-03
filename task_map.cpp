#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"
#include "creature_name.h"

SERIALIZE_DEF(TaskMap, tasks, positionMap, reversePositions, creatureMap, marked, completionCost, priorityTasks, delayedTasks, highlight, requiredTraits);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

Task* TaskMap::getClosestTask(Creature* c, MinionTrait trait) {
  if (Random.roll(20))
    for (Task* t : extractRefs(tasks))
      if (t->isDone())
        removeTask(t);
  Task* closest = nullptr;
  for (PTask& task : tasks)
    if (requiredTraits.count(task.get()) && requiredTraits.at(task.get()) == trait && task->canPerform(c))
      if (auto pos = getPosition(task.get())) {
        double dist = pos->dist8(c->getPosition());
        const Creature* owner = getOwner(task.get());
        auto delayed = delayedTasks.getMaybe(task.get());
        if (!task->isDone() &&
            (!owner || (task->canTransfer() && pos->dist8(owner->getPosition()) > dist && dist <= 6)) &&
            (!closest || dist < getPosition(closest)->dist8(c->getPosition()) || isPriorityTask(task.get())) &&
            c->canNavigateTo(*pos) && !task->isBlocked(c) &&
            (!delayed || *delayed < c->getLocalTime())) {
          closest = task.get();
          if (isPriorityTask(task.get()))
            return task.get();
        }
      }
  return closest;
}

vector<const Task*> TaskMap::getAllTasks() const {
  return transform2(tasks, [] (const PTask& t) -> const Task* { return t.get(); });
}

void TaskMap::setPriorityTasks(Position pos) {
  for (Task* t : getTasks(pos))
    priorityTasks.insert(t);
  pos.setNeedsRenderUpdate(true);
}

Task* TaskMap::addTaskCost(PTask task, Position position, CostInfo cost) {
  completionCost[task.get()] = cost;
  return addTask(std::move(task), position, MinionTrait::WORKER);
}

CostInfo TaskMap::removeTask(Task* task) {
  if (!task->isDone())
    task->cancel();
  CostInfo cost;
  if (completionCost.count(task)) {
    cost = completionCost.at(task);
    completionCost.erase(task);
  }
  if (auto pos = getPosition(task)) {
    marked.set(*pos, nullptr);
    pos->setNeedsRenderUpdate(true);
  }
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (creatureMap.contains(task))
    creatureMap.erase(task);
  if (positionMap.count(task)) {
    removeElement(reversePositions.getOrFail(positionMap.at(task)), task);
    positionMap.erase(task);
  }
  if (requiredTraits.count(task))
    requiredTraits.erase(task);
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

bool TaskMap::hasPriorityTasks(Position pos) const {
  for (Task* task : reversePositions.get(pos))
    if (isPriorityTask(task))
      return true;
  return false;
}

Task* TaskMap::getMarked(Position pos) const {
  return marked.get(pos);
}

void TaskMap::markSquare(Position pos, HighlightType h, PTask task) {
  marked.set(pos, task.get());
  pos.setNeedsRenderUpdate(true);
  highlight.set(pos, h);
  addTask(std::move(task), pos);
}

HighlightType TaskMap::getHighlightType(Position pos) const {
  return highlight.get(pos);
}

bool TaskMap::hasTask(const Creature* c) const {
  if (creatureMap.contains(c))
    return !creatureMap.get(c)->isDone();
  else
    return false;
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

const vector<Task*>& TaskMap::getTasks(Position pos) const {
  return reversePositions.get(pos);
}

Task* TaskMap::addTaskFor(PTask task, const Creature* c) {
  CHECK(!hasTask(c)) << c->getName().bare() << " already has a task";
  creatureMap.insert(c, task.get());
  if (auto pos = task->getPosition())
    setPosition(task.get(), *pos);
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

Task* TaskMap::addTask(PTask task, Position position, MinionTrait required) {
  setPosition(task.get(), position);
  requiredTraits[task.get()] = required;
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void TaskMap::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  creatureMap.insert(c, task);
}

optional<Position> TaskMap::getPosition(Task* task) const {
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

void TaskMap::setPosition(Task* task, Position position) {
  positionMap[task] = position;
  reversePositions.getOrInit(position).push_back(task);
}

CostInfo TaskMap::freeFromTask(const Creature* c) {
  if (Task* task = getTask(c)) {
    if (!task->canTransfer())
      return removeTask(task);
    else {
      freeTask(task);
      delayedTasks.set(task, c->getLocalTime() + 50);
    }
  }
  return CostInfo::noCost();
}

const map<Task*, CostInfo>& TaskMap::getCompletionCosts() const {
  return completionCost;
}

