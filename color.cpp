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
#include "sdl.h"
#include "color.h"

SERIALIZE_DEF(Color, r, g, b, a)

Color Color::WHITE(255, 255, 255);
Color Color::YELLOW(250, 255, 0);
Color Color::LIGHT_BROWN(210, 150, 0);
Color Color::ORANGE_BROWN(250, 150, 0);
Color Color::BROWN(240, 130, 0);
Color Color::DARK_BROWN(100, 40, 0);
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
Color Color::DARK_RED(150, 0, 0);
Color Color::PINK(255, 20, 147);
Color Color::ORANGE(255, 165, 0);
Color Color::BLUE(0, 0, 255);
Color Color::DARK_BLUE(50, 50, 200);
Color Color::LIGHT_BLUE(100, 100, 255);
Color Color::SKY_BLUE(0, 191, 255);
Color Color::PURPLE(160, 32, 240);
Color Color::VIOLET(120, 0, 255);
Color Color::TRANSPARENT(0, 0, 0, 0);

Color::Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) {}
Color::Color(const SDL::SDL_Color& col) :r(col.r), g(col.g), b(col.b), a(col.a) {}
Color::operator SDL::SDL_Color() const { return {r, g, b, a}; }

Color Color::transparency(int trans) const {
  return Color(r, g, b, (Uint8)trans);
}

Color::Color() : Color(0, 0, 0) {}

Color Color::f(double r, double g, double b, double a) {
  return Color(r * 255, g * 255, b * 255, a * 255);
}

Color Color::operator*(Color c2) {
  return Color(r * c2.r / 255, g * c2.g / 255, b * c2.b / 255, a * c2.a / 255);
}

Color Color::blend(Color c) const {
  unsigned int alpha = c.a + 1;
  unsigned int inv_alpha = 256 - c.a;
  return Color(
      (alpha * c.r + inv_alpha * r) >> 8,
      (alpha * c.g + inv_alpha * g) >> 8,
      (alpha * c.b + inv_alpha * b) >> 8,
      a);
}

bool Color::operator == (const Color& rhs) const {
  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

bool Color::operator != (const Color& rhs) const {
  return !(*this == rhs);
}

bool Color::operator <(const Color& rhs) const {
  return std::forward_as_tuple(r, g, b, a) < std::forward_as_tuple(rhs.r, rhs.g, rhs.b, rhs.a);
}

#include "pretty_archive.h"

RICH_ENUM(
    ColorId,
    WHITE,
    MAIN_MENU_ON,
    MAIN_MENU_OFF,
    YELLOW,
    LIGHT_BROWN,
    ORANGE_BROWN,
    BROWN,
    DARK_BROWN,
    LIGHT_GRAY,
    GRAY,
    ALMOST_GRAY,
    DARK_GRAY,
    ALMOST_BLACK,
    ALMOST_DARK_GRAY,
    BLACK,
    ALMOST_WHITE,
    GREEN,
    LIGHT_GREEN,
    DARK_GREEN,
    RED,
    LIGHT_RED,
    DARK_RED,
    PINK,
    ORANGE,
    BLUE,
    DARK_BLUE,
    LIGHT_BLUE,
    SKY_BLUE,
    PURPLE,
    VIOLET,
    TRANSPARENT
);

struct Rgb {
  int SERIAL(r);
  int SERIAL(g);
  int SERIAL(b);
  int SERIAL(a);
  SERIALIZE_ALL(r, g, b, a)
};

MAKE_VARIANT2(ColorDef, ColorId, Rgb);

#define MAP_COLOR(Name) case ColorId::Name: return Color::Name

template<>
void Color::serialize(PrettyInputArchive& ar, unsigned int) {
  ColorDef SERIAL(def);
  ar(def);
  *this = def.visit(
      [&](Rgb c) -> Color {
        return Color(c.r, c.g, c.b, c.a);
      },
      [&](ColorId id) -> Color {
        switch (id) {
          MAP_COLOR(WHITE);
          MAP_COLOR(MAIN_MENU_ON);
          MAP_COLOR(MAIN_MENU_OFF);
          MAP_COLOR(YELLOW);
          MAP_COLOR(LIGHT_BROWN);
          MAP_COLOR(ORANGE_BROWN);
          MAP_COLOR(BROWN);
          MAP_COLOR(DARK_BROWN);
          MAP_COLOR(LIGHT_GRAY);
          MAP_COLOR(GRAY);
          MAP_COLOR(ALMOST_GRAY);
          MAP_COLOR(DARK_GRAY);
          MAP_COLOR(ALMOST_BLACK);
          MAP_COLOR(ALMOST_DARK_GRAY);
          MAP_COLOR(BLACK);
          MAP_COLOR(ALMOST_WHITE);
          MAP_COLOR(GREEN);
          MAP_COLOR(LIGHT_GREEN);
          MAP_COLOR(DARK_GREEN);
          MAP_COLOR(RED);
          MAP_COLOR(LIGHT_RED);
          MAP_COLOR(DARK_RED);
          MAP_COLOR(PINK);
          MAP_COLOR(ORANGE);
          MAP_COLOR(BLUE);
          MAP_COLOR(DARK_BLUE);
          MAP_COLOR(LIGHT_BLUE);
          MAP_COLOR(SKY_BLUE);
          MAP_COLOR(PURPLE);
          MAP_COLOR(VIOLET);
          MAP_COLOR(TRANSPARENT);
        }
      }
  );
}

#undef MAP_COLOR


#include "text_serialization.h"
template<> void Color::serialize(TextInputArchive& ar1, unsigned) {
  int r;
  int g;
  int b;
  int a;
  ar1 >> r >> g >> b >> a;
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

template<> void Color::serialize(TextOutputArchive& ar1, unsigned) {
  int r = this->r;
  int g = this->g;
  int b = this->b;
  int a = this->a;
  ar1 << r << g << b << a;
}

string toString(const Color& c) {
  return "(" + toString<int>(c.r) + ", " + toString<int>(c.g) + ", " + toString<int>(c.b) + ")";
}
