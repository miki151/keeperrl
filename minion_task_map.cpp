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
#include "collective_config.h"

void MinionTaskMap::setValue(MinionTask t, double v) {
  tasks[t] = v;
}

void MinionTaskMap::clear() {
  tasks.clear();
}

double MinionTaskMap::getValue(MinionTask t, bool ignoreTaskLock) const {
  return (locked.contains(t) && !ignoreTaskLock) ? 0 : tasks[t];
}

void MinionTaskMap::toggleLock(MinionTask task) {
  locked.toggle(task);
}

bool MinionTaskMap::isLocked(MinionTask task) const {
  return locked.contains(task);
}

bool MinionTaskMap::hasAnyTask() const {
  for (MinionTask t : ENUM_ALL(MinionTask))
    if (tasks[t] > 0 && !locked.contains(t))
      return true;
  return false;
}

template <class Archive>
void MinionTaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tasks);
  if (version >= 1)
    ar & SVAR(locked);
}

SERIALIZABLE(MinionTaskMap);
