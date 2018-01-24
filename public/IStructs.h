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

#ifndef _INCLUDE_SOURCEMOD_ISTRUCTS_H
#define _INCLUDE_SOURCEMOD_ISTRUCTS_H

#include <IShareSys.h>
#include <IHandleSys.h>

#define SMINTERFACE_STRUCTS_NAME	"IStructs"
#define SMINTERFACE_STRUCTS_VERSION	2


namespace SourceMod
{
	enum MemberType
	{
		Member_Unknown,
		Member_Int,
		Member_Float,
		Member_Char,
		Member_Vector,
		Member_EHandle,
	};

	class MemberInfo
	{
	public:
		MemberInfo()
		{
			name[0] = 0;
			size = -1;
			type = Member_Unknown;
			offset = -1;
			indirection_level = 0;
		}

		char name[100];
		MemberType type;
		int size;
		int indirection_level;
		int offset;
	};

	class IStructInfo
	{
	public:
		virtual const char *GetName() = 0;
		virtual void AddMember(const char *name, MemberInfo *member) = 0;
		virtual void SetStructSize(unsigned int size) = 0;
		virtual bool getOffset(const char *member, int *offset) = 0;
		virtual bool getType(const char *member, MemberType *type) = 0;
		virtual bool getSize(const char *member, int *size) = 0;
		virtual bool getIndirection(const char *member, int *level) = 0;
	};

	class IStructs : public SMInterface
	{
	public:
		virtual Handle_t CreateStructHandle(const char *type, void *str) = 0;
		virtual void AddStruct(const char *name, IStructInfo *str) = 0;
		virtual IStructInfo *CreateStruct(const char *name) = 0;
		virtual IStructInfo *FindStruct(const char *name) = 0;
	};
}

#endif //_INCLUDE_SOURCEMOD_ISTRUCTS_H
