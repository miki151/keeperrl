#include <string.h>
#include "scripthelper.h"
#include <string>
#include <assert.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>

using namespace std;

BEGIN_AS_NAMESPACE

int CompareRelation(asIScriptEngine *engine, void *lobj, void *robj, int typeId, int &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when sorting an array,
    //       then the method id and context should be cached between calls.

	int retval = -1;
	asIScriptFunction *func = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opCmp method
		for( asUINT n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *f = ot->GetMethodByIndex(n);
			asDWORD flags;
			if( strcmp(f->GetName(), "opCmp") == 0 &&
				f->GetReturnTypeId(&flags) == asTYPEID_INT32 &&
				flags == asTM_NONE &&
				f->GetParamCount() == 1 )
			{
				int paramTypeId;
				f->GetParam(0, &paramTypeId, &flags);

				// The parameter must be an input reference of the same type
				// If the reference is a inout reference, then it must also be read-only
				if( !(flags & asTM_INREF) || typeId != paramTypeId || ((flags & asTM_OUTREF) && !(flags & asTM_CONST)) )
					break;

				// Found the method
				func = f;
				break;
			}
		}
	}

	if( func )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(func);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = (int)ctx->GetReturnDWord();

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}

	return retval;
}

int CompareEquality(asIScriptEngine *engine, void *lobj, void *robj, int typeId, bool &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when searching for an
	//       entry in a set, then the method and context should be cached between calls.

	int retval = -1;
	asIScriptFunction *func = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opEquals method
		for( asUINT n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *f = ot->GetMethodByIndex(n);
			asDWORD flags;
			if( strcmp(f->GetName(), "opEquals") == 0 &&
				f->GetReturnTypeId(&flags) == asTYPEID_BOOL &&
				flags == asTM_NONE &&
				f->GetParamCount() == 1 )
			{
				int paramTypeId;
				f->GetParam(0, &paramTypeId, &flags);

				// The parameter must be an input reference of the same type
				// If the reference is a inout reference, then it must also be read-only
				if( !(flags & asTM_INREF) || typeId != paramTypeId || ((flags & asTM_OUTREF) && !(flags & asTM_CONST)) )
					break;

				// Found the method
				func = f;
				break;
			}
		}
	}

	if( func )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(func);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = ctx->GetReturnByte() ? true : false;

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}
	else
	{
		// If the opEquals method doesn't exist, then we try with opCmp instead
		int relation;
		retval = CompareRelation(engine, lobj, robj, typeId, relation);
		if( retval >= 0 )
			result = relation == 0 ? true : false;
	}

	return retval;
}

int ExecuteString(asIScriptEngine *engine, const char *code, asIScriptModule *mod, asIScriptContext *ctx)
{
	return ExecuteString(engine, code, 0, asTYPEID_VOID, mod, ctx);
}

