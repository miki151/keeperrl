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

#include "util.h"
#include "opengl.h"
#include "sdl.h"
#include "color.h"
#include "file_path.h"

class Texture {
  public:
  Texture(const Texture&) = delete;
  Texture(Texture&&) noexcept;
  Texture& operator=(Texture&&) noexcept;
  Texture(const FilePath& path);
  Texture(const FilePath& path, int px, int py, int w, int h);
  Texture(Color, int width, int height);
  explicit Texture(SDL::SDL_Surface*);
  ~Texture();

  static optional<Texture> loadMaybe(const FilePath&);
  optional<SDL::GLenum> loadFromMaybe(SDL::SDL_Surface*);
  bool loadPixels(unsigned char* pixels);

  Vec2 getSize() const {
    return size;
  }
  Vec2 getRealSize() const {
    return realSize;
  }
  const optional<SDL::GLuint>& getTexId() const {
    return texId;
  }
  const optional<FilePath>& getPath() const {
    return path;
  }

  static SDL::SDL_Surface* createSurface(int w, int h);
  static SDL::SDL_Surface* createPowerOfTwoSurface(SDL::SDL_Surface*);

  enum class Filter { nearest, linear };
  enum class Wrapping { repeat, clamp };
  void setParams(Filter, Wrapping);

  private:
  Texture();
  void addTexCoord(int x, int y) const;

  // When texId != none, it's always > 0
  optional<SDL::GLuint> texId;
  Vec2 size;
  Vec2 realSize;
  optional<FilePath> path;
};
