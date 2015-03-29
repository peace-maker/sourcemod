// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2006-2015 AlliedModders LLC
// 
// This file is part of SourcePawn. SourcePawn is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License along with
// SourcePawn. If not, see http://www.gnu.org/licenses/.
//
// Based on pawndbg, the Pawn debugger, by ITB CompuPhase.
//
#include "debugger.h"
#include "smx-v1-image.h"
#include <cctype>
#include "sp_typeutil.h"
#include "environment.h"
#include "x86/frames-x86.h"
#include "x86/jit_x86.h"
#include "watchdog_timer.h"

#if defined __GNUC__
#include <unistd.h>
#endif
#if defined __linux__
#include <termios.h>
#endif
#if defined WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sp {

enum {
  DISP_DEFAULT = 0x10,
  DISP_STRING  = 0x20,
  DISP_BIN     = 0x30,   /* ??? not implemented */
  DISP_HEX     = 0x40,
  DISP_BOOL    = 0x50,
  DISP_FIXED   = 0x60,   /* unused in sourcepawn */
  DISP_FLOAT   = 0x70,
};
#define DISP_MASK 0x0f

// Convince debugging console to act more like an
// interactive input.
#if defined __linux__
tcflag_t GetTerminalLocalMode()
{
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  return term.c_lflag;
}

void SetTerminalLocalMode(tcflag_t flag)
{
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag = flag;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

tcflag_t EnableTerminalEcho()
{
  tcflag_t flags = GetTerminalLocalMode();
  tcflag_t old_flags = flags;
  flags |= (ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | IEXTEN);
  SetTerminalLocalMode(flags);
  return old_flags;
}

void ResetTerminalEcho(tcflag_t flag)
{
  SetTerminalLocalMode(flag);
}
#endif

#if defined WIN32
DWORD EnableTerminalEcho()
{
  DWORD mode, old_mode;
  HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hConsole, &mode);
  old_mode = mode;
  mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE);
  SetConsoleMode(hConsole, mode);
  return old_mode;
}

void ResetTerminalEcho(DWORD mode)
{
  HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
  SetConsoleMode(hConsole, mode);
}
#endif

int InvokeDebugger(PluginContext *ctx)
{
  if (!ctx->IsDebugging())
    return SP_ERROR_NOTDEBUGGING;
  
  Debugger *debugger = ctx->GetDebugger();
  if (!debugger->active())
    return SP_ERROR_NONE;
  
  FrameIterator iter;
  cell_t cip = iter.findCip();
  
  debugger->SetBreakCount(debugger->breakcount() + 1);
  Runmode orig_runmode = debugger->runmode();
  
  // when running until the function exit, check the frame address
  if (debugger->runmode() == Runmode::STEPOUT &&
      ctx->frm() > debugger->lastframe())
  {
    debugger->SetRunmode(Runmode::STEPPING);
  }
  
  bool isBreakpoint = false;
  // when running, check the breakpoints
  if (debugger->runmode() != Runmode::STEPPING &&
      debugger->runmode() != Runmode::STEPOVER)
  {
    // check breakpoint address
    isBreakpoint = debugger->CheckBreakpoint(cip);
    if (!isBreakpoint)
      return SP_ERROR_NONE;
    
    debugger->SetRunmode(Runmode::STEPPING);
  }
  
  // try to avoid halting on the same line twice
  uint32_t line = 0;
  if (ctx->runtime()->LookupLine(cip, &line) == SP_ERROR_NONE) {
    // assume that there are no more than 5 breaks on a single line.
    // if there are, halt.
    if (line == debugger->lastline() && debugger->breakcount() < 5) {
      debugger->SetRunmode(orig_runmode);
      return SP_ERROR_NONE;
    }
  }
  debugger->SetLastLine(line);
  debugger->SetBreakCount(0);
  
  // check whether we are stepping through a sub-function
  if (debugger->runmode() == Runmode::STEPOVER) {
    assert(debugger->lastframe() != 0);
    if (ctx->frm() < debugger->lastframe())
      return SP_ERROR_NONE;
  }
  
  const char *filename;
  if (ctx->runtime()->LookupFile(cip, &filename) == SP_ERROR_NONE) {
    debugger->SetCurrentFile(filename);
  }
  
  // Tell the watchdog to take a break.
  Environment::get()->watchdog()->SetIgnore(true);
  
  // Echo input back and enable basic control
  unsigned int old_flags = EnableTerminalEcho();
  debugger->HandleInput(cip, isBreakpoint);
  ResetTerminalEcho(old_flags);
  
  // Enable the watchdog timer again.
  Environment::get()->watchdog()->SetIgnore(false);
  
  // step OVER functions (so save the stack frame)
  if (debugger->runmode() == Runmode::STEPOVER ||
      debugger->runmode() == Runmode::STEPOUT)
    debugger->SetLastFrame(ctx->frm());

  return SP_ERROR_NONE;
}

Debugger::Debugger(PluginContext *context)
 : context_(context),
   runmode_(RUNNING),
   lastfrm_(0),
   lastline_(-1),
   currentfile_(""),
   active_(false)
{
}

bool
Debugger::Initialize()
{
  if (!breakpoint_map_.init())
    return false;
  
  if (!watch_table_.init())
    return false;
  
  return true;
}

bool
Debugger::active()
{
  return active_;
}

void
Debugger::Activate()
{
  // TODO: Patch all breaks to call the debug handler
  active_ = true;
}

void
Debugger::Deactivate()
{
  // TODO: Unpatch calls
  active_ = false;
  
  ClearAllBreakpoints();
  ClearAllWatches();
  SetRunmode(RUNNING);
}

