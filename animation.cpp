#include "stdafx.h"

#include "animation.h"
#include "animation_id.h"
#include "renderer.h"
#include "view_object.h"

Animation::Animation(milliseconds d) : duration(d) {}

bool Animation::isDone(milliseconds time) const {
  return begin && time > *begin + duration;
}

void Animation::setBegin(milliseconds time) {
  CHECK(!begin);
  begin = time;
}

void Animation::render(Renderer& r, Rectangle bounds, Vec2 origin, Vec2 squareSize, milliseconds time) {
  CHECK(begin);
  CHECK(time - *begin <= duration) << time << " " << *begin << " " << duration;
  renderSpec(r, bounds, origin, squareSize, (double)(time - *begin).count() / duration.count());
}

class ThrownObject : public Animation {
  public:
  ThrownObject(Vec2 dir, ViewId obj, bool sprite)
    : Animation(milliseconds{dir.length8()}), direction(dir), viewObject(obj), useSprite(sprite) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) {
    int x = origin.x + state * direction.x;
    int y = origin.y + state * direction.y;
    renderer.drawViewObject(Vec2(x, y), viewObject, useSprite, squareSize, Color::WHITE,
        Renderer::SpriteOrientation(direction, false));
  }

  private:
  Vec2 direction;
  ViewId viewObject;
  bool useSprite;
};

PAnimation Animation::thrownObject(Vec2 direction, ViewId obj, bool useSprite) {
  return PAnimation(new ThrownObject(direction, obj, useSprite));
}

class SpriteAnim : public Animation {
  public:

  SpriteAnim(AnimationId id, milliseconds duration, Dir o) : Animation(duration), id(id), orientation(o) {}

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) override {
    renderer.drawAnimation(id, origin, state, squareSize, orientation);
  }

  private:
  AnimationId id;
  Dir orientation;
};
 
PAnimation Animation::fromId(AnimationId id, Dir orientation) {
  return unique<SpriteAnim>(id, getDuration(id), orientation);
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

  virtual void renderSpec(Renderer& renderer, Rectangle bounds, Vec2 origin, Vec2 squareSize, double state) override {

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
