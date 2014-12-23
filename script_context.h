#ifndef _SCRIPT_CONTEXT_H
#define _SCRIPT_CONTEXT_H

#include <angelscript.h>

#define AS_TRY(X) CHECK(X >= 0)

#define ADD_SCRIPT_ENUM(E) ScriptContext::registerEnum<E>(#E)
#define ADD_SCRIPT_VALUE_TYPE(T)\
  AS_TRY(ScriptContext::engine->RegisterObjectType(#T, sizeof(T), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDAK))
#define ADD_SCRIPT_TEMPLATE(T)\
  AS_TRY(ScriptContext::engine->RegisterObjectType(#T, 0, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDAK))
#define ADD_SCRIPT_METHOD(T, NAME, SIG)\
  AS_TRY(ScriptContext::engine->RegisterObjectMethod(#T, SIG, asMETHOD(T, NAME), asCALL_THISCALL));
#define ADD_SCRIPT_FAKE_METHOD(T, NAME, SIG)\
  AS_TRY(ScriptContext::engine->RegisterObjectMethod(#T, SIG, asFUNCTION(NAME), asCALL_CDECL_OBJFIRST));
#define ADD_SCRIPT_METHOD_OVERLOAD(T, NAME, SIG, ...)\
  AS_TRY(ScriptContext::engine->RegisterObjectMethod(#T, SIG, asMETHODPR(T, NAME, __VA_ARGS__), asCALL_THISCALL));
#define ADD_SCRIPT_FUNCTION(NAME, SIG)\
  AS_TRY(ScriptContext::engine->RegisterGlobalFunction(SIG, asFUNCTION(NAME), asCALL_CDECL));
#define ADD_SCRIPT_FUNCTION_OVERLOAD(NAME, SIG, ...)\
  AS_TRY(ScriptContext::engine->RegisterGlobalFunction(SIG, asFUNCTIONPR(NAME, __VA_ARGS__), asCALL_CDECL));
#define ADD_SCRIPT_FUNCTION_FROM_SINGLETON(NAME, SIG, OBJECT)\
  AS_TRY(ScriptContext::engine->RegisterGlobalFunction(SIG, asMETHOD(decltype(OBJECT), NAME),\
        asCALL_THISCALL_ASGLOBAL, &OBJECT));

class ScriptContext {
  public:
  static void init();

  template<typename E>
  static void registerEnum(const string& name) {
    AS_TRY(engine->RegisterEnum(name.c_str()));
    for (E e : ENUM_ALL(E)) {
      AS_TRY(engine->RegisterEnumValue(name.c_str(), EnumInfo<E>::getString(e).c_str(), int(e)));
    }
  }

  static void execute(const string& path, const string& function);

  static asIScriptEngine* engine;
};

#endif
