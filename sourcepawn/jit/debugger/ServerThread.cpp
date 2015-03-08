#include "ServerThread.h"
#include <serversock.h>
#include "GDBProtocol.h"

ServerThread::ServerThread(int port)
	: m_iPort(port),
	m_bStop(false),
	m_pCurrentChild(NULL),
	m_pServerSocket(NULL),
	m_pCurrentGDBProtocol(NULL),
	m_bExecutionStopped(false)
{
}

void ServerThread::Stop()
{
	m_bStop = true;
	if (m_pServerSocket != NULL)
		m_pServerSocket->close();
	if (m_pCurrentChild != NULL)
		m_pCurrentChild->close();
}

void ServerThread::Run()
{
	try
	{
		m_pServerSocket = new ServerSock(m_iPort);
		while (!m_bStop)
		{
			m_pCurrentChild = new ServerSock();
			m_pServerSocket->accept(*m_pCurrentChild);
			try
			{
				m_pCurrentGDBProtocol = new GDBProtocol(this, m_pCurrentChild);
				while (!m_bStop)
				{
					string data;
					*m_pCurrentChild >> data;
					m_pCurrentGDBProtocol->parse(data);
				}
			}
			catch (SockExcept& e)
			{
				printf("Error in childsocket: %s", e.get_SockExcept().c_str());
				Continue();
			}
			delete m_pCurrentGDBProtocol;
			m_pCurrentGDBProtocol = NULL;
			if (!m_bStop)
				m_pCurrentChild->close();
			delete m_pCurrentChild;
			m_pCurrentChild = NULL;
		}
	}
	catch (SockExcept& e)
	{
		printf("Error in serversocket: %s", e.get_SockExcept().c_str());
		Continue();
	}
	if (!m_bStop)
		m_pServerSocket->close();
	delete m_pServerSocket;
	m_pServerSocket = NULL;
}

void ServerThread::OnDebugBreakHalt(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt)
{
	if (m_pCurrentGDBProtocol == NULL)
		return;

	m_bExecutionStopped = true;
	//ctx->GetRuntime()->SetPauseState(true);
	m_pCurrentGDBProtocol->OnDebugBreakHalt(ctx, frm, cip, stk, pri, alt);
	while (m_bExecutionStopped)
	{
		Sleep(100);
	}
}

void ServerThread::Continue()
{
	m_bExecutionStopped = false;
}