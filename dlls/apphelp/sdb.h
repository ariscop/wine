/*
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

#include <windef.h>

typedef enum _PATH_TYPE {
    DOS_PATH,
    NT_PATH
} PATH_TYPE;

typedef HANDLE PDB;
typedef WORD   TAG;
typedef DWORD  TAGID;


typedef struct tagATTRINFO {
    TAG   tAttrID;
    DWORD dwFlags;
    union {
        ULONGLONG  ullAttr;
        DWORD      dwAttr;
        //TODO: v check type
        LPWSTR     lpAttr;
    };
} ATTRINFO, *PATTRINFO;

#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

#define TAG_TYPE(x) ((x) & 0xF000)

#define TAGID_NULL 0x0
#define TAGID_ROOT 0xC

//TAG Types
#define TAG_TYPE_NULL      0x1000
#define TAG_TYPE_BYTE      0x2000
#define TAG_TYPE_WORD      0x3000
#define TAG_TYPE_DWORD     0x4000
#define TAG_TYPE_QWORD     0x5000
#define TAG_TYPE_STRINGREF 0x6000
#define TAG_TYPE_LIST      0x7000
#define TAG_TYPE_STRING    0x8000
#define TAG_TYPE_BINARY    0x9000

#define TAG_NULL 0x0000

//TAG_TYPE_NULL, 0x1000
#define TAG_INCLUDE 0x1001
#define TAG_GENERAL 0x1002
#define TAG_MATCH_LOGIC_NOT 0x1003
#define TAG_APPLY_ALL_SHIMS 0x1004
#define TAG_USE_SERVICE_PACK_FILES 0x1005
#define TAG_MITIGATION_OS 0x1006
#define TAG_BLOCK_UPGRADE 0x1007
#define TAG_INCLUDEEXCLUDEDLL 0x1008
#define TAG_RAC_EVENT_OFF 0x1009
#define TAG_TELEMETRY_OFF 0x100A
#define TAG_SHIM_ENGINE_OFF 0x100B
#define TAG_LAYER_PROPAGATION_OFF 0x100C
#define TAG_REINSTALL_UPGRADE 0x100D


//TAG_TYPE_BYTE, 0x2000
// ???

//TAG_TYPE_WORD, 0x3000
#define TAG_MATCH_MODE 0x3001 
#define TAG_TAG 0x3801 
#define TAG_INDEX_TAG 0x3802 
#define TAG_INDEX_KEY 0x3803 

//TAG_TYPE_DWORD, 0x4000
#define TAG_SIZE 0x4001
#define TAG_OFFSET 0x4002
#define TAG_CHECKSUM 0x4003
#define TAG_SHIM_TAGID 0x4004
#define TAG_PATCH_TAGID 0x4005
#define TAG_MODULE_TYPE 0x4006
#define TAG_VERDATEHI 0x4007
#define TAG_VERDATELO 0x4008
#define TAG_VERFILEOS 0x4009
#define TAG_VERFILETYPE 0x400A
#define TAG_PE_CHECKSUM 0x400B
#define TAG_PREVOSMAJORVER 0x400C
#define TAG_PREVOSMINORVER 0x400D
#define TAG_PREVOSPLATFORMID 0x400E
#define TAG_PREVOSBUILDNO 0x400F
#define TAG_PROBLEMSEVERITY 0x4010 
#define TAG_LANGID 0x4011 
#define TAG_VER_LANGUAGE 0x4012 
#define TAG_ENGINE 0x4014 
#define TAG_HTMLHELPID 0x4015 
#define TAG_INDEX_FLAGS 0x4016 
#define TAG_FLAGS 0x4017 
#define TAG_DATA_VALUETYPE 0x4018 
#define TAG_DATA_DWORD 0x4019
#define TAG_LAYER_TAGID 0x401A 
#define TAG_MSI_TRANSFORM_TAGID 0x401B 
#define TAG_LINKER_VERSION 0x401C 
#define TAG_LINK_DATE 0x401D 
#define TAG_UPTO_LINK_DATE 0x401E 
#define TAG_OS_SERVICE_PACK 0x401F 
#define TAG_FLAG_TAGID 0x4020 
#define TAG_RUNTIME_PLATFORM 0x4021 
#define TAG_OS_SKU 0x4022 
#define TAG_OS_PLATFORM 0x4023 
#define TAG_APP_NAME_RC_ID 0x4024 
#define TAG_VENDOR_NAME_RC_ID 0x4025 
#define TAG_SUMMARY_MSG_RC_ID 0x4026 
#define TAG_VISTA_SKU 0x4027 
#define TAG_DESCRIPTION_RC_ID 0x4028 
#define TAG_PARAMETER1_RC_ID 0x4029 
#define TAG_EXE_WRAPPER 0x4030 
#define TAG_TAGID 0x4801 

//TAG_TYPE_QWORD, 0x5000
#define TAG_TIME 0x5001 
#define TAG_BIN_FILE_VERSION 0x5002 
#define TAG_BIN_PRODUCT_VERSION 0x5003 
#define TAG_MODTIME 0x5004 
#define TAG_FLAG_MASK_KERNEL 0x5005 
#define TAG_UPTO_BIN_PRODUCT_VERSION 0x5006 
#define TAG_DATA_QWORD 0x5007 
#define TAG_FLAG_MASK_USER 0x5008 
#define TAG_FLAGS_NTVDM1 0x5009 
#define TAG_FLAGS_NTVDM2 0x500A 
#define TAG_FLAGS_NTVDM3 0x500B 
#define TAG_FLAG_MASK_SHELL 0x500C 
#define TAG_UPTO_BIN_FILE_VERSION 0x500D 
#define TAG_FLAG_MASK_FUSION 0x500E 
#define TAG_FLAG_PROCESSPARAM 0x500F 
#define TAG_FLAG_LUA 0x5010 
#define TAG_FLAG_INSTALL 0x5011

//TAG_TYPE_STRINGREF, 0x6000
#define TAG_NAME 0x6001 
#define TAG_DESCRIPTION 0x6002 
#define TAG_MODULE 0x6003 
#define TAG_API 0x6004
#define TAG_VENDOR 0x6005 
#define TAG_APP_NAME 0x6006 
#define TAG_COMMAND_LINE 0x6008 
#define TAG_COMPANY_NAME 0x6009 
#define TAG_DLLFILE 0x600A 
#define TAG_WILDCARD_NAME 0x600B 
#define TAG_PRODUCT_NAME 0x6010 
#define TAG_PRODUCT_VERSION 0x6011 
#define TAG_FILE_DESCRIPTION 0x6012 
#define TAG_FILE_VERSION 0x6013 
#define TAG_ORIGINAL_FILENAME 0x6014 
#define TAG_INTERNAL_NAME 0x6015 
#define TAG_LEGAL_COPYRIGHT 0x6016 
#define TAG_16BIT_DESCRIPTION 0x6017 
#define TAG_APPHELP_DETAILS 0x6018 
#define TAG_LINK_URL 0x6019 
#define TAG_LINK_TEXT 0x601A 
#define TAG_APPHELP_TITLE 0x601B 
#define TAG_APPHELP_CONTACT 0x601C 
#define TAG_SXS_MANIFEST 0x601D 
#define TAG_DATA_STRING 0x601E 
#define TAG_MSI_TRANSFORM_FILE 0x601F 
#define TAG_16BIT_MODULE_NAME 0x6020 
#define TAG_LAYER_DISPLAYNAME 0x6021 
#define TAG_COMPILER_VERSION 0x6022 
#define TAG_ACTION_TYPE 0x6023 
#define TAG_EXPORT_NAME 0x6024  

//TAG_TYPE_LIST, 0x7000
#define TAG_DATABASE 0x7001 
#define TAG_LIBRARY 0x7002 
#define TAG_INEXCLUDE 0x7003 
#define TAG_SHIM 0x7004 
#define TAG_PATCH 0x7005 
#define TAG_APP 0x7006 
#define TAG_EXE 0x7007 
#define TAG_MATCHING_FILE 0x7008 
#define TAG_SHIM_REF 0x0x7009
#define TAG_PATCH_REF 0x700A 
#define TAG_LAYER 0x700B 
#define TAG_FILE 0x700C 
#define TAG_APPHELP 0x700D 
#define TAG_LINK 0x700E 
#define TAG_DATA 0x700F 
#define TAG_MSI_TRANSFORM 0x7010 
#define TAG_MSI_TRANSFORM_REF 0x7011 
#define TAG_MSI_PACKAGE 0x7012 
#define TAG_FLAG 0x7013 
#define TAG_MSI_CUSTOM_ACTION 0x7014 
#define TAG_FLAG_REF 0x7015 
#define TAG_ACTION 0x7016 
#define TAG_LOOKUP 0x7017 
#define TAG_CONTEXT 0x7018
#define TAG_CONTEXT_REF 0x7019
#define TAG_STRINGTABLE 0x7801 
#define TAG_INDEXES 0x7802 
#define TAG_INDEX 0x7803 

//TAG_TYPE_STRING, 0x8000
#define TAG_STRINGTABLE_ITEM 0x8801

//TAG_TYPE_BINARY, 0x9000
#define TAG_PATCH_BITS 0x9002 
#define TAG_FILE_BITS 0x9003 
#define TAG_EXE_ID 0x9004 
#define TAG_DATA_BITS 0x9005 
#define TAG_MSI_PACKAGE_ID 0x9006 
#define TAG_DATABASE_ID 0x9007 
#define TAG_CONTEXT_PLATFORM_ID 0x9008
#define TAG_CONTEXT_BRANCH_ID 0x9009
#define TAG_FIX_ID 0x9010
#define TAG_APP_ID 0x9011
#define TAG_INDEX_BITS 0x9801 

PDB WINAPI SdbCreateDatabase( LPCWSTR path, PATH_TYPE type );
PDB WINAPI SdbOpenDatabase( LPCWSTR path, PATH_TYPE type );
VOID WINAPI SdbCloseDatabase( PDB pdb );
TAGID WINAPI SdbFindFirstTag( PDB pdb, TAGID tiParent, TAG tTag );
TAGID WINAPI SdbGetFirstChild( PDB pdb, TAGID tiParent );
TAGID WINAPI SdbGetNextChild( PDB pdb, TAGID tiParent, TAGID tiPrev );
TAGID WINAPI SdbFindNextTag( PDB pdb, TAGID tiParent, TAGID tiPrev );
TAG WINAPI SdbGetTagFromTagID( PDB pdb, TAGID tiWhich );
HMODULE WINAPI SdbOpenApphelpResourceFile( LPCWSTR pwszACResourceFile );
LPWSTR WINAPI SdbGetStringTagPtr( PDB pdb, TAGID tiWhich );
BOOL WINAPI SdbReadStringTag( PDB pdb, TAGID tiWhich, LPWSTR pwszBuffer, DWORD cchBufferSize );
DWORD WINAPI SdbReadDWORDTag( PDB pdb, TAGID tiWhich, DWORD dwDefault );
ULONGLONG WINAPI SdbReadQWORDTag( PDB pdb, TAGID tiWhich, ULONGLONG qwDefault );
DWORD WINAPI SdbGetTagDataSize( PDB pdb, TAGID tiWhich );
PVOID WINAPI SdbGetBinaryTagData( PDB pdb, TAGID tiWhich );
BOOL WINAPI SdbFormatAttribute( PATTRINFO pAttrInfo, LPWSTR pchBuffer, DWORD dwBufferSize );
BOOL WINAPI SdbGetFileAttributes(LPCWSTR lpwszFileName, PATTRINFO *ppAttrInfo, LPDWORD lpdwAttrCount);
LPCWSTR WINAPI SdbTagToString( TAG t );
