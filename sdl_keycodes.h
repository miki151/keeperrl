#pragma once
#include "util.h"
#include "sdl.h"

const char* getString(SDL::SDL_Keycode key);
optional<SDL::SDL_Keycode> keycodeFromString(const char* key);