void
Debugger::ReportError(const IErrorReport& report, FrameIterator& iter)
{
  if (report.IsFatal())
    printf("STOP on FATAL exception: %s\n", report.Message());
  else
    printf("STOP on exception: %s\n", report.Message());
  
  iter.Reset();
  
  // Find the nearest scripted frame.
  for (; !iter.Done(); iter.Next()) {
    if (iter.IsScriptedFrame()) {
      break;
    }
  }
  
  cell_t cip = iter.findCip();
  
  uint32_t line = 0;
  context_->runtime()->LookupLine(cip, &line);
  SetLastLine(line);
  
  const char *filename;
  if (context_->runtime()->LookupFile(cip, &filename) == SP_ERROR_NONE) {
    SetCurrentFile(filename);
  }
  
  // Tell the watchdog to take a break.
  Environment::get()->watchdog()->SetIgnore(true);
  
  // Echo input back and enable basic control
  unsigned int old_flags = EnableTerminalEcho();
  HandleInput(cip, false);
  ResetTerminalEcho(old_flags);
  
  // Enable the watchdog timer again.
  Environment::get()->watchdog()->SetIgnore(false);
  
  // step OVER functions (so save the stack frame)
  if (runmode() == Runmode::STEPOVER ||
      runmode() == Runmode::STEPOUT)
    SetLastFrame(context_->frm());
}

Runmode
Debugger::runmode()
{
  return runmode_;
}

void 
Debugger::SetRunmode(Runmode runmode)
{
  runmode_ = runmode;
}

cell_t
Debugger::lastframe()
{
  return lastfrm_;
}

void
Debugger::SetLastFrame(cell_t lastfrm)
{
  lastfrm_ = lastfrm;
}

uint32_t
Debugger::lastline()
{
  return lastline_;
}

void
Debugger::SetLastLine(uint32_t line)
{
  lastline_ = line;
}

uint32_t
Debugger::breakcount()
{
  return breakcount_;
}

void
Debugger::SetBreakCount(uint32_t breakcount)
{
  breakcount_ = breakcount;
}

const char *
Debugger::currentfile()
{
  return currentfile_;
}

void
Debugger::SetCurrentFile(const char *file)
{
  currentfile_ = file;
}

static char *strstrip(char *string)
{
  int pos;

  /* strip leading white space */
  while (*string!='\0' && *string<=' ')
    memmove(string,string+1,strlen(string));
  /* strip trailing white space */
  for (pos=strlen(string); pos>0 && string[pos-1]<=' '; pos--)
    string[pos-1]='\0';
  return string;
}

static char *skipwhitespace(char *str)
{
  while (*str==' ' || *str=='\t')
    str++;
  return (char*)str;
}

static const char *skippath(const char *str)
{
  const char *p1,*p2;

  /* DOS/Windows pathnames */
  if ((p1=strrchr(str,'\\'))!=NULL)
    p1++;
  else
    p1=str;
  if (p1==str && p1[1]==':')
    p1=str+2;
  /* Unix pathnames */
  if ((p2=strrchr(str,'/'))!=NULL)
    p2++;
  else
    p2=str;
  return p1>p2 ? p1 : p2;
}

