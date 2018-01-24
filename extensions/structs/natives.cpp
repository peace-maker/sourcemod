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

#include "extension.h"

//native GetStructInt(Handle:struct, const String:member[]);
static cell_t GetStructInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	int value;

	if (!pHandle->GetInt(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return value;
}

//native SetStructInt(Handle:struct, const String:member[], value);
static cell_t SetStructInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = handlesys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	if (!pHandle->SetInt(member, params[3]))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructFloat(Handle:struct, const String:member[], &Float:value);
static cell_t GetStructFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	float value;

	if (!pHandle->GetFloat(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return sp_ftoc(value);
}	

//native SetStructFloat(Handle:struct, const String:member[], Float:value);
static cell_t SetStructFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	if (!pHandle->SetFloat(member, sp_ctof(params[3])))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructVector(Handle:struct, const String:member[], Float:vec[3]);
static cell_t GetStructVector(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	Vector value;

	if (!pHandle->GetVector(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	addr[0] = sp_ftoc(value.x);
	addr[1] = sp_ftoc(value.y);
	addr[2] = sp_ftoc(value.z);

	return 1;
}

//native SetStructVector(Handle:struct, const String:member[], Float:vec[3]);
static cell_t SetStructVector(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	Vector value;

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	value.x = sp_ctof(addr[0]);
	value.y = sp_ctof(addr[1]);
	value.z = sp_ctof(addr[2]);

	if (!pHandle->SetVector(member, value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructString(Handle:struct, const String:member[], String:value[], maxlen);
static cell_t GetStructString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	char *string;
	if (!pHandle->GetString(member, &string))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	pContext->StringToLocal(params[3], params[4], string);

	return 1;
}

//native SetStructString(Handle:struct, const String:member[], String:value[]);
static cell_t SetStructString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	char *string;
	pContext->LocalToString(params[3], &string);

	if (!pHandle->SetString(member, string))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructEnt(Handle:struct, const String:member[], &ent);
static cell_t GetStructEnt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	edict_t *pEnt;
	if (!pHandle->GetEHandle(member, &pEnt))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	if (pEnt == NULL)
	{
		return -1;
	}

	return gamehelpers->IndexOfEdict(pEnt);
}

//native SetStructEnt(Handle:struct, const String:member[], ent);
static cell_t SetStructEnt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	edict_t *pEdict = gamehelpers->EdictOfIndex(params[3]);

	if (pEdict == NULL)
	{
		return pContext->ThrowNativeError("Invalid entity %i", params[3]);
	}

	if (!pHandle->SetEHandle(member, pEdict))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

const sp_nativeinfo_t MyNatives[] = 
{
	{"GetStructInt",	GetStructInt},
	{"SetStructInt",	SetStructInt},
	{"GetStructFloat",	GetStructFloat},
	{"SetStructFloat",	SetStructFloat},
	{"GetStructVector",	GetStructVector},
	{"SetStructVector",	SetStructVector},
	{"GetStructString",	GetStructString},
	{"SetStructString",	SetStructString},
	{"GetStructEnt",	GetStructEnt},
	{"SetStructEnt",	SetStructEnt},
	{NULL,				NULL},
};
