/* This public domain code prints a stacktrace on program crash.
  Copied from https://gist.github.com/jvranish/4441299*/

#include "stdafx.h"
#include "stack_printer.h"

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef WINDOWS
  #include <windows.h>
  #include <imagehlp.h>
#else
  #include <err.h>
  #include <execinfo.h>
#endif


static char const * icky_global_program_name;
time_t startTime;

/* Resolve symbol name and source location given the path to the executable 
   and an address */
int addr2line(char const * const program_name, void const * const addr)
{
  char addr2line_cmd[512] = {0};

  /* have addr2line map the address to the relent line in the code */
  #ifdef OSX
    /* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o %.256s %p >> stacktrace.out 2>&1", program_name, addr); 
  #else
    sprintf(addr2line_cmd,"addr2line -C -f -p -e \"%.256s\" %p >> stacktrace.out 2>&1", program_name, addr); 
  #endif

  return system(addr2line_cmd);
}



#ifdef WINDOWS

int printStacktraceWithGdb() {
  char gdbcmd[512] = {0};
  sprintf(gdbcmd, "rungdb.bat %d", GetCurrentProcessId());
  fputs(gdbcmd, stderr);
  fflush(stderr);
  return system(gdbcmd);
}

  void windows_print_stacktrace(CONTEXT* context)
  {
    SymInitialize(GetCurrentProcess(), 0, true);

    STACKFRAME frame = { 0 };

    /* setup initial stack frame */
#ifdef AMD64
    frame.AddrPC.Offset         = context->Rip;
    frame.AddrStack.Offset      = context->Rsp;
    frame.AddrFrame.Offset      = context->Rsp;
#else
    frame.AddrPC.Offset         = context->Eip;
    frame.AddrStack.Offset      = context->Esp;
    frame.AddrFrame.Offset      = context->Ebp;
#endif
    frame.AddrPC.Mode           = AddrModeFlat;
    frame.AddrStack.Mode        = AddrModeFlat;
    frame.AddrFrame.Mode        = AddrModeFlat;

#ifdef AMD64
    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64,
#else
    while (StackWalk(IMAGE_FILE_MACHINE_I386,
#endif
                     GetCurrentProcess(),
                     GetCurrentThread(),
                     &frame,
                     context,
                     0,
                     SymFunctionTableAccess,
                     SymGetModuleBase,
                     0 ) )
    {
      addr2line(icky_global_program_name, (void*)frame.AddrPC.Offset);
    }

    SymCleanup( GetCurrentProcess() );
  }

  LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
  {
	  fputs("Printing windows stack.\n", stderr);
    if (!printStacktraceWithGdb("gdb.exe")) {
      fputs("Successfully printed stacktrace using GDB.\n", stderr);
      fflush(stderr);
      return EXCEPTION_EXECUTE_HANDLER;
    }
    fputs("Failed to print stacktrace using GDB. Using built-in stack printer.\n", stderr);
    fflush(stderr);

    switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
      case EXCEPTION_ACCESS_VIOLATION:
        fputs("Error: EXCEPTION_ACCESS_VIOLATION\n", stderr);
        break;
      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        fputs("Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n", stderr);
        break;
      case EXCEPTION_BREAKPOINT:
        fputs("Error: EXCEPTION_BREAKPOINT\n", stderr);
        break;
      case EXCEPTION_DATATYPE_MISALIGNMENT:
        fputs("Error: EXCEPTION_DATATYPE_MISALIGNMENT\n", stderr);
        break;
      case EXCEPTION_FLT_DENORMAL_OPERAND:
        fputs("Error: EXCEPTION_FLT_DENORMAL_OPERAND\n", stderr);
        break;
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        fputs("Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n", stderr);
        break;
      case EXCEPTION_FLT_INEXACT_RESULT:
        fputs("Error: EXCEPTION_FLT_INEXACT_RESULT\n", stderr);
        break;
      case EXCEPTION_FLT_INVALID_OPERATION:
        fputs("Error: EXCEPTION_FLT_INVALID_OPERATION\n", stderr);
        break;
      case EXCEPTION_FLT_OVERFLOW:
        fputs("Error: EXCEPTION_FLT_OVERFLOW\n", stderr);
        break;
      case EXCEPTION_FLT_STACK_CHECK:
        fputs("Error: EXCEPTION_FLT_STACK_CHECK\n", stderr);
        break;
      case EXCEPTION_FLT_UNDERFLOW:
        fputs("Error: EXCEPTION_FLT_UNDERFLOW\n", stderr);
        break;
      case EXCEPTION_ILLEGAL_INSTRUCTION:
        fputs("Error: EXCEPTION_ILLEGAL_INSTRUCTION\n", stderr);
        break;
      case EXCEPTION_IN_PAGE_ERROR:
        fputs("Error: EXCEPTION_IN_PAGE_ERROR\n", stderr);
        break;
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
        fputs("Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n", stderr);
        break;
      case EXCEPTION_INT_OVERFLOW:
        fputs("Error: EXCEPTION_INT_OVERFLOW\n", stderr);
        break;
      case EXCEPTION_INVALID_DISPOSITION:
        fputs("Error: EXCEPTION_INVALID_DISPOSITION\n", stderr);
        break;
      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        fputs("Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n", stderr);
        break;
      case EXCEPTION_PRIV_INSTRUCTION:
        fputs("Error: EXCEPTION_PRIV_INSTRUCTION\n", stderr);
        break;
      case EXCEPTION_SINGLE_STEP:
        fputs("Error: EXCEPTION_SINGLE_STEP\n", stderr);
        break;
      case EXCEPTION_STACK_OVERFLOW:
        fputs("Error: EXCEPTION_STACK_OVERFLOW\n", stderr);
        break;
      default:
        fputs("Error: Unrecognized Exception\n", stderr);
        break;
    }
    fflush(stderr);
    /* If this is a stack overflow then we can't walk the stack, so just show
      where the error happened */
    if (EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        windows_print_stacktrace(ExceptionInfo->ContextRecord);
    }
    else
    {
#ifdef AMD64
        addr2line(icky_global_program_name, (void*)ExceptionInfo->ContextRecord->Rip);
#else
        addr2line(icky_global_program_name, (void*)ExceptionInfo->ContextRecord->Eip);
#endif
    }

    return EXCEPTION_EXECUTE_HANDLER;
  }

  void set_signal_handler()
  {
    SetUnhandledExceptionFilter(windows_exception_handler);
  }
