#pragma once
#include "util.h"

struct ExitAndQuit {
};

using ExitInfo = variant<ExitAndQuit, GameSaveType>;