void
Debugger::ListCommands(char *command)
{
  if (command == nullptr)
    command = (char *)"";
  
  if (strlen(command) == 0 || !strcmp(command, "?")) {
    printf("At the prompt, you can type debug commands. For example, the word \"step\" is a\n"
           "command to execute a single line in the source code. The commands that you will\n"
           "use most frequently may be abbreviated to a single letter: instead of the full\n"
           "word \"step\", you can also type the letter \"s\" followed by the enter key.\n\n"
           "Available commands:\n");
  }
  else {
    printf("Options for command \"%s\":\n",command);
  }
  
  if (!stricmp(command, "break") || !stricmp(command, "tbreak")) {
    printf("\tUse TBREAK for one-time breakpoints\n\n"
           "\tBREAK\t\tlist all breakpoints\n"
           "\tBREAK n\t\tset a breakpoint at line \"n\"\n"
           "\tBREAK name:n\tset a breakpoint in file \"name\" at line \"n\"\n"
           "\tBREAK func\tset a breakpoint at function with name \"func\"\n"
           "\tBREAK .\tset a breakpoint at the current location\n");
  }
  else if (!stricmp(command, "cbreak")) {
    printf("\tCBREAK n\tremove breakpoint number \"n\"\n"
           "\tCBREAK *\tremove all breakpoints\n");
  }
  else if (!stricmp(command,"cw") || !stricmp(command,"cwatch")) {
    printf("\tCWATCH may be abbreviated to CW\n\n"
           "\tCWATCH n\tremove watch number \"n\"\n"
           "\tCWATCH var\tremove watch from \"var\"\n"
           "\tCWATCH *\tremove all watches\n");
  }
  else if (!stricmp(command, "d") || !stricmp(command, "disp")) {
    printf("\tDISP may be abbreviated to D\n\n"
           "\tDISP\t\tdisplay all variables that are currently in scope\n"
           "\tDISP var\tdisplay the value of variable \"var\"\n"
           "\tDISP var[i]\tdisplay the value of array element \"var[i]\"\n");
  }
  else if (!stricmp(command, "f") || !stricmp(command, "frame")) {
    printf("\tFRAME may be abbreviated to F\n\n");
    printf("\tFRAME n\tselect frame n and show/change local variables in that function\n");
  }
  else if (!stricmp(command, "g") || !stricmp(command, "go")) {
    printf("\tGO may be abbreviated to G\n\n"
           "\tGO\t\trun until the next breakpoint or program termination\n"
           //"\tGO n\t\trun until line number \"n\"\n"
           //"\tGO name:n\trun until line number \"n\" in file \"name\"\n"
           "\tGO func\t\trun until the current function returns (\"step out\")\n");
  }
  else if (!stricmp(command, "set")) {
    printf("\tSET var=value\t\tset variable \"var\" to the numeric value \"value\"\n"
           "\tSET var[i]=value\tset array item \"var\" to a numeric value\n");
  }
  else if (!stricmp(command, "type")) {
    printf("\tTYPE var STRING\tdisplay \"var\" as string\n"
           "\tTYPE var STD\tset default display format (decimal integer)\n"
           "\tTYPE var HEX\tset hexadecimal integer format\n"
           "\tTYPE var FLOAT\tset floating point format\n");
  }
  else if (!stricmp(command, "watch") || !stricmp(command, "w")) {
    printf("\tWATCH may be abbreviated to W\n\n"
           "\tWATCH var\tset a new watch at variable \"var\"\n");
  }
  else if (!stricmp(command, "n") || !stricmp(command, "next") ||
           !stricmp(command, "quit") || !stricmp(command, "pos") ||
           !stricmp(command, "s") || !stricmp(command, "step") ||
           !stricmp(command, "files"))
  {
    printf("\tno additional information\n");
  }
  else {
    printf("\tB(ack)T(race)\t\tdisplay the stack trace\n"
           "\tBREAK\t\tset breakpoint at line number or variable name\n"
           "\tCBREAK\t\tremove breakpoint\n"
           "\tCW(atch)\tremove a \"watchpoint\"\n"
           "\tD(isp)\t\tdisplay the value of a variable, list variables\n"
           "\tFILES\t\tlist all files that this program is composed off\n"
           "\tF(rame)\t\tSelect a frame from the back trace to operate on\n"
           "\tFUNCS\t\tdisplay functions\n"
           "\tG(o)\t\trun program (until breakpoint)\n"
           "\tN(ext)\t\tRun until next line, step over functions\n"
           "\tPOS\t\tShow current file and line\n"
           "\tQUIT\t\texit debugger\n"
           "\tSET\t\tSet a variable to a value\n"
           "\tS(tep)\t\tsingle step, step into functions\n"
           "\tTYPE\t\tset the \"display type\" of a symbol\n"
           "\tW(atch)\t\tset a \"watchpoint\" on a variable\n"
           "\n\tUse \"? <command name>\" to view more information on a command\n");
  }
}

