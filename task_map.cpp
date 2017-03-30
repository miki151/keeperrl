#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"
#include "creature_name.h"

SERIALIZE_DEF(TaskMap, tasks, positionMap, reversePositions, taskByCreature, creatureByTask, marked, completionCost, priorityTasks, delayedTasks, highlight, requiredTraits);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

Task* TaskMap::getClosestTask(WCreature c, MinionTrait trait) {
  if (Random.roll(20))
    for (Task* t : extractRefs(tasks))
      if (t->isDone())
        removeTask(t);
  Task* closest = nullptr;
  for (PTask& task : tasks)
    if (requiredTraits.count(task.get()) && requiredTraits.at(task.get()) == trait && task->canPerform(c))
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
  if (auto c = creatureByTask.getMaybe(task)) {
    CHECK(taskByCreature.getMaybe(*c));
    taskByCreature.erase(*c);
    creatureByTask.erase(task);
  }
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  if (positionMap.count(task)) {
    removeElement(reversePositions.getOrFail(positionMap.at(task)), task);
    positionMap.erase(task);
  }
  if (requiredTraits.count(task))
    requiredTraits.erase(task);
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
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

bool TaskMap::hasTask(WConstCreature c) const {
  if (auto task = taskByCreature.getMaybe(c))
    return !(*task)->isDone();
  else
    return false;
}

Task* TaskMap::getTask(WConstCreature c) {
  if (auto task = taskByCreature.getMaybe(c)) {
    if ((*task)->isDone()) {
      removeTask(*task);
      CHECK(!taskByCreature.getMaybe(c));
    } else
      return *task;
  }
  return nullptr;
}

const vector<Task*>& TaskMap::getTasks(Position pos) const {
  return reversePositions.get(pos);
}

Task* TaskMap::addTaskFor(PTask task, WCreature c) {
  CHECK(!hasTask(c)) << c->getName().bare() << " already has a task";
  CHECK(!taskByCreature.getMaybe(c));
  CHECK(!creatureByTask.getMaybe(task.get()));
  taskByCreature.set(c, task.get());
  creatureByTask.set(task.get(), c);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
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

void TaskMap::takeTask(WCreature c, Task* task) {
  freeTask(task);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  CHECK(!taskByCreature.getMaybe(c));
  CHECK(!creatureByTask.getMaybe(task));
  taskByCreature.set(c, task);
  creatureByTask.set(task, c);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
}

optional<Position> TaskMap::getPosition(Task* task) const {
  if (positionMap.count(task))
    return positionMap.at(task);
  else
    return none;
}

WCreature TaskMap::getOwner(const Task* task) const {
  if (auto c = creatureByTask.getMaybe(const_cast<Task*>(task)))
    return *c;
  else
    return nullptr;
}

void TaskMap::freeTask(Task* task) {
  if (auto c = creatureByTask.getMaybe(task)) {
    CHECK(taskByCreature.getMaybe(*c));
    taskByCreature.erase(*c);
    creatureByTask.erase(task);
    CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  }
}

void TaskMap::setPosition(Task* task, Position position) {
  positionMap[task] = position;
  reversePositions.getOrInit(position).push_back(task);
}

CostInfo TaskMap::freeFromTask(WConstCreature c) {
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

