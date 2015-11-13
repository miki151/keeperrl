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
    & SVAR(highlight)
    & SVAR(requiredTraits);
}

SERIALIZABLE(TaskMap);

SERIALIZATION_CONSTRUCTOR_IMPL(TaskMap);

TaskMap::TaskMap(const vector<Level*>& levels) : reversePositions(levels), marked(levels), highlight(levels) {
}

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
        if (!task->isDone() &&
            (!owner || (task->canTransfer() && pos->dist8(owner->getPosition()) > dist && dist <= 10)) &&
            (!closest || dist < getPosition(closest)->dist8(c->getPosition()) || isPriorityTask(task.get())) &&
            c->canNavigateTo(*pos) &&
            (!delayedTasks.count(task->getUniqueId()) || delayedTasks.at(task->getUniqueId()) < c->getTime())) {
          closest = task.get();
          if (isPriorityTask(task.get()))
            return task.get();
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

void TaskMap::setPriorityTasks(Position pos) {
  for (Task* t : getTasks(pos))
    priorityTasks.insert(t);
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
  for (Task* task : reversePositions[pos])
    if (isPriorityTask(task))
      return true;
  return false;
}

Task* TaskMap::getMarked(Position pos) const {
  return marked[pos];
}

void TaskMap::markSquare(Position pos, HighlightType h, PTask task) {
  marked[pos] = task.get();
  highlight[pos] = h;
  addTask(std::move(task), pos);
}

HighlightType TaskMap::getHighlightType(Position pos) const {
  return highlight[pos];
}

void TaskMap::unmarkSquare(Position pos) {
  Task* task = marked[pos];
  marked[pos] = nullptr;
  removeTask(task);
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

Task* TaskMap::addTask(PTask task, Position position, MinionTrait required) {
  positionMap[task.get()] = position;
  requiredTraits[task.get()] = required;
  reversePositions[position].push_back(task.get());
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

void TaskMap::freeFromTask(Creature* c) {
  if (Task* t = getTask(c))
    freeTask(t);
}

const map<Task*, CostInfo>& TaskMap::getCompletionCosts() const {
  return completionCost;
}