void
Debugger::HandleInput(cell_t cip, bool isBp)
{
  static char lastcommand[32] = "";
  LegacyImage *image = context_->runtime()->image();
  
  FrameIterator frames;
  frames_ = &frames;
  frame_count_ = 0;
  selected_frame_ = 0;
  cip_ = cip;
  frm_ = context_->frm();
  selected_context_ = context_;
  
  // Count the frames
  // Select first scripted frame, if it's not frame 0
  bool selected_first_scripted = false;
  for ( ; !frames.Done(); frames.Next(), frame_count_++) {
    if (!selected_first_scripted && frames.IsScriptedFrame()) {
      selected_frame_ = frame_count_;
      selected_first_scripted = true;
    }
  }
  
  frames.Reset();
  
  if (!isBp)
    printf("STOP at line %d in %s\n", lastline_, skippath(currentfile_));
  else
    printf("BREAK at line %d in %s\n", lastline_, skippath(currentfile_));
  
  // Print all watched variables now.
  ListWatches();
  
  char line[512], command[32];
  int result;
  char *params;
  for ( ;; ) {
    fgets(line, sizeof(line), stdin);
    
    // strip newline character, plus leading or trailing white space
    strstrip(line);
    
    // Repeat the last command, if no new command was given.
    if (strlen(line) == 0) {
      strncpy(line, lastcommand, sizeof(line));
    }
    lastcommand[0] = '\0';
    
    result = sscanf(line, "%8s", command);
    if (result <= 0) {
      ListCommands(nullptr);
      continue;
    }
    
    params = strchr(line, ' ');
    params = (params != nullptr) ? skipwhitespace(params) : (char*)"";
    
    if (!stricmp(command, "?")) {
      result = sscanf(line, "%*s %30s", command);
      ListCommands(result ? command : nullptr);
    }
    else if (!stricmp(command, "quit")) {
      printf("Clearing all breakpoints. Running normally.\n");
      Deactivate();
      return;
    }
    else if (!stricmp(command, "g") || !stricmp(command, "go")) {
      if (!stricmp(params, "func")) {
        SetRunmode(STEPOUT);
        return;
      }
      SetRunmode(RUNNING);
      return;
    }
    else if (!stricmp(command, "s") || !stricmp(command, "step")) {
      strncpy(lastcommand, "s", sizeof(lastcommand));
      SetRunmode(STEPPING);
      return;
    }
    else if (!stricmp(command, "n") || !stricmp(command, "next")) {
      strncpy(lastcommand, "n", sizeof(lastcommand));
      SetRunmode(STEPOVER);
      return;
    }
    else if (!stricmp(command, "funcs")) {
      printf("Listing functions:\n");
      SmxV1Image *imagev1 = (SmxV1Image *)image;
      const uint8_t *cursor = reinterpret_cast<const uint8_t *>(imagev1->debug_syms_);
      const uint8_t *cursor_end = cursor + imagev1->debug_symbols_section_->size;
      for (unsigned int i = 0; i < imagev1->debug_info_->num_syms; i++) {
        if (cursor + sizeof(sp_fdbg_symbol_t) > cursor_end)
          break;

        const sp_fdbg_symbol_t *sym = reinterpret_cast<const sp_fdbg_symbol_t *>(cursor);
        if (sym->ident == sp::IDENT_FUNCTION &&
            sym->name < imagev1->debug_names_section_->size)
        {
          printf("%s", imagev1->debug_names_ + sym->name);
          const char *filename = image->LookupFile(sym->addr);
          if (filename != nullptr) {
            printf("\t(%s)",skippath(filename));
          }
          printf("\n");
        }

        if (sym->dimcount > 0)
          cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
        cursor += sizeof(sp_fdbg_symbol_t);
      }
    }
    else if (!stricmp(command, "bt") || !stricmp(command, "backtrace")) {
      printf("Stack trace:\n");
      DumpStack();
    }
    else if (!stricmp(command, "f") || !stricmp(command, "frame")) {
      if (*params == '\0' || !isdigit(*params)) {
        puts("Invalid syntax. Type \"? frame\" for help.\n");
        continue;
      }
      
      uint32_t frame = atoi(params);
      if (frame < 0 || frame_count_ <= frame) {
        printf("Invalid frame. There are only %d frames on the stack.\n", frame_count_);
        continue;
      }
      
      if (frame == selected_frame_) {
        puts("This frame is already selected.\n");
        continue;
      }
      
      // Select this frame to operate on.
      frames_->Reset();
      uint32_t index = 0;
      for ( ; !frames_->Done(); frames_->Next(), index++) {
        // Iterator is at the chosen frame now.
        if (index == frame)
          break;
      }
      
      if (!frames_->IsScriptedFrame()) {
        printf("%d is not a scripted frame.\n", frame);
        continue;
      }

      // Get the plugin context of the target frame
      selected_context_ = (PluginContext *)frames_->Context();
      image = selected_context_->runtime()->image();

      // Reset the frame iterator again and count all above frames in the context.
      frames_->Reset();
      index = 0;
      uint32_t num_scripted_frames = 0;
      for (; !frames_->Done(); frames_->Next(), index++) {
        // Count the scripted frames in the context to find the right frm pointer.
        if (frames_->IsScriptedFrame() && frames_->Context() == selected_context_)
          num_scripted_frames++;
        // We've reached the chosen frame.
        if (index == frame)
          break;
      }
      
      // Update internal state for this frame.
      selected_frame_ = frame;
      cip_ = frames_->findCip();
      currentfile_ = image->LookupFile(cip_);
      image->LookupLine(cip_, &lastline_);
      
      // Find correct new frame pointer.
      // TODO: make sure all scripted frames are from the same PluginContext
      cell_t frm = selected_context_->frm();
      for (uint32_t i = 1; i < num_scripted_frames; i++) {
        frm = *(cell_t *)(selected_context_->memory() + frm + 4);
      }
      frm_ = frm;
      
      printf("Selected frame %d.\n", frame);
    }
    else if (!stricmp(command, "break") || !stricmp(command, "tbreak")) {
      if (*params == '\0') {
        ListBreakpoints();
      }
      else {
        // check if a filename precedes the breakpoint location
        char *sep;
        const char *filename = currentfile_;
        if ((sep = strchr(params, ':')) != nullptr) {
          /* the user may have given a partial filename (e.g. without a path), so
           * walk through all files to find a match
           */
          *sep = '\0';
          filename = image->FindFileByPartialName(params);
          if (filename == nullptr) {
            printf("Invalid filename.\n");
            continue;
          }
          
          // Skip colon and settle before line number
          params = sep + 1;
        }
        
        params = skipwhitespace(params);
        
        Breakpoint *bp;
        // User specified a line number
        if (isdigit(*params)) {
          bp = AddBreakpoint(filename, strtol(params, NULL, 10)-1, !stricmp(command, "tbreak"));
        }
        // User wants to add a breakpoint at the current location
        else if (*params == '.') {
          bp = AddBreakpoint(filename, lastline_, !stricmp(command, "tbreak"));
        }
        // User specified a function name
        else {
          bp = AddBreakpoint(filename, params, !stricmp(command, "tbreak"));
        }
        
        if (bp == nullptr) {
          printf("Invalid breakpoint\n");
        }
        else {
          uint32_t bpline = 0;
          image->LookupLine(bp->addr(), &bpline);
          printf("Set breakpoint %d in file %s on line %d", breakpoint_map_.elements(), skippath(filename), bpline);
          if (bp->name() != nullptr)
            printf(" in function %s", bp->name());
        }
      }
    }
    else if (!stricmp(command, "cbreak")) {
      if (*params == '*') {
        // clear all breakpoints
        ClearAllBreakpoints();
      }
      else {
        int number = FindBreakpoint(params);
        if (number < 0 || !ClearBreakpoint(number))
          printf("\tUnknown breakpoint (or wrong syntax)\n");
        else
          printf("\tCleared breakpoint %d\n", number);
      }
    }
    else if (!stricmp(command, "disp") || !stricmp(command, "d")) {
      uint32_t idx[sDIMEN_MAX];
      int dim = 0;
      memset(idx, 0, sizeof(idx));
      SmxV1Image *imagev1 = (SmxV1Image *)image;
      if (*params == '\0') {
        // display all variables that are in scope
        const uint8_t *cursor = reinterpret_cast<const uint8_t *>(imagev1->debug_syms_);
        const uint8_t *cursor_end = cursor + imagev1->debug_symbols_section_->size;
        
        sp_fdbg_symbol_t *sym;
        for (unsigned int i = 0; i < imagev1->debug_info_->num_syms; i++) {
          if (cursor + sizeof(sp_fdbg_symbol_t) > cursor_end)
            break;

          sym = (sp_fdbg_symbol_t *)reinterpret_cast<const sp_fdbg_symbol_t *>(cursor);
          if (sym->ident != sp::IDENT_FUNCTION &&
              sym->codestart <= (uint32_t)cip_ &&
              sym->codeend >= (uint32_t)cip_)
          {
            printf("%s\t", (sym->vclass & DISP_MASK) > 0 ? "loc" : "glb");
            bool display = true;
            if(sym->name < imagev1->debug_names_section_->size) {
              printf("%s\t", imagev1->debug_names_ + sym->name);
            }
            
            if (display)
              DisplayVariable(sym, idx, 0);
            printf("\n");
          }

          if (sym->dimcount > 0)
            cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
          cursor += sizeof(sp_fdbg_symbol_t);
        }
      }
      // Display single variable
      else {
        sp_fdbg_symbol_t *sym = nullptr;
        char *indexptr = strchr(params, '[');
        char *behindname = nullptr;
        assert(dim == 0);
        // Parse all [x][y] dimensions
        while (indexptr != nullptr && dim < sDIMEN_MAX) {
          if (behindname == nullptr)
            behindname = indexptr;
          
          indexptr++;
          idx[dim++] = atoi(indexptr);
          indexptr = strchr(indexptr, '[');
        }
        
        // End the string before the first [ temporarily, 
        // so GetVariable only looks for the variable name.
        if (behindname != nullptr)
          *behindname = '\0';
        
        // find the symbol with the smallest scope
        if (imagev1->GetVariable(params, cip_, (const sp_fdbg_symbol_t **)&sym)) {
          // Add the [ back again
          if (behindname != nullptr)
            *behindname = '[';
          
          printf("%s\t%s\t", (sym->vclass & DISP_MASK) > 0 ? "loc" : "glb", params);
          DisplayVariable(sym, idx, dim);
          printf("\n");
        }
        else {
          printf("\tSymbol not found, or not a variable\n");
        }
      }
    }
    else if (!stricmp(command, "set")) {
      char varname[32];
      uint32_t index;
      cell_t value;
      const sp_fdbg_symbol_t *sym = nullptr;
      if (sscanf(params, " %[^[ ][%d] = %d", varname, &index, &value) != 3) {
        index = 0;
        if (sscanf(params, " %[^= ] = %d", varname, &value) != 2) {
          varname[0] = '\0';
        }
      }
      
      if (varname[0] != '\0') {
        // find the symbol with the given range with the smallest scope
        SmxV1Image *imagev1 = (SmxV1Image *)image;
        if (imagev1->GetVariable(varname, cip_, &sym)) {
          SetSymbolValue(sym, index, value);
          if (index > 0)
            printf("%s[%d] set to %d\n", varname, index, value);
          else
            printf("%s set to %d\n", varname, value);
        }
        else {
          printf("Symbol not found or not a variable\n");
        }
      }
      else {
        printf("Invalid syntax for \"set\". Type \"? set\".\n");
      }
    }
    else if (!stricmp(command, "files")) {
      printf("Source files:\n");
      // browse through the file table
      SmxV1Image *imagev1 = (SmxV1Image *)image;
      for (unsigned int i = 0; i < imagev1->debug_info_->num_files; i++) {
        if(imagev1->debug_files_[i].name < imagev1->debug_names_section_->size) {
          printf("%s\n", imagev1->debug_names_ + imagev1->debug_files_[i].name);
        }
      }
    }
    // Change display format of symbol
    else if (!stricmp(command, "type")) {
      char symname[32], *ptr;
      for (ptr=params; *ptr!='\0' && *ptr!=' ' && *ptr!='\t'; ptr++)
        /* nothing */;
      int len = (int)(ptr-params);
      if (len == 0 || len > 31) {
        printf("\tInvalid (or missing) symbol name\n");
      }
      else {
        sp_fdbg_symbol_t *sym;
        strncpy(symname, params, len);
        symname[len] = '\0';
        params = skipwhitespace(ptr);
        
        SmxV1Image *imagev1 = (SmxV1Image *)image;
        if (imagev1->GetVariable(symname, cip_, (const sp_fdbg_symbol_t **)&sym)) {
          assert(sym != nullptr);
          if (!stricmp(params, "std")) {
            sym->vclass = (sym->vclass & DISP_MASK) | DISP_DEFAULT;
          }
          else if (!stricmp(params, "string")) {
            // check array with single dimension
            if (!(sym->ident == sp::IDENT_ARRAY || sym->ident == sp::IDENT_REFARRAY) ||
                sym->dimcount != 1)
              printf("\t\"string\" display type is only valid for arrays with one dimension\n");
            else
              sym->vclass = (sym->vclass & DISP_MASK) | DISP_STRING;
          }
          else if (!stricmp(params, "bin")) {
            sym->vclass = (sym->vclass & DISP_MASK) | DISP_BIN;
          }
          else if (!stricmp(params, "hex")) {
            sym->vclass = (sym->vclass & DISP_MASK) | DISP_HEX;
          }
          else if (!stricmp(params,"float")) {
            sym->vclass = (sym->vclass & DISP_MASK) | DISP_FLOAT;
          }
          else {
            printf("\tUnknown (or missing) display type\n");
          }
          ListWatches();
        }
        else {
          printf("\tUnknown symbol \"%s\"\n",symname);
        }
      }
    }
    else if (!stricmp(command, "pos")) {
      printf("\tfile: %s", skippath(currentfile_));
      
      const char *function = image->LookupFunction(cip_);
      if (function != nullptr)
        printf("\tfunction: %s", function);
      
      printf("\tline: %d", lastline_);
      
      if (selected_frame_ > 0)
        printf("\tframe: %d", selected_frame_);
      
      puts("\n");
    }
    else if (!stricmp(command, "w") || !stricmp(command, "watch")) {
      if (strlen(params) == 0) {
        printf("Missing variable name\n");
        continue;
      }
      if (AddWatch(params))
        ListWatches();
      else
        printf("Invalid watch\n");
    }
    else if (!stricmp(command, "cw") || !stricmp(command, "cwatch")) {
      if (strlen(params) == 0) {
        printf("Missing variable name\n");
        continue;
      }
      
      if (*params == '*') {
        ClearAllWatches();
      }
      else if (isdigit(*params)) {
        // Delete watch by index
        if (!ClearWatch(atoi(params)))
          printf("Bad watch number\n");
      }
      else {
        if (!ClearWatch(params))
          printf("Variable not watched\n");
      }
      ListWatches();
    }
    else {
      printf("\tInvalid command \"%s\", use \"?\" to view all commands\n", command);
    }
  }
}

