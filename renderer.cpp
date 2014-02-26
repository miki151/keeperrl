#include "stdafx.h"
#include "renderer.h"

using namespace sf;

Font textFont;
Font tileFont;
Font symbolFont;

namespace colors {
  const Color white(255, 255, 255);
  const Color yellow(250, 255, 0);
  const Color lightBrown(210, 150, 0);
  const Color orangeBrown(250, 150, 0);
  const Color brown(240, 130, 0);
  const Color darkBrown(100, 60, 0);
  const Color lightGray(150, 150, 150);
  const Color gray(100, 100, 100);
  const Color almostGray(102, 102, 102);
  const Color darkGray(50, 50, 50);
  const Color almostBlack(20, 20, 20);
  const Color almostDarkGray(60, 60, 60);
  const Color black(0, 0, 0);
  const Color almostWhite(200, 200, 200);
  const Color green(0, 255, 0);
  const Color lightGreen(100, 255, 100);
  const Color darkGreen(0, 150, 0);
  const Color red(255, 0, 0);
  const Color lightRed(255, 100, 100);
  const Color pink(255, 20, 147);
  const Color orange(255, 165, 0);
  const Color blue(0, 0, 255);
  const Color darkBlue(50, 50, 200);
  const Color lightBlue(100, 100, 255);
  const Color purple(160, 32, 240);
  const Color violet(120, 0, 255);
  const Color translucentBlack(0, 0, 0);

  Color transparency(const Color& color, int trans) {
    return Color(color.r, color.g, color.b, trans);
  }
}

vector<Texture> Renderer::tiles;
const vector<int> Renderer::tileSize { 36, 36, 36, 24, 36, 36 };
const int Renderer::nominalSize = 36;

int Renderer::getTextLength(string s) {
  Text t(s, textFont, textSize);
  return t.getLocalBounds().width;
}

Font& getFont(Renderer::FontId id) {
  switch (id) {
    case Renderer::TEXT_FONT: return textFont;
    case Renderer::TILE_FONT: return tileFont;
    case Renderer::SYMBOL_FONT: return symbolFont;
  }
  FAIL << "ewf";
  return textFont;
}

void Renderer::drawText(FontId id, int size, Color color, int x, int y, String s, bool center) {
  int ox = 0;
  int oy = 0;
  Text t(s, getFont(id), size);
  if (center) {
    sf::FloatRect bounds = t.getLocalBounds();
    ox -= bounds.left + bounds.width / 2;
    //oy -= bounds.top + bounds.height / 2;
  }
  t.setPosition(x + ox, y + oy);
  t.setColor(color);
  display->draw(t);
}

void Renderer::drawText(Color color, int x, int y, string s, bool center, int size) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  drawText(TEXT_FONT, size, color, x, y, utf32, center);
}

void Renderer::drawText(Color color, int x, int y, const char* c, bool center, int size) {
  drawText(TEXT_FONT, size, color, x, y, String(c), center);
}

void Renderer::drawImage(int px, int py, const Image& image, double scale) {
  Texture t;
  t.loadFromImage(image);
  Sprite s(t);
  s.setPosition(px, py);
  if (scale != 1)
    s.setScale(scale, scale);
  display->draw(s);
}

void Renderer::drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw, int dh,
    Optional<Color> color) {
  Sprite s(t, sf::IntRect(px, py, w, h));
  s.setPosition(x, y);
  if (color)
    s.setColor(*color);
  if (dw != -1)
    s.setScale(double(dw) / w, double(dh) / h);
  display->draw(s);
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline) {
  RectangleShape r(Vector2f(t.getW(), t.getH()));
  r.setPosition(t.getPX(), t.getPY());
  r.setFillColor(color);
  if (outline) {
    r.setOutlineThickness(-2);
    r.setOutlineColor(*outline);
  }
  display->draw(r);
}

void Renderer::drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

void Renderer::drawAndClearBuffer() {
  display->display();
  display->clear(Color(0, 0, 0));
}

void Renderer::resize(int width, int height) {
  display->setView(*(sfView = new sf::View(sf::FloatRect(0, 0, width, height))));
}

int Renderer::getWidth() {
  return display->getSize().x;
}

int Renderer::getHeight() {
  return display->getSize().y;
}

void Renderer::initialize(int width, int height, int color, string title) {
  display = new RenderWindow(VideoMode(1024, 600, 32), "KeeperRL");
  sfView = new sf::View(display->getDefaultView());
  CHECK(textFont.loadFromFile("Lato-Bol.ttf"));
  CHECK(tileFont.loadFromFile("Lato-Bol.ttf"));
  CHECK(symbolFont.loadFromFile("Symbola.ttf"));
}

bool Renderer::pollEvent(Event& ev) {
  return display->pollEvent(ev);
}

void Renderer::waitEvent(Event& ev) {
  display->waitEvent(ev);
}

Vec2 Renderer::getMousePos() {
  auto pos = Mouse::getPosition(*display);
  return Vec2(pos.x, pos.y);
}

