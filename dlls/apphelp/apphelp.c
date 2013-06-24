/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Andrew Cook
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * Application Shim Database:
 * 
 * Header consists of two DWORD's, followed by "sdbf"
 * 
 * 'Tags' are 2 byte identifiers followed by their data
 * their format type is defined by the first nibble, following 3 carry
 * other type information. the TAG_TYPE_ macro's map type to names
 * 
 * All tags are word aligned
 * 
 * STRINGREF's and other tag refferences go by 'Tag id' which is the
 * offset of that tag within the file
 * 
 **/

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include <appcompatapi.h>

#include "wine/debug.h"

#include "sdb.h"

WINE_DEFAULT_DEBUG_CHANNEL(apphelp);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

BOOL WINAPI ApphelpCheckInstallShieldPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

BOOL WINAPI ApphelpCheckMsiPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

BOOL WINAPI ApphelpCheckShellObject( REFCLSID clsid, BOOL shim, ULONGLONG *flags )
{
    TRACE("(%s, %d, %p)\n", debugstr_guid(clsid), shim, flags);
    if (flags) *flags = 0;
    return TRUE;
}

#include "pshpack1.h"

typedef struct _SdbTag {
    TAG tag;
    union {
        WORD word;
        DWORD dword;
        DWORD length;
        ULONGLONG qword;
        struct {
            DWORD length;
            WCHAR str[];
        } str;
        struct {
            DWORD length;
            char data[];
        } data;
    };
} SdbTag;

#include "poppack.h"

typedef struct _SdbPrivate {
    char *data;
    HANDLE file;
    DWORD size;
    TAGID stringTable;
} SdbPrivate;

static SdbPrivate *new_sdb(void) {
    SdbPrivate *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(SdbPrivate));
    ret->data = NULL;
    ret->file = INVALID_HANDLE_VALUE;
    ret->size = 0;
    ret->stringTable = 0;
    return ret;
}

static void free_sdb(SdbPrivate *sdb) {
    HANDLE hHeap = GetProcessHeap();
    CloseHandle(sdb->file);
    HeapFree(hHeap, 0, sdb->data);
    HeapFree(hHeap, 0, sdb);
}

static DWORD tag_size(SdbTag *tag) {
    DWORD tagSize = 0;
    switch(TAG_TYPE(tag->tag)) {
        case TAG_TYPE_NULL:
            tagSize = 2;
            break;
        case TAG_TYPE_WORD:
            tagSize = 4;
            break;
        case TAG_TYPE_DWORD:
            tagSize = 6;
            break;
        case TAG_TYPE_QWORD:
            tagSize = 10;
            break;
        case TAG_TYPE_STRINGREF:
            tagSize = 6;
            break;
        case TAG_TYPE_LIST:
            tagSize = 6 + tag->length;
            break;
        case TAG_TYPE_STRING:
            tagSize = 6 + tag->length;
            break;
        case TAG_TYPE_BINARY:
            tagSize = 6 + tag->length;
            break;
        case TAG_TYPE_BYTE:
        default:
            /* unknown tag layout, cannot parse this file */
            FIXME("Unknown tag 0x%x : %s\n",
                tag->tag, debugstr_w(SdbTagToString(tag->tag)));
    }
    //pad to word boundry
    if(tagSize & 0x1)
        tagSize++;
    return tagSize;
}

static SdbTag* get_tag(SdbPrivate *sdb, TAGID tagId) {
    SdbTag *ret;
    char *data = sdb->data;
    
    /* Should be 2 byte aligned */
    if(tagId & 0x1) {
        FIXME("Unaligned tag\n");
        return NULL;
    }
    
    if((tagId + 2) > sdb->size)
        return NULL;
    
    if(tagId == TAGID_NULL)
        tagId = TAGID_ROOT;
    
    ret = (SdbTag*)(&data[tagId]);
    
    return ret;
}

static TAGID next_tag(SdbPrivate *sdb, TAGID tagId) {
    SdbTag *tag = get_tag(sdb, tagId);
    TAGID ret;
    
    if(tag == NULL)
        return TAGID_NULL;
    
    ret = tagId + tag_size(tag);
    
    if(ret >= sdb->size)
        return TAGID_NULL;
    
    return ret;
}

static BOOL parse_tags(SdbPrivate *sdb) {
    char *data = (char*)sdb->data;
    DWORD size = sdb->size;
    //TODO: check for header
    DWORD offset = TAGID_ROOT;
    
    while(offset < size) {
        SdbTag *cur = (SdbTag*)(&data[offset]);
        DWORD tagSize;
        
        tagSize = tag_size(cur);
        
        if(tagSize == 0) {
            ERR("Unknown/Invalid tag at 0x%x\n", offset);
            return FALSE;
        }
        /* TRACE("Parsing Tag 0x%x,0x%x:0x%x : %s\n",
            offset, tagSize, cur->tag,
            debugstr_w(SdbTagToString(cur->tag))); */
        
        switch(cur->tag) {
            case TAG_STRINGTABLE:
                sdb->stringTable = offset;
                break;
        }
        if(TAG_TYPE(cur->tag) == TAG_TYPE_LIST)
            tagSize = 6;
        
        offset += tagSize;
    }
    return TRUE;
}