#else

  #define MAX_STACK_FRAMES 64
  static void *stack_traces[MAX_STACK_FRAMES];
  void posix_print_stack_trace()
  {
    int i, trace_size = 0;
    char **messages = (char **)NULL;

    trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
    messages = backtrace_symbols(stack_traces, trace_size);

    /* skip the first couple stack frames (as they are this function and
     our handler) and also skip the last frame as it's (always?) junk. */
    // for (i = 3; i < (trace_size - 1); ++i)
    for (i = 0; i < trace_size; ++i) // we'll use this for now so you can see what's going on
    {
      if (addr2line(icky_global_program_name, stack_traces[i]) != 0)
      {
        printf("  error determining line # for: %s\n", messages[i]);
      }

    }
    if (messages) { free(messages); } 
  }

  void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
  {
    if (!printStacktraceWithGdb("gdb")) {
      fputs("Successfully printed stacktrace using GDB.\n", stderr);
      _Exit(1);
    }
    FILE* errorOut = fopen("stacktrace.out", "a");
    (void)context;
    switch(sig)
    {
      case SIGSEGV:
        fputs("Caught SIGSEGV: Segmentation Fault\n", errorOut);
        break;
      case SIGINT:
        fputs("Caught SIGINT: Interactive attention signal, (usually ctrl+c)\n", errorOut);
        break;
      case SIGFPE:
        switch(siginfo->si_code)
        {
          case FPE_INTDIV:
            fputs("Caught SIGFPE: (integer divide by zero)\n", errorOut);
            break;
          case FPE_INTOVF:
            fputs("Caught SIGFPE: (integer overflow)\n", errorOut);
            break;
          case FPE_FLTDIV:
            fputs("Caught SIGFPE: (floating-point divide by zero)\n", errorOut);
            break;
          case FPE_FLTOVF:
            fputs("Caught SIGFPE: (floating-point overflow)\n", errorOut);
            break;
          case FPE_FLTUND:
            fputs("Caught SIGFPE: (floating-point underflow)\n", errorOut);
            break;
          case FPE_FLTRES:
            fputs("Caught SIGFPE: (floating-point inexact result)\n", errorOut);
            break;
          case FPE_FLTINV:
            fputs("Caught SIGFPE: (floating-point invalid operation)\n", errorOut);
            break;
          case FPE_FLTSUB:
            fputs("Caught SIGFPE: (subscript out of range)\n", errorOut);
            break;
          default:
            fputs("Caught SIGFPE: Arithmetic Exception\n", errorOut);
            break;
        }
      case SIGILL:
        switch(siginfo->si_code)
        {
          case ILL_ILLOPC:
            fputs("Caught SIGILL: (illegal opcode)\n", errorOut);
            break;
          case ILL_ILLOPN:
            fputs("Caught SIGILL: (illegal operand)\n", errorOut);
            break;
          case ILL_ILLADR:
            fputs("Caught SIGILL: (illegal addressing mode)\n", errorOut);
            break;
          case ILL_ILLTRP:
            fputs("Caught SIGILL: (illegal trap)\n", errorOut);
            break;
          case ILL_PRVOPC:
            fputs("Caught SIGILL: (privileged opcode)\n", errorOut);
            break;
          case ILL_PRVREG:
            fputs("Caught SIGILL: (privileged register)\n", errorOut);
            break;
          case ILL_COPROC:
            fputs("Caught SIGILL: (coprocessor error)\n", errorOut);
            break;
          case ILL_BADSTK:
            fputs("Caught SIGILL: (internal stack error)\n", errorOut);
            break;
          default:
            fputs("Caught SIGILL: Illegal Instruction\n", errorOut);
            break;
        }
        break;
      case SIGTERM:
        fputs("Caught SIGTERM: a termination request was sent to the program\n", errorOut);
        break;
      case SIGABRT:
        fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", errorOut);
        break;
      default:
        break;
    }
    time_t curTime = time(0);
    fprintf(errorOut, "Current time %ld, running time %ld\n", curTime, curTime - startTime);
    fclose(errorOut);
    posix_print_stack_trace();
    _Exit(1);
  }

  static uint8_t alternate_stack[SIGSTKSZ];
  void set_signal_handler()
  {
    /* setup alternate stack */
    {
      stack_t ss = {};
      /* malloc is usually used here, I'm not 100% sure my static allocation
         is valid but it seems to work just fine. */
      ss.ss_sp = (void*)alternate_stack;
      ss.ss_size = SIGSTKSZ;
      ss.ss_flags = 0;

      if (sigaltstack(&ss, NULL) != 0) { err(1, "sigaltstack"); }
    }

    /* register our signal handlers */
    {
      struct sigaction sig_action = {};
      sig_action.sa_sigaction = posix_signal_handler;
      sigemptyset(&sig_action.sa_mask);

      #ifdef OSX
          /* for some reason we backtrace() doesn't work on osx
             when we use an alternate stack */
          sig_action.sa_flags = SA_SIGINFO;
      #else
          sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
      #endif

      if (sigaction(SIGSEGV, &sig_action, NULL) != 0) { err(1, "sigaction"); }
      if (sigaction(SIGFPE,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
   //   if (sigaction(SIGINT,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
      if (sigaction(SIGILL,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
      if (sigaction(SIGTERM, &sig_action, NULL) != 0) { err(1, "sigaction"); }
      if (sigaction(SIGABRT, &sig_action, NULL) != 0) { err(1, "sigaction"); }
    }
  }
#endif

void StackPrinter::initialize(const char* program_path, time_t startT)
{
  /* store off program path so we can use it later */
  icky_global_program_name = program_path;
  startTime = startT;
  set_signal_handler();
}

