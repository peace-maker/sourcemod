// vim: set ts=8 sw=2 sts=2 tw=99 et:
#ifndef _INCLUDE_SOURCEPAWN_DEBUGGER_H_
#define _INCLUDE_SOURCEPAWN_DEBUGGER_H_

#include <ServerThread.h>

class BaseContext;
int GlobalDebugBreak(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt);

class SPDebugger
{
public:
	SPDebugger();
	bool StartDebugger(int port);
	bool StopDebugger();
	int GetPort();
	ServerThread *GetServerThread();

private:
	int m_iPort;
	ke::AutoPtr<ServerThread> m_pServerThread;
	ke::AutoPtr<ke::Thread> m_thread;
};

#endif // _INCLUDE_SOURCEPAWN_DEBUGGER_H_