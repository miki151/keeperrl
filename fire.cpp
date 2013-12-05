#include "stdafx.h"

Fire::Fire(double objectWeight, double objectFlamability) : weight(objectWeight), flamability(objectFlamability) {}

double epsilon = 0.001;

void Fire::tick(Level* level, Vec2 position) {
  burnt = min(1., burnt + size / weight);
  size += (burnt * weight - size) / 10;
  size *= (1 - burnt);
  if (size < epsilon && burnt > 1 - epsilon) {
    size = 0;
    burnt = 1;
  }
}

void Fire::set(double amount) {
  if (!isBurntOut() && amount > epsilon)
    size = max(size, amount * flamability);
}

bool Fire::isBurning() const {
  return size > 0;
}

double Fire::getSize() const {
  return size;
}

bool Fire::isBurntOut() const {
  return burnt > 0.999 && size == 0;
}

