#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"
#include "creature_name.h"

SERIALIZE_DEF(TaskMap, tasks, positionMap, reversePositions, taskByCreature, creatureByTask, marked, completionCost, priorityTasks, delayedTasks, highlight, taskById, taskByActivity, activityByTask);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

void TaskMap::clearFinishedTasks() {
  for (WTask t : getWeakPointers(tasks))
    if (t->isDone())
      removeTask(t);
}

WTask TaskMap::getClosestTask(const Creature* c, MinionActivity activity, bool priorityOnly) const {
  PROFILE;
  WTask closest = nullptr;
  auto isBetter = [&](WTask task, optional<int> dist) {
    PROFILE_BLOCK("isBetter");
    if (!closest)
      return true;
    bool pTask = isPriorityTask(task);
    bool pClosest = isPriorityTask(closest);
    if (pTask && !pClosest)
      return true;
    if (!pTask && pClosest)
      return false;
    return dist.value_or(10000) < getPosition(closest)->dist8(c->getPosition()).value_or(10000);
  };
  optional<StorageId> storageDropTask;
  for (auto& task : taskByActivity[activity])
    if (auto id = task->getStorageId(true))
      if (task->canPerform(c)) {
        storageDropTask = *id;
        break;
      }
  for (auto& task : taskByActivity[activity])
    if (task->canPerform(c) && (!priorityOnly || isPriorityTask(task)) &&
        (!storageDropTask || storageDropTask == task->getStorageId(false)))
      if (auto pos = getPosition(task)) {
        PROFILE_BLOCK("Task check");
        auto dist = pos->dist8(c->getPosition());
        const Creature* owner = getOwner(task);
        auto delayed = delayedTasks.getMaybe(task);
        if (!task->isDone() &&
            (!owner || (task->canTransfer() && dist && pos->dist8(owner->getPosition()).value_or(10000) > *dist && *dist <= 6)) &&
            isBetter(task, dist) &&
            c->canNavigateToOrNeighbor(*pos) &&
            (!delayed || *delayed < *c->getLocalTime())) {
          closest = task;
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

WTask TaskMap::addTaskCost(PTask task, Position position, CostInfo cost, MinionActivity activity) {
  completionCost.set(task.get(), cost);
  return addTask(std::move(task), position, activity);
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
    marked.erase(*pos);
    pos->setNeedsRenderUpdate(true);
  }
  if (auto c = creatureByTask.getMaybe(task)) {
    CHECK(taskByCreature.getMaybe(*c));
     taskByCreature.erase(*c);
    creatureByTask.erase(task);
  }
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  if (auto pos = positionMap.getMaybe(task)) {
    CHECK(reversePositions.count(*pos)) << "Task position not found: " <<
        task->getDescription() << " " << pos->getCoord();
    reversePositions.at(*pos).removeElement(task);
    positionMap.erase(task);
  }
  if (auto activity = activityByTask.getMaybe(task)) {
    activityByTask.erase(task);
    taskByActivity[*activity].removeElement(task);
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
  PROFILE;
  return priorityTasks.contains(t);
}


bool TaskMap::hasPriorityTasks(Position pos) const {
  for (auto task : getTasks(pos))
    if (isPriorityTask(task))
      return true;
  return false;
}

WTask TaskMap::getMarked(Position pos) const {
  return getValueMaybe(marked, pos).value_or(nullptr);
}

void TaskMap::markSquare(Position pos, HighlightType h, PTask task, MinionActivity activity) {
  marked[pos] = task.get();
  pos.setNeedsRenderUpdate(true);
  highlight[pos] = h;
  addTask(std::move(task), pos, activity);
}

HighlightType TaskMap::getHighlightType(Position pos) const {
  return highlight.at(pos);
}

bool TaskMap::hasTask(const Creature* c) const {
  if (auto task = taskByCreature.getMaybe(c))
    return !(*task)->isDone();
  else
    return false;
}

WTask TaskMap::getTask(const Creature* c) {
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
  if (auto tasks = getReferenceMaybe(reversePositions, pos))
    return *tasks;
  else {
    static const vector<WTask> empty;
    return empty;
  }
}

WTask TaskMap::addTaskFor(PTask task, Creature* c) {
  auto previousTask = getTask(c);
  CHECK(!previousTask) << c->getName().bare() << " already has a task " << previousTask->getDescription();
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

WTask TaskMap::addTask(PTask task, Position position, MinionActivity activity) {
  setPosition(task.get(), position);
  taskById.set(task.get(), task.get());
  taskByActivity[activity].push_back(task.get());
  activityByTask.set(task.get(), activity);
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void TaskMap::takeTask(Creature* c, WTask task) {
  freeTask(task);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
  CHECK(!taskByCreature.getMaybe(c));
  CHECK(!creatureByTask.getMaybe(task));
  taskByCreature.set(c, task);
  creatureByTask.set(task, c);
  CHECK(taskByCreature.getSize() == creatureByTask.getSize());
}

optional<Position> TaskMap::getPosition(WTask task) const {
  PROFILE;
  return positionMap.getMaybe(task);
}

Creature* TaskMap::getOwner(WConstTask task) const {
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
  reversePositions[position].push_back(task);
}

CostInfo TaskMap::freeFromTask(const Creature* c) {
  if (WTask task = getTask(c)) {
    if (task->isDone() || !task->canTransfer())
      return removeTask(task);
    else {
      freeTask(task);
      delayedTasks.set(task, *c->getLocalTime() + 50_visible);
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
