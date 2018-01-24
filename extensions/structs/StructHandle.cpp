/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Sample Extension
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

#include "StructHandle.h"
#include "extension.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

bool StructHandle::GetInt(const char *member, int *value)
{
	int offset;
	
	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Int)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	int size;
	if (!info->getSize(member, &size))
	{
		return false;
	}

	uint32 *location = (uint32 *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<uint32 **>(location);
	}

	switch (size)
	{
		case 1:
		{
			*value = *(uint8 *)location;
			break;
		}

		case 2:
		{
			*value = *(uint16 *)location;
			break;
		}

		case 4:
		{
			*value = *(uint32 *)location;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::GetFloat(const char *member, float *value)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Float)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	float *location = (float *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<float **>(location);
	}

	*value = *location;

	return true;
}

bool StructHandle::SetInt(const char *member, int value)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Int)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	int size;
	if (!info->getSize(member, &size))
	{
		return false;
	}

	uint32 *location = (uint32 *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<uint32 **>(location);
	}

	switch (size)
	{
		case 1:
		{
			*(uint8 *)location = value;
			break;
		}

		case 2:
		{
			*(uint16 *)location = value;
			break;
		}

		case 4:
		{
			*(uint32 *)location = value;
			break;
		}

		default:
		{
			return false;
		}
	}

	return true;
}

bool StructHandle::SetFloat(const char *member, float value)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Float)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	float *location = (float *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<float **>(location);
	}

	*location = value;

	return true;
}

bool StructHandle::IsPointerNull(const char *member, bool *result)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection) || indirection == 0)
	{
		return false;
	}

	unsigned char **location = (unsigned char **)data + offset;

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<unsigned char ***>(location);
	}

	*result = (*location == NULL);

	return true;
}

bool StructHandle::SetPointerNull(const char *member)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection) || indirection == 0)
	{
		return false;
	}

	unsigned char **location = (unsigned char **)data + offset;

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<unsigned char ***>(location);
	}

	*location = NULL;

	return true;
}

bool StructHandle::GetVector(const char *member, Vector *vec)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Vector)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	Vector *location = (Vector *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<Vector **>(location);
	}

	*vec = *location;

	return true;
}

bool StructHandle::SetVector(const char *member, Vector vec)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Vector)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	Vector *location = (Vector *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<Vector **>(location);
	}

	*location = vec;

	return true;
}

bool StructHandle::GetString(const char *member, char **string)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Char)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	char *location = (char *)((unsigned char *)data + offset);

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<char **>(location);
	}

	*string = location;

	return true;
}

bool StructHandle::SetString(const char *member, char *string)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_Char)
	{
		return false;
	}

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	int size;
	info->getSize(member, &size);
	char *location = (char *)data + offset;

	for(int i = 0; i < indirection; i++)
	{
		location = *reinterpret_cast<char **>(location);
	}
	
	UTIL_Format(location, size, "%s", string);

	return true;
}

bool StructHandle::GetEHandle(const char *member, edict_t **pEnt)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_EHandle)
	{
		return false;
	}

	CBaseHandle *pHandle = (CBaseHandle *)((unsigned char *)data + offset);

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	for(int i = 0; i < indirection; i++)
	{
		pHandle = *reinterpret_cast<CBaseHandle **>(pHandle);
	}

	*pEnt = gamehelpers->GetHandleEntity(*pHandle);

	return true;
}

bool StructHandle::SetEHandle(const char *member, edict_t *pEnt)
{
	int offset;

	if (!info->getOffset(member, &offset))
	{
		return false;
	}

	MemberType type;

	if (!info->getType(member, &type) || type != Member_EHandle)
	{
		return false;
	}

	CBaseHandle *pHandle = (CBaseHandle *)((unsigned char *)data + offset);

	int indirection;
	if (!info->getIndirection(member, &indirection))
	{
		return false;
	}

	for(int i = 0; i < indirection; i++)
	{
		pHandle = *reinterpret_cast<CBaseHandle **>(pHandle);
	}

	gamehelpers->SetHandleEntity(*pHandle, pEnt);

	return true;
}
