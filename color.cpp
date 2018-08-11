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
#include "color.h"

Color Color::WHITE(255, 255, 255);
Color Color::YELLOW(250, 255, 0);
Color Color::LIGHT_BROWN(210, 150, 0);
Color Color::ORANGE_BROWN(250, 150, 0);
Color Color::BROWN(240, 130, 0);
Color Color::DARK_BROWN(100, 60, 0);
Color Color::MAIN_MENU_ON(255, 255, 255, 190);
Color Color::MAIN_MENU_OFF(255, 255, 255, 70);
Color Color::LIGHT_GRAY(150, 150, 150);
Color Color::GRAY(100, 100, 100);
Color Color::ALMOST_GRAY(102, 102, 102);
Color Color::DARK_GRAY(50, 50, 50);
Color Color::ALMOST_BLACK(20, 20, 20);
Color Color::ALMOST_DARK_GRAY(60, 60, 60);
Color Color::BLACK(0, 0, 0);
Color Color::ALMOST_WHITE(200, 200, 200);
Color Color::GREEN(0, 255, 0);
Color Color::LIGHT_GREEN(100, 255, 100);
Color Color::DARK_GREEN(0, 150, 0);
Color Color::RED(255, 0, 0);
Color Color::LIGHT_RED(255, 100, 100);
Color Color::PINK(255, 20, 147);
Color Color::ORANGE(255, 165, 0);
Color Color::BLUE(0, 0, 255);
Color Color::NIGHT_BLUE(0, 0, 20);
Color Color::DARK_BLUE(50, 50, 200);
Color Color::LIGHT_BLUE(100, 100, 255);
Color Color::SKY_BLUE(0, 191, 255);
Color Color::PURPLE(160, 32, 240);
Color Color::VIOLET(120, 0, 255);
Color Color::TRANSLUCENT_BLACK(0, 0, 0);
Color Color::TRANSPARENT(0, 0, 0, 0);

Color::Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) : SDL_Color{r, g, b, a} {
}

Color Color::transparency(int trans) {
  return Color(r, g, b, (Uint8)trans);
}

Color::Color() : Color(0, 0, 0) {
}

Color Color::f(double r, double g, double b, double a) {
  return Color(r * 255, g * 255, b * 255, a * 255);
}

void Color::applyGl() const {
  SDL::glColor4f((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255);
}

Color Color::operator*(Color c2) {
  return Color(r * c2.r / 255, g * c2.g / 255, b * c2.b / 255, a * c2.a / 255);
}
