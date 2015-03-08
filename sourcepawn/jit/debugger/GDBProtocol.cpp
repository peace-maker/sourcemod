// vim: set ts=8 sts=2 sw=2 tw=99 et:
#include "GDBProtocol.h"
#include "serversock.h"
#include <sstream>
#include <iomanip>

static const char hexchars[] = "0123456789abcdef";

static const string gdb_sourcepawn_xml = 
"<?xml version=\"1.0\"?>"
"<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
"<feature name = \"org.gnu.gdb.sourcepawn.core\">"
"  <reg name = \"pri\" bitsize = \"32\" type = \"int32\" />"
"  <reg name = \"alt\" bitsize = \"32\" type = \"int32\" />"
"  <reg name = \"cip\" bitsize = \"32\" type = \"code_ptr\" />"
"  <reg name = \"sp\" bitsize = \"32\" type = \"data_ptr\" />"
"  <reg name = \"frm\" bitsize = \"32\" type = \"data_ptr\" />"
"</feature>";

/* Convert ch from a hex digit to an int */
static int
hex(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

GDBProtocol::GDBProtocol(ServerThread *thread, ServerSock *sock)
	: m_sock(sock),
	m_thread(thread),
	m_inCommand(false),
	m_numCheckChars(0),
	m_checksum(0),
	m_xmitcsum(0),
	m_outWait(false),
	m_noAckMode(false)
{
}

void GDBProtocol::parse(string data)
{
	size_t offset = 0;
	while (true)
	{
		if (data == "")
			break;

		printf("GDB debug recv %s\n", data.c_str());

		// We're waiting for the confirmation of our last packet.
		// Not confirmed yet. Resend.
		if (m_outWait && data[0] == '+')
		{
			// We got our confirmation.
			m_outWait = false;
			data = data.substr(1, string::npos);
			continue;
		}
		else if (data[0] == '-')
		{
			putpacket(m_outbuf);
			data = data.substr(1, string::npos);
			continue;
		}
	
		string packet = getpacket(data, &offset);
		data = data.substr(offset, string::npos);
		// Keep parsing until we found a whole packet.
		if (packet == "")
			continue;

		handlepacket(packet);
	}
}

void GDBProtocol::handlepacket(string packet)
{
	string ret;
	std::stringstream sstr;

	printf("handling packet: %s\n", packet.c_str());

	try
	{
		switch (packet[0])
		{
		case '?':
			ret.append("S00");
			break;
		case 'g': // return the value of the CPU registers
			for (int i = 0; i < 5; i++)
			{
				sstr << std::setfill('0') << std::setw(8) << std::hex << m_registers[i];
			}
			ret.append(sstr.str());
			break;
		case 'q': // general query
			if (packet.find("qSupported:") == 0)
			{
				if (packet.find("multiprocess") != string::npos)
					ret.append("multiprocess-;");
				ret.append("PacketSize=1024;");
				ret.append("QStartNoAckMode+;");
				ret.append("xmlRegisters=;");
				ret.append("qXfer:features:read+;");
			}
			else if (packet == "qC") // Return the current thread ID
			{
				ret.append("QC0"); // We don't have threads, so always return 'any' (0)
			}
			else if (packet == "qAttached") // Return an indication of whether the remote server attached to an existing process or created a new process. 
			{
				ret.append("1"); // The remote server attached to an existing process. 
			}
			else if (packet.find("qXfer:libraries:read:") == 0) // qXfer:features:read:annex:offset,length
			{
				int offset = packet.find("target.xml:");
				if (offset == string::npos)
				{
					throw GDBProtocolException("E00");
				}
				
				string params = packet.substr(offset + 11, string::npos);
				offset = atoi(params.c_str());
				if (params.find_first_of(',') == string::npos)
				{
					throw GDBProtocolException("E00"); // The request was malformed, or annex was invalid.
				}
				
				int length = atoi(params.c_str());
				int str_len = gdb_sourcepawn_xml.length();

				if (offset < 0 || length < 0)
				{
					throw GDBProtocolException("E01"); // The offset was invalid, or there was an error encountered reading the data. The nn part is a hex-encoded errno value.
				}

				if (offset >= str_len)
				{
					throw GDBProtocolException("l"); // The offset in the request is at the end of the data. There is no more data to be read. 
				}

				if (offset + length > str_len)
				{
					length = str_len - offset;
					ret.append("l");
				}
				else
				{
					ret.append("m");
				}

				ret.append(gdb_sourcepawn_xml.substr(offset, length));
			}
			break;
		case 'Q':
			if (packet == "QStartNoAckMode")
			{
				ret.append("OK");
				m_noAckMode = true;
			}

			break;
		case 'H': // Set thread for subsequent operations
			ret.append("OK"); // We're single threaded.
			break;
		}
	}
	catch (GDBProtocolException& e)
	{
		ret.append(e.getMsg());
	}

	// reply to the request
	putpacket(ret);
}

string GDBProtocol::getpacket(string data, size_t *offset)
{
	size_t commandStart = 0;
	bool isEscaped = false;
	
	// Only search for a $ if we're not currently parsing a command.
	if (!m_inCommand)
	{
		// A packet starts with a '$'
		commandStart = data.find_first_of('$');
		if (commandStart == string::npos)
		{
			*offset = data.length();
			return "";
		}
	}

	// We are past that $
	m_inCommand = true;

	std::string::iterator it = data.begin();

	// Skip to the first '$', ignore all other characters
	it += commandStart;
	*offset = commandStart;

	// We aren't in the process of validating the checksum. Read the chars normally.
	if (!m_numCheckChars)
	{
		// now, read until a # or end of buffer is found
		for (; it != data.end(); ++it)
		{
			*offset += 1;
			if (*it == '$')
			{
				m_inbuf.clear();
				m_inbuf.assign("");
				m_checksum = 0;
			}
			else if (*it == '#')
			{
				break;
			}
			else
			{
				m_checksum += (unsigned char)*it;
				// This is the escape character. ignore it and unescape the next one.
				if (*it == ESCAPE_CHAR)
				{
					isEscaped = true;
					continue;
				}

				// This char is escaped.
				if (isEscaped)
				{
					isEscaped = false;
					// Unescape by XOR'ing with 0x20
					m_inbuf.push_back(*it ^ 0x20);
				}
				else
				{
					m_inbuf.push_back(*it);
				}
			}
		}
	}

	// We're at the checksum now or already were (split packets)
	if (*it == '#' || m_numCheckChars > 0)
	{
		++it;
		for (; it != data.end() && m_numCheckChars < 2; ++it, m_numCheckChars++)
		{
			*offset += 1;
			// Shift the first byte up
			if (m_numCheckChars == 0)
				m_xmitcsum = hex(*it) << 4;
			else
				m_xmitcsum += hex(*it);
		}
		
		// We don't have the whole checksum yet. Wait for another package.
		if (m_numCheckChars != 2)
			return "";

		if (m_checksum != m_xmitcsum)
		{
			if (!m_noAckMode)
				*m_sock << "-"; // failed checksum
			resetInputState();
		}
		else
		{
			if (!m_noAckMode)
				*m_sock << "+"; // successful transfer
			resetInputState();

			// if a sequence char is present, reply the sequence ID
			if (m_inbuf.length() > 3 && m_inbuf[2] == ':')
			{
				*m_sock << m_inbuf.substr(0, 2);
				return m_inbuf.substr(3, string::npos);
			}
			return m_inbuf;
		}
	}
	return "";
}

void GDBProtocol::resetInputState()
{
	// Reset our parsing state.
	m_checksum = 0;
	m_xmitcsum = 0;
	m_inCommand = false;
	m_numCheckChars = 0;
}

void GDBProtocol::putpacket(string buffer)
{
	m_outbuf = buffer;

	unsigned char checksum = 0;

	// escape the '*', so it's not interpreted as the start of a run-length encoded sequence.
	size_t pos;
	while ((pos = buffer.find_first_of('*')) != string::npos)
	{
		buffer.replace(pos, 1, "\x2a\xa");
	}

	// $<packet info>#<checksum>
	string tmp;
	stringstream sstr(tmp);
	sstr << "$" << buffer << "#";

	// Calculate the checksum
	for (std::string::iterator it = buffer.begin(); it != buffer.end(); ++it)
	{
		checksum += *it;
	}

	sstr << hexchars[checksum >> 4] << hexchars[checksum & 0xf];

	printf("GDB debug send: %s\n", sstr.str().c_str());

	*m_sock << sstr.str();
	if (!m_noAckMode)
		m_outWait = true;
}

void GDBProtocol::OnDebugBreakHalt(BaseContext *ctx, cell_t frm, cell_t cip, cell_t stk, cell_t pri, cell_t alt)
{
	m_basectx = ctx;
	m_registers[0] = pri;
	m_registers[1] = alt;
	m_registers[2] = cip;
	m_registers[3] = stk;
	m_registers[4] = frm;

	/*std::stringstream sstr;
	sstr << "T02";
	for (int i = 0; i < 5; i++)
	{
		sstr << "0" << i << ":" << std::setfill('0') << std::setw(8) << std::hex << m_registers[i] << ";";
	}
	putpacket(sstr.str());*/
}