int ExecuteString(asIScriptEngine *engine, const char *code, void *ref, int refTypeId, asIScriptModule *mod, asIScriptContext *ctx)
{
	// Wrap the code in a function so that it can be compiled and executed
	string funcCode = " ExecuteString() {\n";
	funcCode += code;
	funcCode += "\n;}";

	// Determine the return type based on the type of the ref arg
	funcCode = engine->GetTypeDeclaration(refTypeId, true) + funcCode;

	// GetModule will free unused types, so to be on the safe side we'll hold on to a reference to the type
	asIObjectType *type = 0;
	if( refTypeId & asTYPEID_MASK_OBJECT )
	{
		type = engine->GetObjectTypeById(refTypeId);
		if( type )
			type->AddRef();
	}

	// If no module was provided, get a dummy from the engine
	asIScriptModule *execMod = mod ? mod : engine->GetModule("ExecuteString", asGM_ALWAYS_CREATE);

	// Now it's ok to release the type
	if( type )
		type->Release();

	// Compile the function that can be executed
	asIScriptFunction *func = 0;
	int r = execMod->CompileFunction("ExecuteString", funcCode.c_str(), -1, 0, &func);
	if( r < 0 )
		return r;

	// If no context was provided, request a new one from the engine
	asIScriptContext *execCtx = ctx ? ctx : engine->CreateContext();
	r = execCtx->Prepare(func);
	if( r < 0 )
	{
		func->Release();
		if( !ctx ) execCtx->Release();
		return r;
	}

	// Execute the function
	r = execCtx->Execute();

	// Unless the provided type was void retrieve it's value
	if( ref != 0 && refTypeId != asTYPEID_VOID )
	{
		if( refTypeId & asTYPEID_OBJHANDLE )
		{
			// Expect the pointer to be null to start with
			assert( *reinterpret_cast<void**>(ref) == 0 );
			*reinterpret_cast<void**>(ref) = *reinterpret_cast<void**>(execCtx->GetAddressOfReturnValue());
			engine->AddRefScriptObject(*reinterpret_cast<void**>(ref), engine->GetObjectTypeById(refTypeId));
		}
		else if( refTypeId & asTYPEID_MASK_OBJECT )
		{
			// Expect the pointer to point to a valid object
			assert( *reinterpret_cast<void**>(ref) != 0 );
			engine->AssignScriptObject(ref, execCtx->GetAddressOfReturnValue(), engine->GetObjectTypeById(refTypeId));
		}
		else
		{
			// Copy the primitive value
			memcpy(ref, execCtx->GetAddressOfReturnValue(), engine->GetSizeOfPrimitiveType(refTypeId));
		}
	}

	// Clean up
	func->Release();
	if( !ctx ) execCtx->Release();

	return r;
}

int WriteConfigToFile(asIScriptEngine *engine, const char *filename)
{
	ofstream strm;
	strm.open(filename);

	return WriteConfigToStream(engine, strm);
}