bool
Debugger::CheckBreakpoint(cell_t cip)
{
  BreakpointMap::Result result = breakpoint_map_.find(cip);
  if(!result.found())
    return false;
  
  // Remove the temporary breakpoint
  if (result->value->temporary()) {
    ClearBreakpoint(result->value);
  }
  
  return true;
}

Breakpoint *
Debugger::AddBreakpoint(const char* file, uint32_t line, bool temporary) {
  LegacyImage *image = context_->runtime()->image();
  const char *targetfile = image->FindFileByPartialName(file);
  if (targetfile == nullptr)
    targetfile = currentfile_;
  
  uint32_t addr;
  if(!image->GetLineAddress(line, targetfile, &addr))
    return nullptr;
  
  Breakpoint *bp;
  {
    BreakpointMap::Insert p = breakpoint_map_.findForAdd(addr);
    if(p.found())
      return p->value;
    
    bp = new Breakpoint(addr, nullptr);
    breakpoint_map_.add(p, addr, bp);
  }
  
  return bp;
}

Breakpoint *
Debugger::AddBreakpoint(const char* file, const char *function, bool temporary) {
  LegacyImage *image = context_->runtime()->image(); 
  const char *targetfile = image->FindFileByPartialName(file);
  if (targetfile == nullptr)
    return nullptr;
  
  uint32_t addr;
  if(!image->GetFunctionAddress(function, targetfile, &addr))
    return nullptr;
  
  Breakpoint *bp;
  {
    BreakpointMap::Insert p = breakpoint_map_.findForAdd(addr);
    if(p.found())
      return p->value;
    
    const char *realname = image->LookupFunction(addr);
    
    bp = new Breakpoint(addr, realname, temporary);
    breakpoint_map_.add(p, addr, bp);
  }
  
  return bp;
}

