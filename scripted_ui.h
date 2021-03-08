#pragma once

#include "stdafx.h"
#include "util.h"
#include "texture_id.h"
#include "pretty_archive.h"
#include "mouse_button_id.h"
#include "color.h"
#include "view_id.h"
#include "sdl.h"
#include <memory>

struct ScriptedUI;
struct ScriptedUIState;
struct ScriptedUIData;
class Renderer;
class GuiFactory;

struct ScriptedContext {
  Renderer* renderer;
  GuiFactory* factory;
  Semaphore* endSemaphore;
  ScriptedUIState& state;
  int sliderCounter;
  int elemCounter;
  int tooltipCounter;
  optional<int> highlightedElemHeight;
};

using EventCallback = function<bool()>;

struct ScriptedUIInterface {
  virtual void render(const ScriptedUIData&, ScriptedContext&, Rectangle) const;
  virtual Vec2 getSize(const ScriptedUIData&, ScriptedContext&) const;
  virtual void onClick(const ScriptedUIData&, ScriptedContext&, MouseButtonId, Rectangle, Vec2, EventCallback&) const;
  virtual void onKeypressed(const ScriptedUIData&, ScriptedContext&, SDL::SDL_Keysym, Rectangle, EventCallback&) const;
  virtual ~ScriptedUIInterface(){}
};

struct ScriptedUI {
  ScriptedUIInterface* operator->();
  const ScriptedUIInterface* operator->() const;
  ScriptedUIInterface& operator*();
  const ScriptedUIInterface& operator*() const;
  ScriptedUI& operator = (const ScriptedUI&) { fail(); }
  ScriptedUI& operator = (ScriptedUI&&) = default;
  ScriptedUI(ScriptedUI&&) noexcept = default;
  ScriptedUI(unique_ptr<ScriptedUIInterface> ui = nullptr) : ui(std::move(ui)) {}
  struct SubclassInfo;
  static vector<SubclassInfo>& getSubclasses();
  void serialize(PrettyInputArchive& ar1, const unsigned int);
  unique_ptr<ScriptedUIInterface> ui;
};

using SubclassSerialize = unique_ptr<ScriptedUIInterface>(*)(PrettyInputArchive&);

struct ScriptedSubclassAdder {
  ScriptedSubclassAdder(const char* name, SubclassSerialize);
};

#define REGISTER_SCRIPTED_UI(CLASS)\
ScriptedSubclassAdder adder##CLASS(#CLASS,\
  [](PrettyInputArchive& ar1) -> unique_ptr<ScriptedUIInterface>{\
    auto ret = unique<CLASS>();\
    ar1(*ret);\
    return std::move(ret);\
  }\
)

