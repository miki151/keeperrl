#include "stdafx.h"
#include "util.h"

#include "extern/scriptstdstring.h"
#include "extern/scriptbuilder.h"
#include "extern/scripthelper.h"

#include "script_context.h"

static void MessageCallback(const asSMessageInfo *msg, void *param) {
  string prefix;
  switch (msg->type) {
    case asMSGTYPE_ERROR: prefix = "ERROR"; break;
    case asMSGTYPE_WARNING: prefix = "WARNING"; break;
    case asMSGTYPE_INFORMATION: prefix = "INFO"; break;
  }
  Debug() << msg->section << "(" << msg->row << "," << msg->col << ") : " << prefix << ": " << msg->message;
}

asIScriptEngine* ScriptContext::engine = nullptr;

void ScriptContext::init() {
  if (engine)
    engine->Release();
  engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
  AS_TRY(engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL));
  RegisterStdString(engine);
}

void ScriptContext::execute(const string& path, const string& function) {
  CScriptBuilder builder;
  AS_TRY(builder.StartNewModule(engine, "MyModule"));
  AS_TRY(builder.AddSectionFromFile(path.c_str()));
  AS_TRY(builder.BuildModule());
  asIScriptFunction * func = engine->GetModule("MyModule")->GetFunctionByDecl(function.c_str());
  CHECK(func) << path << ": function not found: " << function;
  asIScriptContext* ctx = engine->CreateContext();
  ctx->Prepare(func);
  int r = ctx->Execute();
  if( r != asEXECUTION_FINISHED ) {
    if( r == asEXECUTION_EXCEPTION ) {
      FAIL << "An exception '" << ctx->GetExceptionString() << "' occurred. Please correct the code and try again.";
    }
  }
  ctx->Release();
}
