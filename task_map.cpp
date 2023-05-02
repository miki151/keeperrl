#include "stdafx.h"
#include "task_map.h"
#include "creature.h"
#include "task.h"
#include "creature_name.h"
#include "equipment.h"
#include "collective.h"
#include "container_range.h"

void TaskMap::addToTaskByActivity(Task* task, MinionActivity activity) {
  taskByActivity[activity].push_back(task);
  if (isPriorityTask(task))
    priorityTaskByActivity[activity].insertIfDoesntContain(task);
}

template <class Archive>
void TaskMap::serialize(Archive& ar, const unsigned int) {
  if (Archive::is_saving::value) {
    for (auto activity : ENUM_ALL(MinionActivity)) {
      for (auto task : cantPerformByAnyone[activity])
        addToTaskByActivity(task, activity);
      cantPerformByAnyone[activity].clear();
    }
  }
  ar(tasks, positionMap, reversePositions, taskByCreature, creatureByTask, marked, completionCost, priorityTasks, delayedTasks, highlight, taskById, taskByActivity, activityByTask);
  if (Archive::is_loading::value) {
    for (auto activity : ENUM_ALL(MinionActivity)) {
      for (auto& task : taskByActivity[activity])
        if (isPriorityTask(task))
          priorityTaskByActivity[activity].insertIfDoesntContain(task);
    }
  }
}

SERIALIZABLE(TaskMap);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

void TaskMap::tick() {
  for (WTask t : getWeakPointers(tasks)) {
    if (t->isDone())
      removeTask(t);
  }
  for (auto activity : ENUM_ALL(MinionActivity)) {
    for (auto task : Iter(taskByActivity[activity]))
      if (!(*task)->canPerformByAnyone()) {
        task.markToErase();
        cantPerformByAnyone[activity].push_back(*task);
      }
    EntitySet<Task> toErase;
    for (auto task : priorityTaskByActivity[activity].getElems())
      if (!task->canPerformByAnyone())
        toErase.insert(task);
    for (auto& elem : toErase)
      priorityTaskByActivity[activity].remove(elem);
    for (auto task : Iter(cantPerformByAnyone[activity]))
      if ((*task)->canPerformByAnyone()) {
        task.markToErase();
        addToTaskByActivity(*task, activity);
      }
  }
}

WTask TaskMap::getClosestTask(const Creature* c, MinionActivity activity, bool priorityOnly, const Collective* col) const {
  auto header = "getClosestTask " + EnumInfo<MinionActivity>::getString(activity);
  PROFILE_BLOCK(header.data());
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
  auto movementType = c->getMovementType();
  optional<StorageId> storageDropTask;
  {
    PROFILE_BLOCK("StorageId");
    for (auto it : c->getEquipment().getItems())
      if (auto task = col->getItemTask(it))
        if (auto id = task->getStorageId(true))
          if (task->canPerform(c, movementType)) {
            storageDropTask = *id;
            break;
          }
  }
  {
    PROFILE_BLOCK("ByActivity");
    auto& taskList = priorityOnly ? priorityTaskByActivity[activity].getElems() : taskByActivity[activity];
    for (auto& task : taskList)
      if ((!storageDropTask || storageDropTask == task->getStorageId(false)) &&
          task->canPerform(c, movementType))
        if (auto pos = getPosition(task)) {
          PROFILE_BLOCK("Task check");
          auto dist = pos->dist8(c->getPosition());
          const Creature* owner = getOwner(task);
          auto delayed = delayedTasks.getMaybe(task);
          if (!task->isDone() &&
              (!owner || (task->canTransfer() && dist && pos->dist8(owner->getPosition()).value_or(10000) > *dist && *dist <= 6)) &&
              isBetter(task, dist) &&
              pos->canNavigateToOrNeighbor(c->getPosition(), movementType) &&
              (!delayed || *delayed < *c->getLocalTime())) {
            closest = task;
          }
        }
  }
  return closest;
}

vector<WConstTask> TaskMap::getAllTasks() const {
  return tasks.transform([] (const PTask& t) -> WConstTask { return t.get(); });
}

void TaskMap::setPriorityTasks(Position pos) {
  for (WTask t : getTasks(pos)) {
    if (auto activity = activityByTask.getMaybe(t))
      priorityTaskByActivity[*activity].insertIfDoesntContain(t);
    priorityTasks.insert(t);
  }
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
    CHECK(activityByTask.getMaybe(task));
    activityByTask.erase(task);
    taskByActivity[*activity].removeElementMaybe(task);
    priorityTaskByActivity[*activity].removeMaybe(task);
    cantPerformByAnyone[*activity].removeElementMaybe(task);
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
  PROFILE;
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

optional<HighlightType> TaskMap::getHighlightType(Position pos) const {
  if (!!getMarked(pos))
    return getValueMaybe(highlight, pos);
  return none;
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

bool TaskMap::hasTask(Position pos, MinionActivity activity) const {
  if (auto tasks = getReferenceMaybe(reversePositions, pos))
    for (auto task : *tasks)
      if (activityByTask.getMaybe(task) == activity)
        return true;
  return false;
}

vector<WTask> TaskMap::getTasks(MinionActivity a) const {
  return taskByActivity[a];
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
  CHECK(!activityByTask.getMaybe(task.get()));
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
  CHECK(!positionMap.getMaybe(task));
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
