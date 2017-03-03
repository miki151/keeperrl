#pragma once

struct LoadGameChoice { bool operator == (LoadGameChoice) const { return true; } };
struct GoBackChoice { bool operator == (GoBackChoice) const { return true; } };
using PlayerRoleChoice = variant<PlayerRole, LoadGameChoice, GoBackChoice>;