PDB WINAPI SdbCreateDatabase( LPCWSTR path, PATH_TYPE type )
{
    TRACE("(%s, %d) Stub!\n", debugstr_w(path), type);
    //SdbPrivate ret = new_sdb();
    //sdb->file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_, 0, 0);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

PDB WINAPI SdbOpenDatabase( LPCWSTR path, PATH_TYPE type )
{
    HANDLE file;
    SdbPrivate *ret = NULL;
    
    TRACE("(%s, %d)\n", debugstr_w(path), type);
    
    if(type != DOS_PATH)
        goto error;
    
    file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if(file == INVALID_HANDLE_VALUE)
        return NULL;
    
    ret = new_sdb();
    ret->file = file;
    ret->size = GetFileSize(file, NULL);
    ret->data = HeapAlloc(GetProcessHeap(), 0, ret->size);
    TRACE("File size: %d, allocated at %p\n", ret->size, ret->data);
    ReadFile(file, ret->data, ret->size, NULL, NULL);
    
    if(!parse_tags(ret))
        goto error;
    
    return ret;

error:
    if(ret)
        free_sdb(ret);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

VOID WINAPI SdbCloseDatabase( PDB pdb )
{
    TRACE("(%p)\n", pdb);
    free_sdb(pdb);
}

TAGID WINAPI SdbFindFirstTag( PDB pdb, TAGID tiParent, TAG tTag )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    TAGID tagId;
    SdbTag* tag;
    
    TRACE("(%p, 0x%x, 0x%x)\n", pdb, tiParent, tTag);
    
    tagId = SdbGetFirstChild(pdb, tiParent);
    tag = get_tag(sdb, tagId);
    
    while(tagId) {
        if(tag->tag == tTag)
            return tagId;
        
        tagId = next_tag(sdb, tagId);
        tag = get_tag(sdb, tagId);
    }
    
    return TAGID_NULL;
}

TAGID WINAPI SdbGetFirstChild( PDB pdb, TAGID tiParent )
{
    TRACE("(%p, 0x%x)\n", pdb, tiParent);

    if(tiParent == TAGID_NULL)
        return TAGID_ROOT;
    
    if(TAG_TYPE(SdbGetTagFromTagID(pdb, tiParent)) != TAG_TYPE_LIST)
        return TAGID_NULL;
    
    return tiParent + 6;
        /* list tags are 6 bytes, first child is immediately after */
}

TAGID WINAPI SdbGetNextChild( PDB pdb, TAGID tiParent, TAGID tiPrev )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag* in = get_tag(sdb, tiPrev);
    SdbTag* parent;
    TAGID tagId = tiPrev;
    
    TRACE("(%p, 0x%x, 0x%x)\n", pdb, tiParent, tiPrev);
    
    //TODO: check error conditions on windows
    if(in == NULL)
        return TAGID_NULL;
    
    tagId += tag_size(in);
    
    if(tagId >= sdb->size)
        return TAGID_NULL;
    
    if(tiParent == TAGID_NULL)
        return tagId;
    
    parent = get_tag(sdb, tiParent);
    
    if(tagId >= (tiParent + parent->dword + 6))
        return TAGID_NULL;
    
    return tagId;

}

TAGID WINAPI SdbFindNextTag( PDB pdb, TAGID tiParent, TAGID tiPrev )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    TAGID tagId;
    TAG tTag;
    SdbTag* tag;
    
    TRACE("(%p, 0x%x, 0x%x)\n", pdb, tiParent, tiPrev);
    
    tTag = get_tag(sdb, tiPrev)->tag;
    tagId = SdbGetNextChild(pdb, tiParent, tiPrev);
    tag = get_tag(sdb, tagId);
    
    while(tagId) {
        if(tag->tag == tTag)
            return tagId;
        
        tagId = next_tag(sdb, tagId);
        tag = get_tag(sdb, tagId);
    }
    
    return TAGID_NULL;

}

TAG WINAPI SdbGetTagFromTagID( PDB pdb, TAGID tiWhich )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x)\n", pdb, tiWhich);
    
    tag = get_tag(sdb, tiWhich);
    if(tag)
        return tag->tag;
    
    //TODO: check error conditions on windows
    return TAGID_NULL;
}

HMODULE WINAPI SdbOpenApphelpResourceFile( LPCWSTR pwszACResourceFile )
{
    FIXME("(%s) Stub!\n", debugstr_w(pwszACResourceFile));
    
    return INVALID_HANDLE_VALUE;
}

SdbTag *get_string_tag(SdbPrivate* sdb, TAGID tagId) {
    SdbTag *tag = get_tag(sdb, tagId);
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_STRING)
        return tag;
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_STRINGREF)
        return get_string_tag(sdb, tag->length + sdb->stringTable);
    
    return NULL;
}

