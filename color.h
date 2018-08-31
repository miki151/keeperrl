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

#pragma once

#include "sdl.h"

struct Color : public SDL::SDL_Color {
  Color(Uint8, Uint8, Uint8, Uint8 = 255);
  Color transparency(int);
  static Color f(double, double, double, double = 1.0);
  Color operator*(Color);
  Color();

  bool operator==(const Color&) const;

  static Color WHITE;
  static Color MAIN_MENU_ON;
  static Color MAIN_MENU_OFF;
  static Color YELLOW;
  static Color LIGHT_BROWN;
  static Color ORANGE_BROWN;
  static Color BROWN;
  static Color DARK_BROWN;
  static Color LIGHT_GRAY;
  static Color GRAY;
  static Color ALMOST_GRAY;
  static Color DARK_GRAY;
  static Color ALMOST_BLACK;
  static Color ALMOST_DARK_GRAY;
  static Color BLACK;
  static Color ALMOST_WHITE;
  static Color GREEN;
  static Color LIGHT_GREEN;
  static Color DARK_GREEN;
  static Color RED;
  static Color LIGHT_RED;
  static Color PINK;
  static Color ORANGE;
  static Color BLUE;
  static Color DARK_BLUE;
  static Color NIGHT_BLUE;
  static Color LIGHT_BLUE;
  static Color SKY_BLUE;
  static Color PURPLE;
  static Color VIOLET;
  static Color TRANSLUCENT_BLACK;
  static Color TRANSPARENT;
};