bool
Debugger::ClearBreakpoint(int number)
{
  if (number <= 0)
    return false;
  
  int i = 0;
  for (BreakpointMap::iterator iter = breakpoint_map_.iter(); !iter.empty(); iter.next()) {
    if (++i == number) {
      iter.erase();
      return true;
    }
  }
  return false;
}

bool
Debugger::ClearBreakpoint(Breakpoint * bp)
{
  BreakpointMap::Result res = breakpoint_map_.find(bp->addr());
  if (!res.found())
    return false;
  
  breakpoint_map_.remove(res);
  return true;
}

void
Debugger::ClearAllBreakpoints()
{
  breakpoint_map_.clear();
}

int
Debugger::FindBreakpoint(char *breakpoint)
{
  LegacyImage *image = context_->runtime()->image();
  breakpoint = skipwhitespace(breakpoint);
  
  // check if a filename precedes the breakpoint location
  char *sep = strrchr(breakpoint, ':');
  if (sep == nullptr && isdigit(*breakpoint))
    return atoi(breakpoint);
  
  const char *filename = currentfile_;
  if (sep != nullptr) {
    /* the user may have given a partial filename (e.g. without a path), so
     * walk through all files to find a match
     */
    *sep = '\0';
    filename = image->FindFileByPartialName(breakpoint);
    if (filename == nullptr)
      return -1;
    
    // Skip past colon
    breakpoint = sep+1;
  }
  
  breakpoint = skipwhitespace(breakpoint);
  Breakpoint *bp;
  const char *fname;
  uint32_t line;
  uint32_t number = 0;
  for (BreakpointMap::iterator iter = breakpoint_map_.iter(); !iter.empty(); iter.next()) {
    bp = iter->value;
    fname = image->LookupFile(bp->addr());
    number++;
    
    // Correct file?
    if (fname != nullptr && !strcmp(fname, filename)) {
      // A function breakpoint
      if (bp->name() != nullptr && !strcmp(breakpoint, bp->name()))
        return number;
      
      // Line breakpoint
      if (image->LookupLine(bp->addr(), &line) &&
          line == strtoul(breakpoint, NULL, 10)-1)
        return number;
    }
  }
  return -1;
}

