#include "stdafx.h"

#include "animation.h"
#include "view_object.h"

Animation::Animation(milliseconds d) : duration(d) {}

bool Animation::isDone(milliseconds time) const {
  return begin && time > *begin + duration;
}

void Animation::setBegin(milliseconds time) {
  CHECK(!begin);
  begin = time;
}

void Animation::render(Renderer& r, Rectangle bounds, Vec2 origin, milliseconds time) {
  CHECK(begin);
  CHECK(time - *begin <= duration) << time << " " << *begin << " " << duration;
  renderSpec(r, bounds, origin, (double)(time - *begin).count() / duration.count());
}

class ThrownObject : public Animation {
  public:
  ThrownObject(Vec2 dir, ViewId obj, bool sprite, Vec2 sz)
    : Animation(milliseconds{dir.length8()}), direction(dir), viewObject(obj), useSprite(sprite),
      squareSize(sz) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, double state) {
    int x = origin.x + state * direction.x;
    int y = origin.y + state * direction.y;
    renderer.drawViewObject(Vec2(x, y), viewObject, useSprite, squareSize, Color::WHITE,
        Renderer::SpriteOrientation(direction, false));
  }

  private:
  Vec2 direction;
  ViewId viewObject;
  bool useSprite;
  Vec2 squareSize;
};

PAnimation Animation::thrownObject(Vec2 direction, ViewId obj, bool useSprite, Vec2 squareSize) {
  return PAnimation(new ThrownObject(direction, obj, useSprite, squareSize));
}

class SpriteAnim : public Animation {
  public:
  struct FrameInfo {
    Vec2 origin;
    Vec2 size;
    Vec2 offset;
  };
  SpriteAnim(int frame, int tile, vector<FrameInfo> f)
      : Animation(milliseconds{frame * f.size()}), frames(f), tileNum(tile) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, double state) {
    return;
    FrameInfo current = frames[min<int>(frames.size() - 1, max(0, int(state * frames.size())))];
    renderer.drawSprite(origin + current.offset, current.origin, current.size, renderer.tiles[tileNum]);
  }

  private:
  vector<FrameInfo> frames;
  int tileNum;
};
 
PAnimation Animation::fromId(AnimationId id) {
  return PAnimation(new SpriteAnim(50, 6, {
        {{510, 628}, {36, 36}, {0, 0}},
        {{683, 611}, {70, 70}, {-17, -17}},
        {{577, 598}, {94, 94}, {-29, -29}},
        }));
}

class Particle {
  public:
  Particle(Vec2 position, Vec2 origin, Color color, Vec2 speed, int size)
      : position(position), origin(origin), color(color), speed(speed), size(size) {
    color.a = 128;
  }

  void update(double deltaState) {
    // custom particle effect logic
    position += speed * deltaState;

    if (size > 1) {
      size -= 10 * deltaState;
    }

    color.r -= 100 * deltaState;
    color.g -= 100 * deltaState;
    color.b -= 100 * deltaState;  
    color.a -= 100 * deltaState;
  }

  void draw(Renderer& renderer) {
    renderer.drawPoint(position + origin, color, size);
  }

  private:
  Vec2 position;
  Vec2 origin;
  Color color;
  Vec2 speed; 
  int size;

  friend class ParticleEffect;
};

class ParticleEffect : public Animation {
  public:
  ParticleEffect(milliseconds duration, unsigned int particleNum, Vec2 origin)
      : Animation(duration), origin(origin), lastState(0.0) {
    // custom particle effect setup
    particles.reserve(8);
    int size = 15;
    
    particles.push_back(Particle(Vec2(1,1), origin, Color::LIGHT_RED, Vec2(30,30), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::LIGHT_RED, Vec2(30,-30), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::LIGHT_RED, Vec2(-30,30), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::LIGHT_RED, Vec2(-30,-30), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::YELLOW, Vec2(0,70), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::YELLOW, Vec2(70,0), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::YELLOW, Vec2(-70,0), size));
    particles.push_back(Particle(Vec2(1,1), origin, Color::YELLOW, Vec2(0,-70), size));
  }

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, double state) {

    double deltaState = state - lastState;
    lastState = state;

    // update particles
    for (auto& particle : particles) {
      particle.update(deltaState);
    }
    // render particles
    for (auto& particle : particles) {
      particle.draw(renderer);
    } 
  } 

  private:
  Vec2 origin;
  vector<Particle> particles;
  double lastState;
};

PAnimation Animation::perticleEffect(int id, milliseconds duration, unsigned int particleNum, Vec2 origin) {
  return PAnimation(new ParticleEffect(duration, particleNum, origin));
}