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
    renderer.drawViewObject(Vec2(x, y), viewObject, useSprite, squareSize);
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
