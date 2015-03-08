/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Remote Debugger Extension
* Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*
* As a special exception, AlliedModders LLC gives you permission to link the
* code of this program (as well as its derivative works) to "Half-Life 2," the
* "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
* by the Valve Corporation.  You must obey the GNU General Public License in
* all respects for all other code used.  Additionally, AlliedModders LLC grants
* this exception to all derivative works.  AlliedModders LLC defines further
* exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
* or <http://www.sourcemod.net/license.php>.
*
* Version: $Id$
*/

#ifndef _INCLUDE_SOURCEPAWN_SERVERTHREAD_H_
#define _INCLUDE_SOURCEPAWN_SERVERTHREAD_H_

#include <am-thread-utils.h>
#include <sp_vm_basecontext.h>

class ServerSock;
class GDBProtocol;

class ServerThread : 
	public ke::IRunnable
{
public:
	ServerThread(int port);
	void Stop();
	void OnDebugBreakHalt(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt);
	void Continue();

public: // ke::IRunnable
	void Run();

private:
	int m_ServerSocket;
	int m_iPort;
	bool m_bStop;
	ServerSock *m_pServerSocket;
	ServerSock *m_pCurrentChild;
	GDBProtocol *m_pCurrentGDBProtocol;
	bool m_bExecutionStopped;
};

#endif // _INCLUDE_SOURCEPAWN_SERVERTHREAD_H_