int WriteConfigToStream(asIScriptEngine *engine, ostream &strm)
{
	// A helper function for escaping quotes in default arguments
	struct Escape
	{
		static string Quotes(const char *decl)
		{
			string str = decl;
			size_t pos = 0;
			for(;;)
			{
				// Find " characters
				pos = str.find("\"",pos);
				if( pos == string::npos )
					break;

				// Add a \ to escape them
				str.insert(pos, "\\");
				pos += 2;
			}
			
			return str;
		}
	};


	int c, n;

	asDWORD currAccessMask = 0;
	string currNamespace = "";

	// Export the engine version, just for info
	strm << "// AngelScript " << asGetLibraryVersion() << "\n";
	strm << "// Lib options " <<  asGetLibraryOptions() << "\n";

	// Export the relevant engine properties
	strm << "// Engine properties\n";
	for( n = 0; n < asEP_LAST_PROPERTY; n++ )
		strm << "ep " << n << " " << engine->GetEngineProperty(asEEngineProp(n)) << "\n";

	// Make sure the default array type is expanded to the template form
	bool expandDefArrayToTempl = engine->GetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL) ? true : false;
	engine->SetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL, true);

	// Write enum types and their values
	strm << "\n// Enums\n";
	c = engine->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		asDWORD accessMask;
		const char *nameSpace;
		const char *enumName = engine->GetEnumByIndex(n, &typeId, &nameSpace, 0, &accessMask);
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		strm << "enum " << enumName << "\n";
		for( int m = 0; m < engine->GetEnumValueCount(typeId); m++ )
		{
			const char *valName;
			int val;
			valName = engine->GetEnumValueByIndex(typeId, m, &val);
			strm << "enumval " << enumName << " " << valName << " " << val << "\n";
		}
	}

	// Enumerate all types
	strm << "\n// Types\n";

	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		asIObjectType *type = engine->GetObjectTypeByIndex(n);
		asDWORD accessMask = type->GetAccessMask();
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		const char *nameSpace = type->GetNamespace();
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		if( type->GetFlags() & asOBJ_SCRIPT_OBJECT )
		{
			// This should only be interfaces
			assert( type->GetSize() == 0 );

			strm << "intf " << type->GetName() << "\n";
		}
		else
		{
			// Only the type flags are necessary. The application flags are application
			// specific and doesn't matter to the offline compiler. The object size is also
			// unnecessary for the offline compiler
			strm << "objtype \"" << engine->GetTypeDeclaration(type->GetTypeId()) << "\" " << (unsigned int)(type->GetFlags() & 0xFF) << "\n";
		}
	}

	c = engine->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		asDWORD accessMask;
		const char *nameSpace;
		const char *typeDef = engine->GetTypedefByIndex(n, &typeId, &nameSpace, 0, &accessMask);
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		strm << "typedef " << typeDef << " \"" << engine->GetTypeDeclaration(typeId) << "\"\n";
	}

	c = engine->GetFuncdefCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *funcDef = engine->GetFuncdefByIndex(n);
		asDWORD accessMask = funcDef->GetAccessMask();
		const char *nameSpace = funcDef->GetNamespace();
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		strm << "funcdef \"" << funcDef->GetDeclaration() << "\"\n";
	}

	// Write the object types members
	// TODO: All function declarations must use escape sequences for " so as not to cause the parsing of the file to fail
	strm << "\n// Type members\n";

	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		asIObjectType *type = engine->GetObjectTypeByIndex(n);
		const char *nameSpace = type->GetNamespace();
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		string typeDecl = engine->GetTypeDeclaration(type->GetTypeId());
		if( type->GetFlags() & asOBJ_SCRIPT_OBJECT )
		{
			for( asUINT m = 0; m < type->GetMethodCount(); m++ )
			{
				asIScriptFunction *func = type->GetMethodByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
					currAccessMask = accessMask;
				}
				strm << "intfmthd " << typeDecl.c_str() << " \"" << Escape::Quotes(func->GetDeclaration(false)).c_str() << "\"\n";
			}
		}
		else
		{
			asUINT m;
			for( m = 0; m < type->GetFactoryCount(); m++ )
			{
				asIScriptFunction *func = type->GetFactoryByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
					currAccessMask = accessMask;
				}
				strm << "objbeh \"" << typeDecl.c_str() << "\" " << asBEHAVE_FACTORY << " \"" << Escape::Quotes(func->GetDeclaration(false)).c_str() << "\"\n";
			}
			for( m = 0; m < type->GetBehaviourCount(); m++ )
			{
				asEBehaviours beh;
				asIScriptFunction *func = type->GetBehaviourByIndex(m, &beh);

				if( beh == asBEHAVE_CONSTRUCT )
					// Prefix 'void'
					strm << "objbeh \"" << typeDecl.c_str() << "\" " << beh << " \"void " << Escape::Quotes(func->GetDeclaration(false)).c_str() << "\"\n";
				else if( beh == asBEHAVE_DESTRUCT )
					// Prefix 'void' and remove ~
					strm << "objbeh \"" << typeDecl.c_str() << "\" " << beh << " \"void " << Escape::Quotes(func->GetDeclaration(false)).c_str()+1 << "\"\n";
				else
					strm << "objbeh \"" << typeDecl.c_str() << "\" " << beh << " \"" << Escape::Quotes(func->GetDeclaration(false)).c_str() << "\"\n";
			}
			for( m = 0; m < type->GetMethodCount(); m++ )
			{
				asIScriptFunction *func = type->GetMethodByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
					currAccessMask = accessMask;
				}
				strm << "objmthd \"" << typeDecl.c_str() << "\" \"" << Escape::Quotes(func->GetDeclaration(false)).c_str() << "\"\n";
			}
			for( m = 0; m < type->GetPropertyCount(); m++ )
			{
				asDWORD accessMask;
				type->GetProperty(m, 0, 0, 0, 0, 0, &accessMask);
				if( accessMask != currAccessMask )
				{
					strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
					currAccessMask = accessMask;
				}
				strm << "objprop \"" << typeDecl.c_str() << "\" \"" << type->GetPropertyDeclaration(m) << "\"\n";
			}
		}
	}

	// Write functions
	strm << "\n// Functions\n";

	c = engine->GetGlobalFunctionCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *func = engine->GetGlobalFunctionByIndex(n);
		const char *nameSpace = func->GetNamespace();
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		asDWORD accessMask = func->GetAccessMask();
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		strm << "func \"" << Escape::Quotes(func->GetDeclaration()).c_str() << "\"\n";
	}

	// Write global properties
	strm << "\n// Properties\n";

	c = engine->GetGlobalPropertyCount();
	for( n = 0; n < c; n++ )
	{
		const char *name;
		int typeId;
		bool isConst;
		asDWORD accessMask;
		const char *nameSpace;
		engine->GetGlobalPropertyByIndex(n, &name, &nameSpace, &typeId, &isConst, 0, 0, &accessMask);
		if( accessMask != currAccessMask )
		{
			strm << "access " << hex << (unsigned int)(accessMask) << dec << "\n";
			currAccessMask = accessMask;
		}
		if( nameSpace != currNamespace )
		{
			strm << "namespace " << nameSpace << "\n";
			currNamespace = nameSpace;
		}
		strm << "prop \"" << (isConst ? "const " : "") << engine->GetTypeDeclaration(typeId) << " " << name << "\"\n";
	}

	// Write string factory
	strm << "\n// String factory\n";
	asDWORD flags = 0;
	int typeId = engine->GetStringFactoryReturnTypeId(&flags);
	if( typeId > 0 )
		strm << "strfactory \"" << ((flags & asTM_CONST) ? "const " : "") << engine->GetTypeDeclaration(typeId) << ((flags & asTM_INOUTREF) ? "&" : "") << "\"\n";

	// Write default array type
	strm << "\n// Default array type\n";
	typeId = engine->GetDefaultArrayTypeId();
	if( typeId > 0 )
		strm << "defarray \"" << engine->GetTypeDeclaration(typeId) << "\"\n";

	// Restore original settings
	engine->SetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL, expandDefArrayToTempl);

	return 0;
}

