#pragma once

// Simple framebuffer: allows single texture
class Framebuffer {
  public:
  Framebuffer(int width, int height);
  ~Framebuffer();

  void operator=(const Framebuffer&) = delete;
  Framebuffer(const Framebuffer&) = delete;

  static bool isAvailable();

  void bind();
  static void unbind();

  const int width, height;
  const unsigned id, texId;
};
