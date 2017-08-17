#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"
#include "creature_name.h"

SERIALIZE_DEF(TaskMap, tasks, positionMap, reversePositions, taskByCreature, creatureByTask, marked, completionCost, priorityTasks, delayedTasks, highlight, taskById);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

WTask TaskMap::getClosestTask(WCreature c) {
  if (Random.roll(20))
    for (WTask t : getWeakPointers(tasks))
      if (t->isDone())
        removeTask(t);
  WTask closest = nullptr;
  for (PTask& task : tasks)
    if (task->canPerform(c))
      if (auto pos = getPosition(task.get())) {
        double dist = pos->dist8(c->getPosition());
        WConstCreature owner = getOwner(task.get());
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

vector<WConstTask> TaskMap::getAllTasks() const {
  return tasks.transform([] (const PTask& t) -> WConstTask { return t.get(); });
}

void TaskMap::setPriorityTasks(Position pos) {
  for (WTask t : getTasks(pos))
    priorityTasks.insert(t);
  pos.setNeedsRenderUpdate(true);
}

WTask TaskMap::addTaskCost(PTask task, Position position, CostInfo cost) {
  completionCost.set(task.get(), cost);
  return addTask(std::move(task), position);
}

CostInfo TaskMap::removeTask(WTask task) {
  if (!task->isDone())
    task->cancel();
  CostInfo cost;
  if (auto c = completionCost.getMaybe(task)) {
    cost = *c;
    completionCost.erase(task);
  }
  if (auto pos = getPosition(task)) {
    marked.set(*pos, nullptr);
    pos->setNeedsRenderUpdate(true);
  }
  if (auto c = creatureByTask.getMaybe(task)) {
    CHECK(taskByCreature.getMaybe(*c));
     taskByCreature.erase(*c);
    creatureByTask.erase(task);
  }
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  if (auto pos = positionMap.getMaybe(task)) {
    reversePositions.getOrFail(*pos).removeElement(task);
    positionMap.erase(task);
  }
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      taskById.erase(task);
      tasks.removeIndex(i);
      break;
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

bool TaskMap::isPriorityTask(WConstTask t) const {
  return priorityTasks.contains(t);
}

bool TaskMap::hasPriorityTasks(Position pos) const {
  for (WTask task : reversePositions.get(pos))
    if (isPriorityTask(task))
      return true;
  return false;
}

WTask TaskMap::getMarked(Position pos) const {
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

bool TaskMap::hasTask(WConstCreature c) const {
  if (auto task = taskByCreature.getMaybe(c))
    return !(*task)->isDone();
  else
    return false;
}

WTask TaskMap::getTask(WConstCreature c) {
  if (auto task = taskByCreature.getMaybe(c)) {
    if ((*task)->isDone()) {
      removeTask(*task);
      CHECK(!taskByCreature.getMaybe(c));
    } else
      return *task;
  }
  return nullptr;
}

const vector<WTask>& TaskMap::getTasks(Position pos) const {
  return reversePositions.get(pos);
}

WTask TaskMap::addTaskFor(PTask task, WCreature c) {
  CHECK(!hasTask(c)) << c->getName().bare() << " already has a task";
  CHECK(!taskByCreature.getMaybe(c));
  CHECK(!creatureByTask.getMaybe(task.get()));
  taskByCreature.set(c, task.get());
  creatureByTask.set(task.get(), c);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  if (auto pos = task->getPosition())
    setPosition(task.get(), *pos);
  taskById.set(task->getUniqueId(), task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

WTask TaskMap::addTask(PTask task, Position position) {
  setPosition(task.get(), position);
  taskById.set(task->getUniqueId(), task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void TaskMap::takeTask(WCreature c, WTask task) {
  freeTask(task);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  CHECK(!taskByCreature.getMaybe(c));
  CHECK(!creatureByTask.getMaybe(task));
  taskByCreature.set(c, task);
  creatureByTask.set(task, c);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
}

optional<Position> TaskMap::getPosition(WTask task) const {
  return positionMap.getMaybe(task);
}

WCreature TaskMap::getOwner(WConstTask task) const {
  if (auto c = creatureByTask.getMaybe(task))
    return *c;
  else
    return nullptr;
}

void TaskMap::freeTask(WTask task) {
  if (auto c = creatureByTask.getMaybe(task)) {
    CHECK(taskByCreature.getMaybe(*c));
    taskByCreature.erase(*c);
    creatureByTask.erase(task);
    CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  }
}

void TaskMap::setPosition(WTask task, Position position) {
  positionMap.set(task, position);
  reversePositions.getOrInit(position).push_back(task);
}

CostInfo TaskMap::freeFromTask(WConstCreature c) {
  if (WTask task = getTask(c)) {
    if (!task->canTransfer())
      return removeTask(task);
    else {
      freeTask(task);
      delayedTasks.set(task, c->getLocalTime() + 50);
    }
  }
  return CostInfo::noCost();
}

const EntityMap<Task, CostInfo>& TaskMap::getCompletionCosts() const {
  return completionCost;
}

WTask TaskMap::getTask(UniqueEntity<Task>::Id id) const {
  return taskById.getOrFail(id);
}