int ConfigEngineFromStream(asIScriptEngine *engine, istream &strm, const char *configFile)
{
	int r;

	// Some helper functions for parsing the configuration
	struct in
	{
		static asETokenClass GetToken(asIScriptEngine *engine, string &token, const string &text, asUINT &pos)
		{
			int len;
			asETokenClass t = engine->ParseToken(&text[pos], text.length() - pos, &len);
			while( (t == asTC_WHITESPACE || t == asTC_COMMENT) && pos < text.length() )
			{
				pos += len;
				t = engine->ParseToken(&text[pos], text.length() - pos, &len);
			}

			token.assign(&text[pos], len);

			pos += len;

			return t;
		}

		static void ReplaceSlashQuote(string &str)
		{
			size_t pos = 0;
			for(;;)
			{
				// Search for \" in the string
				pos = str.find("\\\"", pos);
				if( pos == string::npos )
					break;

				// Remove the \ character
				str.erase(pos, 1);
			}
		}

		static asUINT GetLineNumber(const string &text, asUINT pos)
		{
			asUINT count = 1;
			for( asUINT n = 0; n < pos; n++ )
				if( text[n] == '\n' )
					count++;

			return count;
		} 
	};

	// Since we are only going to compile the script and never actually execute it,
	// we turn off the initialization of global variables, so that the compiler can
	// just register dummy types and functions for the application interface.
	r = engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false); assert( r >= 0 );

	// Read the entire file
	char buffer[1000];
	string config;
	do {
		strm.getline(buffer, 1000);
		config += buffer;
		config += "\n";
	} while( !strm.eof() );

	// Process the configuration file and register each entity
	asUINT pos  = 0; 
	while( pos < config.length() )
	{
		string token;
		// TODO: The position where the initial token is found should be stored for error messages
		in::GetToken(engine, token, config, pos);
		if( token == "ep" )
		{
			string tmp;
			in::GetToken(engine, tmp, config, pos);

			asEEngineProp ep = asEEngineProp(atol(tmp.c_str()));

			// Only set properties that affect the compiler
			switch( ep )
			{
			case asEP_ALLOW_UNSAFE_REFERENCES:
			case asEP_OPTIMIZE_BYTECODE:
			//case asEP_COPY_SCRIPT_SECTIONS:
			//case asEP_MAX_STACK_SIZE:
			case asEP_USE_CHARACTER_LITERALS:
			case asEP_ALLOW_MULTILINE_STRINGS:
			case asEP_ALLOW_IMPLICIT_HANDLE_TYPES:
			case asEP_BUILD_WITHOUT_LINE_CUES:
			//case asEP_INIT_GLOBAL_VARS_AFTER_BUILD:
			case asEP_REQUIRE_ENUM_SCOPE:
			case asEP_SCRIPT_SCANNER:
			case asEP_INCLUDE_JIT_INSTRUCTIONS:
			case asEP_STRING_ENCODING:
			case asEP_PROPERTY_ACCESSOR_MODE:
			//case asEP_EXPAND_DEF_ARRAY_TO_TMPL:
			//case asEP_AUTO_GARBAGE_COLLECT:
			case asEP_DISALLOW_GLOBAL_VARS:
			case asEP_ALWAYS_IMPL_DEFAULT_CONSTRUCT:
			case asEP_COMPILER_WARNINGS:
			case asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE:
				{
					// Get the value for the property
					in::GetToken(engine, tmp, config, pos);
					stringstream s(tmp);
					asPWORD value;

					s >> value;

					engine->SetEngineProperty(ep, value);
				}
                        default: break;
			}
		}
		else if( token == "namespace" )
		{
			string ns;
			in::GetToken(engine, ns, config, pos);

			r = engine->SetDefaultNamespace(ns.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to set namespace");
				return -1;
			}
		}
		else if( token == "access" )
		{
			string maskStr;
			in::GetToken(engine, maskStr, config, pos);
			asDWORD mask = strtol(maskStr.c_str(), 0, 16);
			engine->SetDefaultAccessMask(mask);
		}
		else if( token == "objtype" )
		{
			string name, flags;
			in::GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			in::GetToken(engine, flags, config, pos);

			// The size of the value type doesn't matter, because the 
			// engine must adjust it anyway for different platforms
			r = engine->RegisterObjectType(name.c_str(), (atol(flags.c_str()) & asOBJ_VALUE) ? 1 : 0, atol(flags.c_str()));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object type");
				return -1;
			}
		}
		else if( token == "objbeh" )
		{
			string name, behaviour, decl;
			in::GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			in::GetToken(engine, behaviour, config, pos);
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);
			in::ReplaceSlashQuote(decl);

			asEBehaviours behave = static_cast<asEBehaviours>(atol(behaviour.c_str()));
			if( behave == asBEHAVE_TEMPLATE_CALLBACK )
			{
				// TODO: How can we let the compiler register this? Maybe through a plug-in system? Or maybe by implementing the callback as a script itself
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_WARNING, "Cannot register template callback without the actual implementation");
			}
			else
			{
				r = engine->RegisterObjectBehaviour(name.c_str(), behave, decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
				if( r < 0 )
				{
					engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register behaviour");
					return -1;
				}
			}
		}
		else if( token == "objmthd" )
		{
			string name, decl;
			in::GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);
			in::ReplaceSlashQuote(decl);

			r = engine->RegisterObjectMethod(name.c_str(), decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object method");
				return -1;
			}
		}
		else if( token == "objprop" )
		{
			string name, decl;
			in::GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			asIObjectType *type = engine->GetObjectTypeById(engine->GetTypeIdByDecl(name.c_str()));
			if( type == 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Type doesn't exist for property registration");
				return -1;
			}

			// All properties must have different offsets in order to make them 
			// distinct, so we simply register them with an incremental offset
			r = engine->RegisterObjectProperty(name.c_str(), decl.c_str(), type->GetPropertyCount());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object property");
				return -1;
			}
		}
		else if( token == "intf" )
		{
			string name, size, flags;
			in::GetToken(engine, name, config, pos);

			r = engine->RegisterInterface(name.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register interface");
				return -1;
			}
		}
		else if( token == "intfmthd" )
		{
			string name, decl;
			in::GetToken(engine, name, config, pos);
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);
			in::ReplaceSlashQuote(decl);

			r = engine->RegisterInterfaceMethod(name.c_str(), decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register interface method");
				return -1;
			}
		}
		else if( token == "func" )
		{
			string decl;
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);
			in::ReplaceSlashQuote(decl);

			r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register global function");
				return -1;
			}
		}
		else if( token == "prop" )
		{
			string decl;
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			// All properties must have different offsets in order to make them 
			// distinct, so we simply register them with an incremental offset.
			// The pointer must also be non-null so we add 1 to have a value.
			r = engine->RegisterGlobalProperty(decl.c_str(), (void*)((long)engine->GetGlobalPropertyCount()+1));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register global property");
				return -1;
			}
		}
		else if( token == "strfactory" )
		{
			string type;
			in::GetToken(engine, type, config, pos);
			type = type.substr(1, type.length() - 2);

			r = engine->RegisterStringFactory(type.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register string factory");
				return -1;
			}
		}
		else if( token == "defarray" )
		{
			string type;
			in::GetToken(engine, type, config, pos);
			type = type.substr(1, type.length() - 2);

			r = engine->RegisterDefaultArrayType(type.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register the default array type");
				return -1;
			}
		}
		else if( token == "enum" )
		{
			string type;
			in::GetToken(engine, type, config, pos);
			
			r = engine->RegisterEnum(type.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register enum type");
				return -1;
			}
		}
		else if( token == "enumval" )
		{
			string type, name, value;
			in::GetToken(engine, type, config, pos);
			in::GetToken(engine, name, config, pos);
			in::GetToken(engine, value, config, pos);

			r = engine->RegisterEnumValue(type.c_str(), name.c_str(), atol(value.c_str()));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register enum value");
				return -1;
			}
		}
		else if( token == "typedef" )
		{
			string type, decl;
			in::GetToken(engine, type, config, pos);
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterTypedef(type.c_str(), decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register typedef");
				return -1;
			}
		}
		else if( token == "funcdef" )
		{
			string decl;
			in::GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterFuncdef(decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, in::GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register funcdef");
				return -1;
			}
		}
	}

	return 0;
}

void PrintException(asIScriptContext *ctx, bool printStack)
{
	if( ctx->GetState() != asEXECUTION_EXCEPTION ) return;

	const asIScriptFunction *function = ctx->GetExceptionFunction();
	printf("func: %s\n", function->GetDeclaration());
	printf("modl: %s\n", function->GetModuleName());
	printf("sect: %s\n", function->GetScriptSectionName());
	printf("line: %d\n", ctx->GetExceptionLineNumber());
	printf("desc: %s\n", ctx->GetExceptionString());

	if( printStack )
	{
		printf("--- call stack ---\n");
		for( asUINT n = 1; n < ctx->GetCallstackSize(); n++ )
		{
			function = ctx->GetFunction(n);
			if( function )
			{
				if( function->GetFuncType() == asFUNC_SCRIPT )
				{
					printf("%s (%d): %s\n", function->GetScriptSectionName(),
											ctx->GetLineNumber(n),
											function->GetDeclaration());
				}
				else
				{
					// The context is being reused by the application for a nested call
					printf("{...application...}: %s\n", function->GetDeclaration());
				}
			}
			else
			{
				// The context is being reused by the script engine for a nested call
				printf("{...script engine...}\n");
			}
		}
	}
}

END_AS_NAMESPACE