void
Debugger::ListBreakpoints()
{
  LegacyImage *image = context_->runtime()->image();
  
  Breakpoint *bp;
  uint32_t line;
  const char *filename;
  uint32_t number = 0;
  for (BreakpointMap::iterator iter = breakpoint_map_.iter(); !iter.empty(); iter.next()) {
    bp = iter->value;
    
    printf("%2d  ", ++number);
    if (image->LookupLine(bp->addr(), &line)) {
      printf("line: %d", line);
    }
    
    if (bp->temporary())
      printf("  (TEMP)");
    
    filename = image->LookupFile(bp->addr());
    if (filename != nullptr) {
      printf("\tfile: %s", skippath(filename));
    }
    
    if (bp->name() != nullptr) {
      printf("\tfunc: %s", bp->name());
    }
    printf("\n");
  }
}

bool
Debugger::AddWatch(const char* symname) {
  WatchTable::Insert i = watch_table_.findForAdd(symname);
  if (i.found())
    return false;
  watch_table_.add(i, symname);
  return true;
}

bool
Debugger::ClearWatch(const char* symname)
{
  WatchTable::Result r = watch_table_.find(symname);
  if (!r.found())
    return false;
  watch_table_.remove(r);
  return true;
}

bool
Debugger::ClearWatch(uint32_t num)
{
  if (num < 1 || num > watch_table_.elements())
    return false;
  
  uint32_t index = 1;
  for (WatchTable::iterator iter = WatchTable::iterator(&watch_table_); !iter.empty(); iter.next()) {
    if (num == index++) {
      iter.erase();
      break;
    }
  }
  return true;
}

void
Debugger::ClearAllWatches()
{
  watch_table_.clear();
}

void
Debugger::ListWatches()
{
  SmxV1Image *imagev1 = (SmxV1Image *)selected_context_->runtime()->image();
  uint32_t num = 1;
  AString symname;
  int dim;
  uint32_t idx[sDIMEN_MAX];
  sp_fdbg_symbol_t *sym = nullptr;
  const char *indexptr;
  char *behindname = nullptr;
  for (WatchTable::iterator iter = WatchTable::iterator(&watch_table_); !iter.empty(); iter.next()) {
    symname = *iter;
    
    sym = nullptr;
    indexptr = strchr(symname.chars(), '[');
    behindname = nullptr;
    dim = 0;
    memset(idx, 0, sizeof(idx));
    // Parse all [x][y] dimensions
    while (indexptr != nullptr && dim < sDIMEN_MAX) {
      if (behindname == nullptr)
        behindname = (char *)indexptr;

      indexptr++;
      idx[dim++] = atoi(indexptr);
      indexptr = strchr(indexptr, '[');
    }

    // End the string before the first [ temporarily, 
    // so GetVariable only looks for the variable name.
    if (behindname != nullptr)
      *behindname = '\0';

    // find the symbol with the smallest scope
    if (imagev1->GetVariable(symname.chars(), cip_, (const sp_fdbg_symbol_t **)&sym)) {
      // Add the [ back again
      if (behindname != nullptr)
        *behindname = '[';

      printf("%d  %-12s ", num++, symname.chars());
      DisplayVariable(sym, idx, dim);
      printf("\n");
    }
    else {
      printf("%d  %-12s (not in scope)\n", num++, symname.chars());
    }
  }
}

bool
Debugger::GetSymbolValue(const sp_fdbg_symbol_t* sym, int index, cell_t* value)
{
  cell_t *vptr;
  cell_t base = sym->addr;
  if (sym->vclass & DISP_MASK)
    base += frm_; // addresses of local vars are relative to the frame
  
  // a reference
  if (sym->ident == sp::IDENT_REFERENCE || sym->ident == sp::IDENT_REFARRAY) {
    if (selected_context_->LocalToPhysAddr(base, &vptr) != SP_ERROR_NONE)
      return false;
    
    assert(vptr != nullptr);
    base = *vptr;
  }
  
  if (selected_context_->LocalToPhysAddr(base + index*sizeof(cell_t), &vptr) != SP_ERROR_NONE)
    return false;
  
  if (vptr != nullptr)
    *value = *vptr;
  return vptr != nullptr;
}

bool
Debugger::SetSymbolValue(const sp_fdbg_symbol_t* sym, int index, cell_t value)
{
  cell_t *vptr;
  cell_t base = sym->addr;
  if (sym->vclass & DISP_MASK)
    base += frm_; // addresses of local vars are relative to the frame
  
  // a reference
  if (sym->ident == sp::IDENT_REFERENCE || sym->ident == sp::IDENT_REFARRAY) {
    selected_context_->LocalToPhysAddr(base, &vptr);
    assert(vptr != nullptr);
    base = *vptr;
  }
  
  selected_context_->LocalToPhysAddr(base + index*sizeof(cell_t), &vptr);
  assert(vptr != nullptr);
  *vptr = value;
  return true;
}

char *
Debugger::GetString(sp_fdbg_symbol_t* sym) {
  assert(sym->ident == sp::IDENT_ARRAY || sym->ident == sp::IDENT_REFARRAY);
  assert(sym->dimcount == 1);
  
  // get the starting address and the length of the string
  cell_t *addr;
  cell_t base = sym->addr;
  if (sym->vclass)
    base += frm_; // addresses of local vars are relative to the frame
  if (sym->ident == sp::IDENT_REFARRAY) {
    selected_context_->LocalToPhysAddr(base, &addr);
    assert(addr != nullptr);
    base = *addr;
  }
  
  char *str;
  if (selected_context_->LocalToStringNULL(base, &str) != SP_ERROR_NONE)
    return nullptr;
  return str;
}

