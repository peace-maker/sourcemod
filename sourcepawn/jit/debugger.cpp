// vim: set ts=8 sts=2 sw=2 tw=99 et:
//
// This file is part of SourcePawn.
// 
// SourcePawn is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// SourcePawn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with SourcePawn.  If not, see <http://www.gnu.org/licenses/>.

#include "debugger.h"
#include <ServerThread.h>
#include "sp_vm_basecontext.h"
#include <am-thread-utils.h>
#include <am-utility.h>
#include "engine2.h"

SPDebugger::SPDebugger() 
	: m_iPort(-1)
{
}

int SPDebugger::GetPort()
{
	return m_iPort;
}

ServerThread *SPDebugger::GetServerThread()
{
	return m_pServerThread;
}

bool SPDebugger::StartDebugger(int port)
{
	m_iPort = port;

	m_pServerThread = new ServerThread(m_iPort);
	m_thread = new ke::Thread(m_pServerThread, "JitDebugger");
	if (!m_thread->Succeeded())
	{
		m_iPort = -1;
		return false;
	}

	return true;
}

bool SPDebugger::StopDebugger()
{
	if (m_iPort < 0)
		return false;

	m_pServerThread->Stop();
	m_thread->Join();

	m_iPort = -1;

	return true;
}

// Always call the correct function
int GlobalDebugBreak(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt)
{
	SPVM_DEBUGBREAK dbgbreak = ctx->GetCtx()->plugin->dbreak;
	if (dbgbreak != NULL)
		dbgbreak(ctx, frm, cip);

	// Handle remote debug breakpoints
	if (g_engine2.GetRemoteDebugPort() > 0)
	{
		SPDebugger *dbg = g_engine2.GetDebugger();
		dbg->GetServerThread()->OnDebugBreakHalt(ctx, frm, cip, stk, pri, alt);
	}

	return SP_ERROR_NONE;
}