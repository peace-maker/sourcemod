/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* Copyright (C) 2018-2024 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* x86 version by Robin Gohmert.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*
* Adopted to provide similar features to SourceHook by AlliedModders LLC.
*/

#ifndef _X64_SYSTEMV_H
#define _X64_SYSTEMV_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "../convention.h"


// ============================================================================
// >> CLASSES
// ============================================================================
/*
Source: DynCall manual and Windows docs

Registers:
	- rax = return value
	- rdx = return value
	- rsp = stack pointer
	- xmm0 = floating point return value
	- xmm1 = floating point return value

Parameter passing:
    - first six integer parameters are passed in rdi, rsi, rdx, rcx, r8, r9
    - first eight floating point parameters are passed in xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
	- rest is passed on the stack. parameter order: right-to-left
	- caller cleans up the stack
	- alignment: 8 bytes

Return values:
	- return values of pointer or intergral type (<= 64 bits) are returned via the rax register
	- integers > 64 bits are returned via the rax and rdx registers
	- floating point types are returned via the xmm0 and xmm1 registers
*/
class x64SystemV: public ICallingConvention
{	
public:
	x64SystemV(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment=8);
	virtual ~x64SystemV();

	virtual std::vector<Register_t> GetRegisters();
	virtual int GetPopSize();
	virtual int GetArgStackSize();
	virtual void** GetStackArgumentPtr(CRegisters* pRegisters);
	virtual int GetArgRegisterSize();
	
	virtual void* GetArgumentPtr(unsigned int iIndex, CRegisters* pRegisters);
	virtual void ArgumentPtrChanged(unsigned int iIndex, CRegisters* pRegisters, void* pArgumentPtr);

	virtual void* GetReturnPtr(CRegisters* pRegisters);
	virtual void ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr);

private:
	void* m_pReturnBuffer;
    Register_t m_intArgRegisters[6] = { RDI, RSI, RDX, RCX, R8, R9 };
    Register_t m_floatArgRegisters[8] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7 };
};


#endif // _X64_SYSTEMV_H