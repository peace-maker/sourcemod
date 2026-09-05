/**
* =============================================================================
* DynamicHooks-x86_64
* Copyright (C) 2024 Benoist "Kenzzer" André. All rights reserved.
* Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
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
*/

#ifndef _X86_64_SYSTEMV_DEFAULT_H
#define _X86_64_SYSTEMV_DEFAULT_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "../convention.h"

// ============================================================================
// >> CLASSES
// ============================================================================
class x86_64SystemVDefault : public ICallingConvention
{
public:
	x86_64SystemVDefault(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment = 8);
	virtual ~x86_64SystemVDefault();

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

#endif //_X86_64_SYSTEMV_DEFAULT_H