LPWSTR WINAPI SdbGetStringTagPtr( PDB pdb, TAGID tiWhich )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x)\n", pdb, tiWhich);
    
    tag = get_string_tag(sdb, tiWhich);
    
    if(tag)
        return tag->str.str;
    
    return NULL;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

BOOL WINAPI SdbReadStringTag( PDB pdb, TAGID tiWhich, LPWSTR pwszBuffer, DWORD cchBufferSize )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x, %p, %d)\n", pdb, tiWhich, pwszBuffer, cchBufferSize);
    
    tag = get_string_tag(sdb, tiWhich);
    
    if(tag == NULL)
        return FALSE;
    
    memcpy(pwszBuffer, tag->str.str, MIN(cchBufferSize, tag->length));
    
    return TRUE;
}

//TODO: these tags have a dwDefault flag, i assume it's for unresolvable
//tag id's, need to check windows

DWORD WINAPI SdbReadDWORDTag( PDB pdb, TAGID tiWhich, DWORD dwDefault )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x, %d)\n", pdb, tiWhich, dwDefault);
    
    tag = get_tag(sdb, tiWhich);
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_DWORD)
        return tag->dword;
    
    return dwDefault;
}

ULONGLONG WINAPI SdbReadQWORDTag( PDB pdb, TAGID tiWhich, ULONGLONG qwDefault )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x, %d)\n", pdb, tiWhich, (int)qwDefault);
    
    tag = get_tag(sdb, tiWhich);
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_QWORD)
        return tag->qword;
    
    return qwDefault;
}

DWORD WINAPI SdbGetTagDataSize( PDB pdb, TAGID tiWhich )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x)\n", pdb, tiWhich);
    
    tag = get_tag(sdb, tiWhich);
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_BINARY)
        return tag->length;
    
    return 0;
}

PVOID WINAPI SdbGetBinaryTagData( PDB pdb, TAGID tiWhich )
{
    SdbPrivate *sdb = (SdbPrivate*)pdb;
    SdbTag *tag;
    
    TRACE("(%p, 0x%x)\n", pdb, tiWhich);
    
    tag = get_tag(sdb, tiWhich);
    
    if(tag && TAG_TYPE(tag->tag) == TAG_TYPE_BINARY)
        return tag->data.data;
    
    return TAGID_NULL;
}

BOOL WINAPI SdbFormatAttribute( PATTRINFO pAttrInfo, LPWSTR pchBuffer, DWORD dwBufferSize )
{
    FIXME("(%p, %p, %d) Stub!\n", pAttrInfo, pchBuffer, dwBufferSize);
    
    return FALSE;
}


