/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)
 
 This file is part of KeeperRL.
 
 KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
 GNU General Public License as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along with this program.
 If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"
#include "minion_task_map.h"

void MinionTaskMap::setValue(MinionTask t, double v) {
  tasks[t] = v;
}

void MinionTaskMap::setWorkshopTasks(double v) {
  for (auto task : getWorkshopTasks())
    setValue(task, v);
}

void MinionTaskMap::clear() {
  tasks.clear();
}

double MinionTaskMap::getValue(MinionTask t) const {
  return tasks[t];
}

static MinionTask chooseRandom(EnumMap<MinionTask, double> vi) {
  vector<MinionTask> v;
  vector<double> p;
  for (MinionTask elem : ENUM_ALL(MinionTask)) {
    v.push_back(elem);
    p.push_back(vi[elem]);
  }
  return chooseRandom(v, p);
}

MinionTask MinionTaskMap::getRandom() const {
  return chooseRandom(tasks);
}

bool MinionTaskMap::hasAnyTask() const {
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (tasks[t] > 0)
      return true;
  return false;
}

vector<MinionTask> MinionTaskMap::getAll() const {
  vector<MinionTask> ret;
  for (auto task : ENUM_ALL(MinionTask))
    if (tasks[task] > 0)
      ret.push_back(task);
  sort(ret.begin(), ret.end(), [this] (MinionTask a, MinionTask b) { return tasks[a] > tasks[b];});
  return ret;
}

template <class Archive>
void MinionTaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tasks);
}

SERIALIZABLE(MinionTaskMap);
