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
#ifndef _include_sourcepawn_vm_debugger_h_
#define _include_sourcepawn_vm_debugger_h_

#include "sp_vm_api.h"
#include "plugin-context.h"
#include <smx/smx-headers.h>
#include <smx/smx-v1.h>
#include "stack-frames.h"

namespace sp {

  using namespace SourcePawn;

  int InvokeDebugger(PluginContext *ctx);
  static void DumpStack(IFrameIterator &iter);
  
  static const uint32_t MAXLINELENGTH = 128;

  class Breakpoint {
  public:
    Breakpoint(ucell_t addr, const char *name, bool temporary = false);

    int number() {
      return number_;
    }
    ucell_t addr() {
      return addr_;
    }
    const char *name() {
      return name_;
    }
  private:
    ucell_t addr_; /* address (in code or data segment) */
    const char *name_; /* name of the symbol (function) */
    int number_; /* sequential breakpoint number (to refer to the breakpoint) */

    static unsigned int last_number;
  };

  enum Runmode {
    STEPPING, /* step into functions */
    STEPOVER, /* step over functions */
    STEPOUT, /* run until the function returns */
    RUNNING, /* just run */
  };

  class Debugger {
  public:
    Debugger(PluginContext *context);
    bool Initialize();
    bool active();
    void Activate();
    void Deactivate();

    void ReportError(const IErrorReport &report, FrameIterator &iter);
    
  public:
    Breakpoint *AddBreakpoint(const char *file, unsigned int line, bool temporary);
    Breakpoint *AddBreakpoint(const char *file, const char *function, bool temporary);
    bool ClearBreakpoint(int number);
    void ClearAllBreakpoints();
    int CheckBreakpoint(cell_t cip);
    int FindBreakpoint(char *breakpoint);
    void ListBreakpoints();
    
    bool AddWatch(const char *symname);
    bool ClearWatch(const char *symname);
    bool ClearWatch(uint32_t num);
    void ClearAllWatches();
    void ListWatches(cell_t cip);
    
    bool GetSymbolValue(const sp_fdbg_symbol_t *sym, int index, cell_t *value);
    bool SetSymbolValue(const sp_fdbg_symbol_t *sym, int index, cell_t value);
    char *GetString(sp_fdbg_symbol_t *sym);
    void PrintValue(long value, int disptype);
    void DisplayVariable(sp_fdbg_symbol_t *sym, uint32_t index[], int idxlevel);
    
  public:
    Runmode runmode();
    void SetRunmode(Runmode runmode);
    cell_t lastframe();
    void SetLastFrame(cell_t lastfrm);
    uint32_t lastline();
    void SetLastLine(uint32_t line);
    const char *currentfile();
    void SetCurrentFile(const char *file);
    
  public:
    void HandleInput(cell_t cip, int bp, uint32_t line);
    void ListCommands(char *command);
    
  private:
    PluginContext *context_;
    Runmode runmode_;
    cell_t lastfrm_;
    uint32_t lastline_;
    const char *currentfile_;
    bool active_;

    struct BreakpointMapPolicy {

      static inline uint32_t hash(ucell_t value) {
        return ke::HashInteger<4>(value);
      }

      static inline bool matches(ucell_t a, ucell_t b) {
        return a == b;
      }
    };
    typedef ke::HashMap<ucell_t, Breakpoint *, BreakpointMapPolicy> BreakpointMap;

    BreakpointMap breakpoint_map_;
    
    struct WatchTablePolicy {
      typedef AString Payload;
      
      static uint32_t hash(const char *str) {
        return ke::HashCharSequence(str, strlen(str));
      }
      
      static bool matches(const char *key, Payload str) {
        return str.compare(key) == 0;
      }
    };
    typedef ke::HashTable<WatchTablePolicy> WatchTable;
    
    WatchTable watch_table_;
  };

} // namespace sp
#endif	/* _include_sourcepawn_vm_debugger_h_ */