static const WCHAR str_NULL[] = {'N','U','L','L',0};
static const WCHAR str_INCLUDE[] = {'I','N','C','L','U','D','E',0};
static const WCHAR str_GENERAL[] = {'G','E','N','E','R','A','L',0};
static const WCHAR str_MATCH_LOGIC_NOT[] = {'M','A','T','C','H','_','L','O','G','I','C','_','N','O','T',0};
static const WCHAR str_APPLY_ALL_SHIMS[] = {'A','P','P','L','Y','_','A','L','L','_','S','H','I','M','S',0};
static const WCHAR str_USE_SERVICE_PACK_FILES[] = {'U','S','E','_','S','E','R','V','I','C','E','_','P','A','C','K','_','F','I','L','E','S',0};
static const WCHAR str_MITIGATION_OS[] = {'M','I','T','I','G','A','T','I','O','N','_','O','S',0};
static const WCHAR str_BLOCK_UPGRADE[] = {'B','L','O','C','K','_','U','P','G','R','A','D','E',0};
static const WCHAR str_INCLUDEEXCLUDEDLL[] = {'I','N','C','L','U','D','E','E','X','C','L','U','D','E','D','L','L',0};
static const WCHAR str_RAC_EVENT_OFF[] = {'R','A','C','_','E','V','E','N','T','_','O','F','F',0};
static const WCHAR str_TELEMETRY_OFF[] = {'T','E','L','E','M','E','T','R','Y','_','O','F','F',0};
static const WCHAR str_SHIM_ENGINE_OFF[] = {'S','H','I','M','_','E','N','G','I','N','E','_','O','F','F',0};
static const WCHAR str_LAYER_PROPAGATION_OFF[] = {'L','A','Y','E','R','_','P','R','O','P','A','G','A','T','I','O','N','_','O','F','F',0};
static const WCHAR str_REINSTALL_UPGRADE[] = {'R','E','I','N','S','T','A','L','L','_','U','P','G','R','A','D','E',0};
static const WCHAR str_MATCH_MODE[] = {'M','A','T','C','H','_','M','O','D','E',0};
static const WCHAR str_TAG[] = {'T','A','G',0};
static const WCHAR str_INDEX_TAG[] = {'I','N','D','E','X','_','T','A','G',0};
static const WCHAR str_INDEX_KEY[] = {'I','N','D','E','X','_','K','E','Y',0};
static const WCHAR str_SIZE[] = {'S','I','Z','E',0};
static const WCHAR str_OFFSET[] = {'O','F','F','S','E','T',0};
static const WCHAR str_CHECKSUM[] = {'C','H','E','C','K','S','U','M',0};
static const WCHAR str_SHIM_TAGID[] = {'S','H','I','M','_','T','A','G','I','D',0};
static const WCHAR str_PATCH_TAGID[] = {'P','A','T','C','H','_','T','A','G','I','D',0};
static const WCHAR str_MODULE_TYPE[] = {'M','O','D','U','L','E','_','T','Y','P','E',0};
static const WCHAR str_VERDATEHI[] = {'V','E','R','D','A','T','E','H','I',0};
static const WCHAR str_VERDATELO[] = {'V','E','R','D','A','T','E','L','O',0};
static const WCHAR str_VERFILEOS[] = {'V','E','R','F','I','L','E','O','S',0};
static const WCHAR str_VERFILETYPE[] = {'V','E','R','F','I','L','E','T','Y','P','E',0};
static const WCHAR str_PE_CHECKSUM[] = {'P','E','_','C','H','E','C','K','S','U','M',0};
static const WCHAR str_PREVOSMAJORVER[] = {'P','R','E','V','O','S','M','A','J','O','R','V','E','R',0};
static const WCHAR str_PREVOSMINORVER[] = {'P','R','E','V','O','S','M','I','N','O','R','V','E','R',0};
static const WCHAR str_PREVOSPLATFORMID[] = {'P','R','E','V','O','S','P','L','A','T','F','O','R','M','I','D',0};
static const WCHAR str_PREVOSBUILDNO[] = {'P','R','E','V','O','S','B','U','I','L','D','N','O',0};
static const WCHAR str_PROBLEMSEVERITY[] = {'P','R','O','B','L','E','M','S','E','V','E','R','I','T','Y',0};
static const WCHAR str_LANGID[] = {'L','A','N','G','I','D',0};
static const WCHAR str_VER_LANGUAGE[] = {'V','E','R','_','L','A','N','G','U','A','G','E',0};
static const WCHAR str_ENGINE[] = {'E','N','G','I','N','E',0};
static const WCHAR str_HTMLHELPID[] = {'H','T','M','L','H','E','L','P','I','D',0};
static const WCHAR str_INDEX_FLAGS[] = {'I','N','D','E','X','_','F','L','A','G','S',0};
static const WCHAR str_FLAGS[] = {'F','L','A','G','S',0};
static const WCHAR str_DATA_VALUETYPE[] = {'D','A','T','A','_','V','A','L','U','E','T','Y','P','E',0};
static const WCHAR str_DATA_DWORD[] = {'D','A','T','A','_','D','W','O','R','D',0};
static const WCHAR str_LAYER_TAGID[] = {'L','A','Y','E','R','_','T','A','G','I','D',0};
static const WCHAR str_MSI_TRANSFORM_TAGID[] = {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','T','A','G','I','D',0};
static const WCHAR str_LINKER_VERSION[] = {'L','I','N','K','E','R','_','V','E','R','S','I','O','N',0};
static const WCHAR str_LINK_DATE[] = {'L','I','N','K','_','D','A','T','E',0};
static const WCHAR str_UPTO_LINK_DATE[] = {'U','P','T','O','_','L','I','N','K','_','D','A','T','E',0};
static const WCHAR str_OS_SERVICE_PACK[] = {'O','S','_','S','E','R','V','I','C','E','_','P','A','C','K',0};
static const WCHAR str_FLAG_TAGID[] = {'F','L','A','G','_','T','A','G','I','D',0};
static const WCHAR str_RUNTIME_PLATFORM[] = {'R','U','N','T','I','M','E','_','P','L','A','T','F','O','R','M',0};
static const WCHAR str_OS_SKU[] = {'O','S','_','S','K','U',0};
static const WCHAR str_OS_PLATFORM[] = {'O','S','_','P','L','A','T','F','O','R','M',0};
static const WCHAR str_APP_NAME_RC_ID[] = {'A','P','P','_','N','A','M','E','_','R','C','_','I','D',0};
static const WCHAR str_VENDOR_NAME_RC_ID[] = {'V','E','N','D','O','R','_','N','A','M','E','_','R','C','_','I','D',0};
static const WCHAR str_SUMMARY_MSG_RC_ID[] = {'S','U','M','M','A','R','Y','_','M','S','G','_','R','C','_','I','D',0};
static const WCHAR str_VISTA_SKU[] = {'V','I','S','T','A','_','S','K','U',0};
static const WCHAR str_DESCRIPTION_RC_ID[] = {'D','E','S','C','R','I','P','T','I','O','N','_','R','C','_','I','D',0};
static const WCHAR str_PARAMETER1_RC_ID[] = {'P','A','R','A','M','E','T','E','R','1','_','R','C','_','I','D',0};
static const WCHAR str_CONTEXT_TAGID[] = {'C','O','N','T','E','X','T','_','T','A','G','I','D',0};
static const WCHAR str_EXE_WRAPPER[] = {'E','X','E','_','W','R','A','P','P','E','R',0};
static const WCHAR str_TAGID[] = {'T','A','G','I','D',0};
static const WCHAR str_TIME[] = {'T','I','M','E',0};
static const WCHAR str_BIN_FILE_VERSION[] = {'B','I','N','_','F','I','L','E','_','V','E','R','S','I','O','N',0};
static const WCHAR str_BIN_PRODUCT_VERSION[] = {'B','I','N','_','P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0};
static const WCHAR str_MODTIME[] = {'M','O','D','T','I','M','E',0};
static const WCHAR str_FLAG_MASK_KERNEL[] = {'F','L','A','G','_','M','A','S','K','_','K','E','R','N','E','L',0};
static const WCHAR str_UPTO_BIN_PRODUCT_VERSION[] = {'U','P','T','O','_','B','I','N','_','P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0};
static const WCHAR str_DATA_QWORD[] = {'D','A','T','A','_','Q','W','O','R','D',0};
static const WCHAR str_FLAG_MASK_USER[] = {'F','L','A','G','_','M','A','S','K','_','U','S','E','R',0};
static const WCHAR str_FLAGS_NTVDM1[] = {'F','L','A','G','S','_','N','T','V','D','M','1',0};
static const WCHAR str_FLAGS_NTVDM2[] = {'F','L','A','G','S','_','N','T','V','D','M','2',0};
static const WCHAR str_FLAGS_NTVDM3[] = {'F','L','A','G','S','_','N','T','V','D','M','3',0};
static const WCHAR str_FLAG_MASK_SHELL[] = {'F','L','A','G','_','M','A','S','K','_','S','H','E','L','L',0};
static const WCHAR str_UPTO_BIN_FILE_VERSION[] = {'U','P','T','O','_','B','I','N','_','F','I','L','E','_','V','E','R','S','I','O','N',0};
static const WCHAR str_FLAG_MASK_FUSION[] = {'F','L','A','G','_','M','A','S','K','_','F','U','S','I','O','N',0};
static const WCHAR str_FLAG_PROCESSPARAM[] = {'F','L','A','G','_','P','R','O','C','E','S','S','P','A','R','A','M',0};
static const WCHAR str_FLAG_LUA[] = {'F','L','A','G','_','L','U','A',0};
static const WCHAR str_FLAG_INSTALL[] = {'F','L','A','G','_','I','N','S','T','A','L','L',0};
static const WCHAR str_NAME[] = {'N','A','M','E',0};
static const WCHAR str_DESCRIPTION[] = {'D','E','S','C','R','I','P','T','I','O','N',0};
static const WCHAR str_MODULE[] = {'M','O','D','U','L','E',0};
static const WCHAR str_API[] = {'A','P','I',0};
static const WCHAR str_VENDOR[] = {'V','E','N','D','O','R',0};
static const WCHAR str_APP_NAME[] = {'A','P','P','_','N','A','M','E',0};
static const WCHAR str_COMMAND_LINE[] = {'C','O','M','M','A','N','D','_','L','I','N','E',0};
static const WCHAR str_COMPANY_NAME[] = {'C','O','M','P','A','N','Y','_','N','A','M','E',0};
static const WCHAR str_DLLFILE[] = {'D','L','L','F','I','L','E',0};
static const WCHAR str_WILDCARD_NAME[] = {'W','I','L','D','C','A','R','D','_','N','A','M','E',0};
static const WCHAR str_PRODUCT_NAME[] = {'P','R','O','D','U','C','T','_','N','A','M','E',0};
static const WCHAR str_PRODUCT_VERSION[] = {'P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0};
static const WCHAR str_FILE_DESCRIPTION[] = {'F','I','L','E','_','D','E','S','C','R','I','P','T','I','O','N',0};
static const WCHAR str_FILE_VERSION[] = {'F','I','L','E','_','V','E','R','S','I','O','N',0};
static const WCHAR str_ORIGINAL_FILENAME[] = {'O','R','I','G','I','N','A','L','_','F','I','L','E','N','A','M','E',0};
static const WCHAR str_INTERNAL_NAME[] = {'I','N','T','E','R','N','A','L','_','N','A','M','E',0};
static const WCHAR str_LEGAL_COPYRIGHT[] = {'L','E','G','A','L','_','C','O','P','Y','R','I','G','H','T',0};
static const WCHAR str_16BIT_DESCRIPTION[] = {'1','6','B','I','T','_','D','E','S','C','R','I','P','T','I','O','N',0};
static const WCHAR str_APPHELP_DETAILS[] = {'A','P','P','H','E','L','P','_','D','E','T','A','I','L','S',0};
static const WCHAR str_LINK_URL[] = {'L','I','N','K','_','U','R','L',0};
static const WCHAR str_LINK_TEXT[] = {'L','I','N','K','_','T','E','X','T',0};
static const WCHAR str_APPHELP_TITLE[] = {'A','P','P','H','E','L','P','_','T','I','T','L','E',0};
static const WCHAR str_APPHELP_CONTACT[] = {'A','P','P','H','E','L','P','_','C','O','N','T','A','C','T',0};
static const WCHAR str_SXS_MANIFEST[] = {'S','X','S','_','M','A','N','I','F','E','S','T',0};
static const WCHAR str_DATA_STRING[] = {'D','A','T','A','_','S','T','R','I','N','G',0};
static const WCHAR str_MSI_TRANSFORM_FILE[] = {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','F','I','L','E',0};
static const WCHAR str_16BIT_MODULE_NAME[] = {'1','6','B','I','T','_','M','O','D','U','L','E','_','N','A','M','E',0};
static const WCHAR str_LAYER_DISPLAYNAME[] = {'L','A','Y','E','R','_','D','I','S','P','L','A','Y','N','A','M','E',0};
static const WCHAR str_COMPILER_VERSION[] = {'C','O','M','P','I','L','E','R','_','V','E','R','S','I','O','N',0};
static const WCHAR str_ACTION_TYPE[] = {'A','C','T','I','O','N','_','T','Y','P','E',0};
static const WCHAR str_EXPORT_NAME[] = {'E','X','P','O','R','T','_','N','A','M','E',0};
static const WCHAR str_DATABASE[] = {'D','A','T','A','B','A','S','E',0};
static const WCHAR str_LIBRARY[] = {'L','I','B','R','A','R','Y',0};
static const WCHAR str_INEXCLUDE[] = {'I','N','E','X','C','L','U','D','E',0};
static const WCHAR str_SHIM[] = {'S','H','I','M',0};
static const WCHAR str_PATCH[] = {'P','A','T','C','H',0};
static const WCHAR str_APP[] = {'A','P','P',0};
static const WCHAR str_EXE[] = {'E','X','E',0};
static const WCHAR str_MATCHING_FILE[] = {'M','A','T','C','H','I','N','G','_','F','I','L','E',0};
static const WCHAR str_SHIM_REF[] = {'S','H','I','M','_','R','E','F',0};
static const WCHAR str_PATCH_REF[] = {'P','A','T','C','H','_','R','E','F',0};
static const WCHAR str_LAYER[] = {'L','A','Y','E','R',0};
static const WCHAR str_FILE[] = {'F','I','L','E',0};
static const WCHAR str_APPHELP[] = {'A','P','P','H','E','L','P',0};
static const WCHAR str_LINK[] = {'L','I','N','K',0};
static const WCHAR str_DATA[] = {'D','A','T','A',0};
static const WCHAR str_MSI_TRANSFORM[] = {'M','S','I','_','T','R','A','N','S','F','O','R','M',0};
static const WCHAR str_MSI_TRANSFORM_REF[] = {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','R','E','F',0};
static const WCHAR str_MSI_PACKAGE[] = {'M','S','I','_','P','A','C','K','A','G','E',0};
static const WCHAR str_FLAG[] = {'F','L','A','G',0};
static const WCHAR str_MSI_CUSTOM_ACTION[] = {'M','S','I','_','C','U','S','T','O','M','_','A','C','T','I','O','N',0};
static const WCHAR str_FLAG_REF[] = {'F','L','A','G','_','R','E','F',0};
static const WCHAR str_ACTION[] = {'A','C','T','I','O','N',0};
static const WCHAR str_LOOKUP[] = {'L','O','O','K','U','P',0};
static const WCHAR str_CONTEXT[] = {'C','O','N','T','E','X','T',0};
static const WCHAR str_CONTEXT_REF[] = {'C','O','N','T','E','X','T','_','R','E','F',0};
static const WCHAR str_STRINGTABLE[] = {'S','T','R','I','N','G','T','A','B','L','E',0};
static const WCHAR str_INDEXES[] = {'I','N','D','E','X','E','S',0};
static const WCHAR str_INDEX[] = {'I','N','D','E','X',0};
static const WCHAR str_STRINGTABLE_ITEM[] = {'S','T','R','I','N','G','T','A','B','L','E','_','I','T','E','M',0};
static const WCHAR str_PATCH_BITS[] = {'P','A','T','C','H','_','B','I','T','S',0};
static const WCHAR str_FILE_BITS[] = {'F','I','L','E','_','B','I','T','S',0};
static const WCHAR str_EXE_ID[] = {'E','X','E','_','I','D',0};
static const WCHAR str_DATA_BITS[] = {'D','A','T','A','_','B','I','T','S',0};
static const WCHAR str_MSI_PACKAGE_ID[] = {'M','S','I','_','P','A','C','K','A','G','E','_','I','D',0};
static const WCHAR str_DATABASE_ID[] = {'D','A','T','A','B','A','S','E','_','I','D',0};
static const WCHAR str_CONTEXT_PLATFORM_ID[] = {'C','O','N','T','E','X','T','_','P','L','A','T','F','O','R','M','_','I','D',0};
static const WCHAR str_CONTEXT_BRANCH_ID[] = {'C','O','N','T','E','X','T','_','B','R','A','N','C','H','_','I','D',0};
static const WCHAR str_FIX_ID[] = {'F','I','X','_','I','D',0};
static const WCHAR str_APP_ID[] = {'A','P','P','_','I','D',0};
static const WCHAR str_INDEX_BITS[] = {'I','N','D','E','X','_','B','I','T','S',0};
static const WCHAR str_InvalidTag[] = {'I','n','v','a','l','i','d','T','a','g',0};

LPCWSTR WINAPI SdbTagToString( TAG t )
{
    LPCWSTR ret;
    TRACE("(0x%x)\n", (int)t);
    
    switch(t) {
        case 0x0000: ret = str_NULL; break;
        case 0x1001: ret = str_INCLUDE; break;
        case 0x1002: ret = str_GENERAL; break;
        case 0x1003: ret = str_MATCH_LOGIC_NOT; break;
        case 0x1004: ret = str_APPLY_ALL_SHIMS; break;
        case 0x1005: ret = str_USE_SERVICE_PACK_FILES; break;
        case 0x1006: ret = str_MITIGATION_OS; break;
        case 0x1007: ret = str_BLOCK_UPGRADE; break;
        case 0x1008: ret = str_INCLUDEEXCLUDEDLL; break;
        case 0x1009: ret = str_RAC_EVENT_OFF; break;
        case 0x100a: ret = str_TELEMETRY_OFF; break;
        case 0x100b: ret = str_SHIM_ENGINE_OFF; break;
        case 0x100c: ret = str_LAYER_PROPAGATION_OFF; break;
        case 0x100d: ret = str_REINSTALL_UPGRADE; break;
        case 0x3001: ret = str_MATCH_MODE; break;
        case 0x3801: ret = str_TAG; break;
        case 0x3802: ret = str_INDEX_TAG; break;
        case 0x3803: ret = str_INDEX_KEY; break;
        case 0x4001: ret = str_SIZE; break;
        case 0x4002: ret = str_OFFSET; break;
        case 0x4003: ret = str_CHECKSUM; break;
        case 0x4004: ret = str_SHIM_TAGID; break;
        case 0x4005: ret = str_PATCH_TAGID; break;
        case 0x4006: ret = str_MODULE_TYPE; break;
        case 0x4007: ret = str_VERDATEHI; break;
        case 0x4008: ret = str_VERDATELO; break;
        case 0x4009: ret = str_VERFILEOS; break;
        case 0x400a: ret = str_VERFILETYPE; break;
        case 0x400b: ret = str_PE_CHECKSUM; break;
        case 0x400c: ret = str_PREVOSMAJORVER; break;
        case 0x400d: ret = str_PREVOSMINORVER; break;
        case 0x400e: ret = str_PREVOSPLATFORMID; break;
        case 0x400f: ret = str_PREVOSBUILDNO; break;
        case 0x4010: ret = str_PROBLEMSEVERITY; break;
        case 0x4011: ret = str_LANGID; break;
        case 0x4012: ret = str_VER_LANGUAGE; break;
        case 0x4014: ret = str_ENGINE; break;
        case 0x4015: ret = str_HTMLHELPID; break;
        case 0x4016: ret = str_INDEX_FLAGS; break;
        case 0x4017: ret = str_FLAGS; break;
        case 0x4018: ret = str_DATA_VALUETYPE; break;
        case 0x4019: ret = str_DATA_DWORD; break;
        case 0x401a: ret = str_LAYER_TAGID; break;
        case 0x401b: ret = str_MSI_TRANSFORM_TAGID; break;
        case 0x401c: ret = str_LINKER_VERSION; break;
        case 0x401d: ret = str_LINK_DATE; break;
        case 0x401e: ret = str_UPTO_LINK_DATE; break;
        case 0x401f: ret = str_OS_SERVICE_PACK; break;
        case 0x4020: ret = str_FLAG_TAGID; break;
        case 0x4021: ret = str_RUNTIME_PLATFORM; break;
        case 0x4022: ret = str_OS_SKU; break;
        case 0x4023: ret = str_OS_PLATFORM; break;
        case 0x4024: ret = str_APP_NAME_RC_ID; break;
        case 0x4025: ret = str_VENDOR_NAME_RC_ID; break;
        case 0x4026: ret = str_SUMMARY_MSG_RC_ID; break;
        case 0x4027: ret = str_VISTA_SKU; break;
        case 0x4028: ret = str_DESCRIPTION_RC_ID; break;
        case 0x4029: ret = str_PARAMETER1_RC_ID; break;
        case 0x4030: ret = str_CONTEXT_TAGID; break;
        case 0x4031: ret = str_EXE_WRAPPER; break;
        case 0x4801: ret = str_TAGID; break;
        case 0x5001: ret = str_TIME; break;
        case 0x5002: ret = str_BIN_FILE_VERSION; break;
        case 0x5003: ret = str_BIN_PRODUCT_VERSION; break;
        case 0x5004: ret = str_MODTIME; break;
        case 0x5005: ret = str_FLAG_MASK_KERNEL; break;
        case 0x5006: ret = str_UPTO_BIN_PRODUCT_VERSION; break;
        case 0x5007: ret = str_DATA_QWORD; break;
        case 0x5008: ret = str_FLAG_MASK_USER; break;
        case 0x5009: ret = str_FLAGS_NTVDM1; break;
        case 0x500a: ret = str_FLAGS_NTVDM2; break;
        case 0x500b: ret = str_FLAGS_NTVDM3; break;
        case 0x500c: ret = str_FLAG_MASK_SHELL; break;
        case 0x500d: ret = str_UPTO_BIN_FILE_VERSION; break;
        case 0x500e: ret = str_FLAG_MASK_FUSION; break;
        case 0x500f: ret = str_FLAG_PROCESSPARAM; break;
        case 0x5010: ret = str_FLAG_LUA; break;
        case 0x5011: ret = str_FLAG_INSTALL; break;
        case 0x6001: ret = str_NAME; break;
        case 0x6002: ret = str_DESCRIPTION; break;
        case 0x6003: ret = str_MODULE; break;
        case 0x6004: ret = str_API; break;
        case 0x6005: ret = str_VENDOR; break;
        case 0x6006: ret = str_APP_NAME; break;
        case 0x6008: ret = str_COMMAND_LINE; break;
        case 0x6009: ret = str_COMPANY_NAME; break;
        case 0x600a: ret = str_DLLFILE; break;
        case 0x600b: ret = str_WILDCARD_NAME; break;
        case 0x6010: ret = str_PRODUCT_NAME; break;
        case 0x6011: ret = str_PRODUCT_VERSION; break;
        case 0x6012: ret = str_FILE_DESCRIPTION; break;
        case 0x6013: ret = str_FILE_VERSION; break;
        case 0x6014: ret = str_ORIGINAL_FILENAME; break;
        case 0x6015: ret = str_INTERNAL_NAME; break;
        case 0x6016: ret = str_LEGAL_COPYRIGHT; break;
        case 0x6017: ret = str_16BIT_DESCRIPTION; break;
        case 0x6018: ret = str_APPHELP_DETAILS; break;
        case 0x6019: ret = str_LINK_URL; break;
        case 0x601a: ret = str_LINK_TEXT; break;
        case 0x601b: ret = str_APPHELP_TITLE; break;
        case 0x601c: ret = str_APPHELP_CONTACT; break;
        case 0x601d: ret = str_SXS_MANIFEST; break;
        case 0x601e: ret = str_DATA_STRING; break;
        case 0x601f: ret = str_MSI_TRANSFORM_FILE; break;
        case 0x6020: ret = str_16BIT_MODULE_NAME; break;
        case 0x6021: ret = str_LAYER_DISPLAYNAME; break;
        case 0x6022: ret = str_COMPILER_VERSION; break;
        case 0x6023: ret = str_ACTION_TYPE; break;
        case 0x6024: ret = str_EXPORT_NAME; break;
        case 0x7001: ret = str_DATABASE; break;
        case 0x7002: ret = str_LIBRARY; break;
        case 0x7003: ret = str_INEXCLUDE; break;
        case 0x7004: ret = str_SHIM; break;
        case 0x7005: ret = str_PATCH; break;
        case 0x7006: ret = str_APP; break;
        case 0x7007: ret = str_EXE; break;
        case 0x7008: ret = str_MATCHING_FILE; break;
        case 0x7009: ret = str_SHIM_REF; break;
        case 0x700a: ret = str_PATCH_REF; break;
        case 0x700b: ret = str_LAYER; break;
        case 0x700c: ret = str_FILE; break;
        case 0x700d: ret = str_APPHELP; break;
        case 0x700e: ret = str_LINK; break;
        case 0x700f: ret = str_DATA; break;
        case 0x7010: ret = str_MSI_TRANSFORM; break;
        case 0x7011: ret = str_MSI_TRANSFORM_REF; break;
        case 0x7012: ret = str_MSI_PACKAGE; break;
        case 0x7013: ret = str_FLAG; break;
        case 0x7014: ret = str_MSI_CUSTOM_ACTION; break;
        case 0x7015: ret = str_FLAG_REF; break;
        case 0x7016: ret = str_ACTION; break;
        case 0x7017: ret = str_LOOKUP; break;
        case 0x7018: ret = str_CONTEXT; break;
        case 0x7019: ret = str_CONTEXT_REF; break;
        case 0x7801: ret = str_STRINGTABLE; break;
        case 0x7802: ret = str_INDEXES; break;
        case 0x7803: ret = str_INDEX; break;
        case 0x8801: ret = str_STRINGTABLE_ITEM; break;
        case 0x9002: ret = str_PATCH_BITS; break;
        case 0x9003: ret = str_FILE_BITS; break;
        case 0x9004: ret = str_EXE_ID; break;
        case 0x9005: ret = str_DATA_BITS; break;
        case 0x9006: ret = str_MSI_PACKAGE_ID; break;
        case 0x9007: ret = str_DATABASE_ID; break;
        case 0x9008: ret = str_CONTEXT_PLATFORM_ID; break;
        case 0x9009: ret = str_CONTEXT_BRANCH_ID; break;
        case 0x9010: ret = str_FIX_ID; break;
        case 0x9011: ret = str_APP_ID; break;
        case 0x9801: ret = str_INDEX_BITS; break;
        default: ret = str_InvalidTag; break;
    }
    
    return ret;
}
