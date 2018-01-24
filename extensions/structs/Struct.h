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

#ifndef _INCLUDE_SOURCEMOD_STRUCT_H_
#define _INCLUDE_SOURCEMOD_STRUCT_H_

#include "sm_trie_tpl.h"
#include "sh_list.h"
#include "IStructs.h"

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

using namespace SourceMod;

class StructInfo : public IStructInfo
{
public:
	StructInfo();
	~StructInfo();
	void SetName(const char *name);
	const char *GetName();
	void AddMember(const char *name, MemberInfo *member);
	MemberInfo *FindMember(const char *name);
	void SetStructSize(unsigned int size);

	bool getOffset(const char *member, int *offset);
	bool getType(const char *member, MemberType*type);
	bool getSize(const char *member, int *size);
	bool getIndirection(const char *member, int *level);

private:
	char name[100];
	int size;
	KTrie<MemberInfo *> members;
	SourceHook::List<MemberInfo *> membersList;
};

#endif //_INCLUDE_SOURCEMOD_STRUCT_H_
