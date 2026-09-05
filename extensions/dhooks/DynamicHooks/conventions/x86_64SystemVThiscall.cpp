/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* Copyright (C) 2026 AlliedModders LLC.  All rights reserved.
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
#include "x86_64SystemVThiscall.h"


// ============================================================================
// >> CLASSES
// ============================================================================

x86_64SystemVThiscall::x86_64SystemVThiscall(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	x86_64SystemVDefault(vecArgTypes, returnType, iAlignment)
{
	// Always add the |this| pointer.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	type.size = GetDataTypeSize(type, iAlignment);
	type.custom_register = None;
	m_vecArgTypes.insert(m_vecArgTypes.begin(), type);
}

x86_64SystemVThiscall::~x86_64SystemVThiscall()
{
}

int x86_64SystemVThiscall::GetArgStackSize()
{
	// Remove the this pointer from the arguments size.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	return x86_64SystemVDefault::GetArgStackSize() - GetDataTypeSize(type, m_iAlignment);
}

void** x86_64SystemVThiscall::GetStackArgumentPtr(CRegisters* pRegisters)
{
	// Skip return address and this pointer.
	DataTypeSized_t type;
	type.type = DATA_TYPE_POINTER;
	return (void **)(pRegisters->m_rsp->GetValue<uintptr_t>() + 8 + GetDataTypeSize(type, m_iAlignment));
}

void x86_64SystemVThiscall::SaveCallArguments(CRegisters* pRegisters)
{
	// Count the this pointer.
	int size = x86_64SystemVDefault::GetArgStackSize() + GetArgRegisterSize();
	std::unique_ptr<uint8_t[]> pSavedCallArguments = std::make_unique<uint8_t[]>(size);
	size_t offset = 0;
	for (size_t i = 0; i < m_vecArgTypes.size(); i++) {
		DataTypeSized_t &type = m_vecArgTypes[i];
		memcpy((void *)((uintptr_t)pSavedCallArguments.get() + offset), GetArgumentPtr(i, pRegisters), type.size);
		offset += type.size;
	}
	m_pSavedCallArguments.push_back(std::move(pSavedCallArguments));
}
