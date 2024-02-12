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
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*
* Adopted to provide similar features to SourceHook by AlliedModders LLC.
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x64SystemV.h"
#include <string.h>


// ============================================================================
// >> x64SystemV
// ============================================================================
x64SystemV::x64SystemV(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	ICallingConvention(vecArgTypes, returnType, iAlignment)
{
	if (m_returnType.size > 8)
	{
		m_pReturnBuffer = malloc(m_returnType.size);
	}
	else
	{
		m_pReturnBuffer = NULL;
	}
}

x64SystemV::~x64SystemV()
{
	if (m_pReturnBuffer)
	{
		free(m_pReturnBuffer);
	}
}

std::vector<Register_t> x64SystemV::GetRegisters()
{
	std::vector<Register_t> registers;

	registers.push_back(RSP);

	if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
	{
		registers.push_back(XMM0);
        if (m_pReturnBuffer)
        {
            registers.push_back(XMM1);
        }
	}
	else
	{
		registers.push_back(RAX);
		if (m_pReturnBuffer)
		{
			registers.push_back(RDX);
		}
	}

    // First six integer parameters are passed in rdi, rsi, rdx, rcx, r8, r9
    // First eight floating point parameters are passed in xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    size_t floatArgCount = 0;
    size_t intArgCount = 0;
    for (size_t i = 0; i < m_vecArgTypes.size(); i++)
    {
        // Save all the custom calling convention registers as well.
        if (m_vecArgTypes[i].custom_register != None)
            registers.push_back(m_vecArgTypes[i].custom_register);

        else if (m_vecArgTypes[i].type == DATA_TYPE_FLOAT || m_vecArgTypes[i].type == DATA_TYPE_DOUBLE)
        {
            if (floatArgCount < 8)
            {
                registers.push_back(m_floatArgRegisters[floatArgCount]);
            }
            floatArgCount++;
        }
        else
        {
            if (intArgCount < 6)
            {
                registers.push_back(m_intArgRegisters[intArgCount]);
            }
            intArgCount++;
        }
    }

    // TODO: Make sure the list is unique? Set?
	return registers;
}

int x64SystemV::GetPopSize()
{
	return 0;
}

int x64SystemV::GetArgStackSize()
{
	int iArgStackSize = 0;
    size_t iIntArgCount = 0;
    size_t iFloatArgCount = 0;
	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register != None)
            continue;

        if (m_vecArgTypes[i].type == DATA_TYPE_FLOAT || m_vecArgTypes[i].type == DATA_TYPE_DOUBLE)
        {
            iFloatArgCount++;
            if (iFloatArgCount > 8)
                iArgStackSize += m_vecArgTypes[i].size;
            continue;
        }

        iIntArgCount++;
        if (iIntArgCount > 6)
            iArgStackSize += m_vecArgTypes[i].size;
	}

	return iArgStackSize;
}

void** x64SystemV::GetStackArgumentPtr(CRegisters* pRegisters)
{
	return (void **)(pRegisters->m_rsp->GetValue<unsigned long>() + sizeof(void *));
}

int x64SystemV::GetArgRegisterSize()
{
	int iArgRegisterSize = 0;
    size_t iIntArgCount = 0;
    size_t iFloatArgCount = 0;
	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register != None)
            iArgRegisterSize += m_vecArgTypes[i].size;

        if (m_vecArgTypes[i].type == DATA_TYPE_FLOAT || m_vecArgTypes[i].type == DATA_TYPE_DOUBLE)
        {
            if (iFloatArgCount < 8)
                iArgRegisterSize += m_vecArgTypes[i].size;
            iFloatArgCount++;
            continue;
        }

        if (iIntArgCount < 6)
            iArgRegisterSize += m_vecArgTypes[i].size;
        iIntArgCount++;
	}

	return iArgRegisterSize;
}

void* x64SystemV::GetArgumentPtr(unsigned int iIndex, CRegisters* pRegisters)
{
	if (iIndex >= m_vecArgTypes.size())
		return NULL;

	// Check if this argument was passed in a register.
	if (m_vecArgTypes[iIndex].custom_register != None)
	{
		CRegister *pRegister = pRegisters->GetRegister(m_vecArgTypes[iIndex].custom_register);
		if (!pRegister)
			return NULL;

		return pRegister->m_pAddress;
	}

	// Skip return address.
	size_t iOffset = 8;
    size_t iIntArgCount = 0;
    size_t iFloatArgCount = 0;
	for(unsigned int i=0; i < iIndex; i++)
	{
		if (m_vecArgTypes[i].custom_register != None)
			continue;

        if (m_vecArgTypes[i].type == DATA_TYPE_FLOAT || m_vecArgTypes[i].type == DATA_TYPE_DOUBLE)
        {
            iFloatArgCount++;
            if (iFloatArgCount > 8)
                iOffset += m_vecArgTypes[i].size;
        }
        else
        {
            iIntArgCount++;
            if (iIntArgCount > 6)
                iOffset += m_vecArgTypes[i].size;
        }
	}

    if (m_vecArgTypes[iIndex].type == DATA_TYPE_FLOAT || m_vecArgTypes[iIndex].type == DATA_TYPE_DOUBLE)
    {
        if (iFloatArgCount < 8)
        {
            return (void *) pRegisters->GetRegister(m_floatArgRegisters[iFloatArgCount])->m_pAddress;
        }
    }
    else
    {
        if (iIntArgCount < 6)
        {
            return (void *) pRegisters->GetRegister(m_intArgRegisters[iIntArgCount])->m_pAddress;
        }
    }

	return (void *) (pRegisters->m_rsp->GetValue<unsigned long>() + iOffset);
}

void x64SystemV::ArgumentPtrChanged(unsigned int iIndex, CRegisters* pRegisters, void* pArgumentPtr)
{
}

void* x64SystemV::GetReturnPtr(CRegisters* pRegisters)
{
	if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
		return pRegisters->m_xmm0->m_pAddress;

	if (m_pReturnBuffer)
	{
		// First half in rax, second half in rdx
		memcpy(m_pReturnBuffer, pRegisters->m_rax, 8);
		memcpy((void *) ((unsigned long) m_pReturnBuffer + 8), pRegisters->m_rdx, 8);
		return m_pReturnBuffer;
	}

	return pRegisters->m_rax->m_pAddress;
}

void x64SystemV::ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr)
{
	if (m_pReturnBuffer)
	{
		// First half in rax, second half in rdx
		memcpy(pRegisters->m_rax, m_pReturnBuffer, 8);
		memcpy(pRegisters->m_rdx, (void *) ((unsigned long) m_pReturnBuffer + 8), 8);
	}
}