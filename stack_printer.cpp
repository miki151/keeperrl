#ifdef WINDOWS

#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include "version.h"

int printStacktraceWithGdb() {
  char gdbcmd[512] = {0};
  sprintf(gdbcmd, "rungdb.bat \"" BUILD_VERSION " " BUILD_DATE "\"");
  fputs(gdbcmd, stderr);
  fflush(stderr);
  return system(gdbcmd);
}

void miniDumpFunction(unsigned int nExceptionCode, EXCEPTION_POINTERS *pException) {
  HANDLE hFile = CreateFileA("KeeperRL.dmp", GENERIC_READ | GENERIC_WRITE,
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = pException;
    mdei.ClientPointers = FALSE;
    MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
      MiniDumpWithDataSegs |
      MiniDumpWithHandleData |
      MiniDumpWithIndirectlyReferencedMemory |
      MiniDumpWithThreadInfo |
      MiniDumpWithUnloadedModules);
    BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
      hFile, mdt, (pException != nullptr) ? &mdei : nullptr, nullptr, nullptr);
    CloseHandle(hFile);
    printStacktraceWithGdb();
  }
}

LONG WINAPI miniDumpFunction2(EXCEPTION_POINTERS *ExceptionInfo) {
  miniDumpFunction(123, ExceptionInfo);
  return EXCEPTION_EXECUTE_HANDLER;
}


void initializeMiniDump() {
  SetUnhandledExceptionFilter(miniDumpFunction2);
}

#else

void initializeMiniDump() {
}

#endif
