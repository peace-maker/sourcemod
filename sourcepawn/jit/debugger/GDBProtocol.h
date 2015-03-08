#ifndef _INCLUDE_SOURCEPAWN_DEBUGGER_GDB_H_
#define _INCLUDE_SOURCEPAWN_DEBUGGER_GDB_H_

#include <string>
#include <sp_vm_basecontext.h>

class ServerSock;
class ServerThread;

using namespace std;

class GDBProtocol
{
public:
	const unsigned char ESCAPE_CHAR = 0x7d;

public:
	GDBProtocol(ServerThread *thread, ServerSock *sock);
	void parse(string data);
	void OnDebugBreakHalt(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt);
private:
	string getpacket(string data, size_t *offset);
	void resetInputState();
	void putpacket(string buffer);
	void handlepacket(string packet);
private:
	ServerSock *m_sock;
	ServerThread *m_thread;
	
	// Input parsing
	bool m_inCommand;
	unsigned char m_numCheckChars;
	unsigned char m_checksum;
	unsigned char m_xmitcsum;
	string m_inbuf;

	// Output handling
	string m_outbuf;
	bool m_outWait;

	// GDB agreed not to send +/- acks?
	bool m_noAckMode;

	// Current state of program
	BaseContext *m_basectx;
	cell_t m_registers[5];
};

class GDBProtocolException {
public:
	GDBProtocolException(string s) : except(s) {};
	~GDBProtocolException() {};
	string getMsg() { return except; }

private:
	string except;
};

#endif // _INCLUDE_SOURCEPAWN_DEBUGGER_GDB_H_