void
Debugger::PrintValue(long value, int disptype)
{
  switch(disptype)
  {
    case DISP_FLOAT:
      printf("%f", sp_ctof(value));
      break;
    case DISP_HEX:
      printf("%lx", value);
      break;
    case DISP_BOOL:
      switch(value)
      {
        case 0:
          printf("false");
          break;
        case 1:
          printf("true");
          break;
        default:
          printf("%ld (false)", value);
          break;
      }
      break;
    default:
      printf("%ld", value);
      break;
  }
}

void
Debugger::DisplayVariable(sp_fdbg_symbol_t *sym, uint32_t index[], int idxlevel)
{
  const sp_fdbg_arraydim_t *symdim = nullptr;
  cell_t value;
  SmxV1Image *image = (SmxV1Image *)selected_context_->runtime()->image();
  
  assert(index != NULL);
  
  // first check whether the variable is visible at all
  if ((uint32_t)cip_ < sym->codestart || (uint32_t)cip_ > sym->codeend) {
    printf("(not in scope)");
    return;
  }
  
  // set default display type for the symbol (if none was set)
  if ((sym->vclass & ~DISP_MASK) == 0) {
    const char *tagname = image->GetTagName(sym->tagid);
    if (tagname != nullptr) {
      if (!stricmp(tagname, "bool")) {
        sym->vclass |= DISP_BOOL;
      }
      else if (!stricmp(tagname, "float")) {
        sym->vclass |= DISP_FLOAT;
      }
    }
    if ((sym->vclass & ~DISP_MASK) == 0 &&
        (sym->ident == sp::IDENT_ARRAY || sym->ident == sp::IDENT_REFARRAY) &&
         sym->dimcount == 1)
    {
      /* untagged array with a single dimension, walk through all elements
       * and check whether this could be a string
       */
      char *ptr = GetString(sym);
      if (ptr != nullptr)
      {
        uint32_t i;
        for (i = 0; i < MAXLINELENGTH-1 && ptr[i] != '\0'; i++) {
          if ((ptr[i] < ' ' && ptr[i] != '\n' && ptr[i] != '\r' && ptr[i] != '\t') ||
              ptr[i] >= 128)
            break; // non-ASCII character
          if (i == 0 && !isalpha(ptr[i]))
            break; // want a letter at the start
        }
        if (i > 0 && i < MAXLINELENGTH-1 && ptr[i] == '\0')
          sym->vclass |= DISP_STRING;
      }
    }
  }
  
  if (sym->ident == sp::IDENT_ARRAY || sym->ident == sp::IDENT_REFARRAY) {
    int dim;
    image->GetArrayDim(sym, &symdim);
    // check whether any of the indices are out of range
    assert(symdim != nullptr);
    for (dim = 0; dim < idxlevel; dim++) {
      if (symdim[dim].size > 0 && index[dim] >= symdim[dim].size)
        break;
    }
    if (dim < idxlevel) {
      printf("(index out of range)");
      return;
    }
  }
  
  if ((sym->ident == sp::IDENT_ARRAY || sym->ident == sp::IDENT_REFARRAY) &&
      idxlevel == 0)
  {
    if ((sym->vclass & ~DISP_MASK) == DISP_STRING) {
      char *str = GetString(sym);
      if (str != nullptr)
        printf("\"%s\"", str); // TODO: truncate to 40 chars
      else
        printf("NULL_STRING");
    }
    else if (sym->dimcount == 1) {
      assert(symdim != nullptr); // set in the previous block
      uint32_t len = symdim[0].size;
      if (len > 5)
        len = 5;
      else if(len == 0)
        len = 1; // unknown array length, assume at least 1 element

      printf("{");
      uint32_t i;
      for (i = 0; i < len; i++) {
        if (i > 0)
          printf(",");
        if (GetSymbolValue(sym, i, &value))
          PrintValue(value, (sym->vclass & ~DISP_MASK));
        else
          printf("?");
      }
      if (len < symdim[0].size || symdim[0].size == 0)
        printf(",...");
      printf("}");
    }
    else {
      printf("(multi-dimensional array)");
    }
  }
  else if (sym->ident != sp::IDENT_ARRAY && sym->ident != sp::IDENT_REFARRAY && idxlevel > 0) {
    // index used on a non-array
    printf("(invalid index, not an array)");
  }
  else {
    // simple variable, or indexed array element
    assert(idxlevel > 0 || index[0] == 0); // index should be zero if non-array
    int dim;
    int base = 0;
    for (dim = 0; dim < idxlevel-1; dim++) {
      base += index[dim];
      if (!GetSymbolValue(sym, base, &value))
        break;
      base += value / sizeof(cell_t);
    }
    
    if (GetSymbolValue(sym, base + index[dim], &value) &&
        sym->dimcount == idxlevel)
      PrintValue(value, (sym->vclass & ~DISP_MASK));
    else if (sym->dimcount != idxlevel)
      printf("(invalid number of dimensions)");
    else
      printf("?");
  }
}

void
Debugger::DumpStack()
{
  FrameIterator &iter = *frames_;
  
  iter.Reset();
  
  uint32_t index = 0;
  for (; !iter.Done(); iter.Next(), index++) {
    if (index == selected_frame_) {
      fputs("->", stdout);
    }
    else {
      fputs("  ", stdout);
    }
    
    const char *name = iter.FunctionName();
    if (!name) {
      fprintf(stdout, "[%d] <unknown>\n", index);
      continue;
    }

    if (iter.IsScriptedFrame()) {
      const char *file = iter.FilePath();
      if (!file)
        file = "<unknown>";
      fprintf(stdout, "[%d] %s::%s, line %d\n", index, file, name, iter.LineNumber());
    } else {
      fprintf(stdout, "[%d] %s()\n", index, name);
    }
  }
}

} // namespace sp