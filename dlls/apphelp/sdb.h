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
        LPWSTR     lpAttr;
    };
} ATTRINFO, *PATTRINFO;

#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED    0x2

#define TAG_TYPE(x) ((x) & 0xF000)

#define TAGID_NULL 0x0
#define TAGID_ROOT 0xC

#define TAG_TYPE_NULL      (0x1000)
#define TAG_TYPE_BYTE      (0x2000)
#define TAG_TYPE_WORD      (0x3000)
#define TAG_TYPE_DWORD     (0x4000)
#define TAG_TYPE_QWORD     (0x5000)
#define TAG_TYPE_STRINGREF (0x6000)
#define TAG_TYPE_LIST      (0x7000)
#define TAG_TYPE_STRING    (0x8000)
#define TAG_TYPE_BINARY    (0x9000)

#define TAG_INCLUDE             (TAG_TYPE_NULL | 0x1)
#define TAG_GENERAL             (TAG_TYPE_NULL | 0x2)
#define TAG_MATCH_LOGIC_NOT     (TAG_TYPE_NULL | 0x3)
#define TAG_APPLY_ALL_SHIMS     (TAG_TYPE_NULL | 0x4)
#define TAG_USE_SERVICE_PACK_FILES (TAG_TYPE_NULL | 0x5)
#define TAG_MITIGATION_OS       (TAG_TYPE_NULL | 0x6)
#define TAG_BLOCK_UPGRADE       (TAG_TYPE_NULL | 0x7)
#define TAG_INCLUDEEXCLUDEDLL   (TAG_TYPE_NULL | 0x8)

#define TAG_MATCH_MODE          (TAG_TYPE_WORD | 0x1)
#define TAG_TAG                 (TAG_TYPE_WORD | 0x801)
#define TAG_INDEX_TAG           (TAG_TYPE_WORD | 0x802)
#define TAG_INDEX_KEY           (TAG_TYPE_WORD | 0x803)

#define TAG_SIZE                (TAG_TYPE_DWORD | 0x1)
#define TAG_OFFSET              (TAG_TYPE_DWORD | 0x2)
#define TAG_CHECKSUM            (TAG_TYPE_DWORD | 0x3)
#define TAG_SHIM_TAGID          (TAG_TYPE_DWORD | 0x4)
#define TAG_PATCH_TAGID         (TAG_TYPE_DWORD | 0x5)
#define TAG_MODULE_TYPE         (TAG_TYPE_DWORD | 0x6)
#define TAG_VERDATEHI           (TAG_TYPE_DWORD | 0x7)
#define TAG_VERDATELO           (TAG_TYPE_DWORD | 0x8)
#define TAG_VERFILEOS           (TAG_TYPE_DWORD | 0x9)
#define TAG_VERFILETYPE         (TAG_TYPE_DWORD | 0xA)
#define TAG_PE_CHECKSUM         (TAG_TYPE_DWORD | 0xB)
#define TAG_PREVOSMAJORVER      (TAG_TYPE_DWORD | 0xC)
#define TAG_PREVOSMINORVER      (TAG_TYPE_DWORD | 0xD)
#define TAG_PREVOSPLATFORMID    (TAG_TYPE_DWORD | 0xE)
#define TAG_PREVOSBUILDNO       (TAG_TYPE_DWORD | 0xF)
#define TAG_PROBLEMSEVERITY     (TAG_TYPE_DWORD | 0x10)
#define TAG_LANGID              (TAG_TYPE_DWORD | 0x11)
#define TAG_VER_LANGUAGE        (TAG_TYPE_DWORD | 0x12)
#define TAG_ENGINE              (TAG_TYPE_DWORD | 0x14)
#define TAG_HTMLHELPID          (TAG_TYPE_DWORD | 0x15)
#define TAG_INDEX_FLAGS         (TAG_TYPE_DWORD | 0x16)
#define TAG_FLAGS               (TAG_TYPE_DWORD | 0x17)
#define TAG_DATA_VALUETYPE      (TAG_TYPE_DWORD | 0x18)
#define TAG_DATA_DWORD          (TAG_TYPE_DWORD | 0x19)
#define TAG_LAYER_TAGID         (TAG_TYPE_DWORD | 0x1A)
#define TAG_MSI_TRANSFORM_TAGID (TAG_TYPE_DWORD | 0x1B)
#define TAG_LINKER_VERSION      (TAG_TYPE_DWORD | 0x1C)
#define TAG_LINK_DATE           (TAG_TYPE_DWORD | 0x1D)
#define TAG_UPTO_LINK_DATE      (TAG_TYPE_DWORD | 0x1E)
#define TAG_OS_SERVICE_PACK     (TAG_TYPE_DWORD | 0x1F)
#define TAG_FLAG_TAGID          (TAG_TYPE_DWORD | 0x20)
#define TAG_RUNTIME_PLATFORM    (TAG_TYPE_DWORD | 0x21)
#define TAG_OS_SKU              (TAG_TYPE_DWORD | 0x22)
#define TAG_OS_PLATFORM         (TAG_TYPE_DWORD | 0x23)
#define TAG_APP_NAME_RC_ID      (TAG_TYPE_DWORD | 0x24)
#define TAG_VENDOR_NAME_RC_ID   (TAG_TYPE_DWORD | 0x25)
#define TAG_SUMMARY_MSG_RC_ID   (TAG_TYPE_DWORD | 0x26)
#define TAG_VISTA_SKU           (TAG_TYPE_DWORD | 0x27)
#define TAG_DESCRIPTION_RC_ID   (TAG_TYPE_DWORD | 0x28)
#define TAG_PARAMETER1_RC_ID    (TAG_TYPE_DWORD | 0x29)
#define TAG_TAGID               (TAG_TYPE_DWORD | 0x801)

#define TAG_TIME                (TAG_TYPE_QWORD | 0x1)
#define TAG_BIN_FILE_VERSION    (TAG_TYPE_QWORD | 0x2)
#define TAG_BIN_PRODUCT_VERSION (TAG_TYPE_QWORD | 0x3)
#define TAG_MODTIME             (TAG_TYPE_QWORD | 0x4)
#define TAG_FLAG_MASK_KERNEL    (TAG_TYPE_QWORD | 0x5)
#define TAG_UPTO_BIN_PRODUCT_VERSION (TAG_TYPE_QWORD | 0x6)
#define TAG_DATA_QWORD          (TAG_TYPE_QWORD | 0x7)
#define TAG_FLAG_MASK_USER      (TAG_TYPE_QWORD | 0x8)
#define TAG_FLAGS_NTVDM1        (TAG_TYPE_QWORD | 0x9)
#define TAG_FLAGS_NTVDM2        (TAG_TYPE_QWORD | 0xA)
#define TAG_FLAGS_NTVDM3        (TAG_TYPE_QWORD | 0xB)
#define TAG_FLAG_MASK_SHELL     (TAG_TYPE_QWORD | 0xC)
#define TAG_UPTO_BIN_FILE_VERSION (TAG_TYPE_QWORD | 0xD)
#define TAG_FLAG_MASK_FUSION    (TAG_TYPE_QWORD | 0xE)
#define TAG_FLAG_PROCESSPARAM   (TAG_TYPE_QWORD | 0xF)
#define TAG_FLAG_LUA            (TAG_TYPE_QWORD | 0x10)
#define TAG_FLAG_INSTALL        (TAG_TYPE_QWORD | 0x11)

#define TAG_NAME                (TAG_TYPE_STRINGREF | 0x1)
#define TAG_DESCRIPTION         (TAG_TYPE_STRINGREF | 0x2)
#define TAG_MODULE              (TAG_TYPE_STRINGREF | 0x3)
#define TAG_API                 (TAG_TYPE_STRINGREF | 0x4)
#define TAG_VENDOR              (TAG_TYPE_STRINGREF | 0x5)
#define TAG_APP_NAME            (TAG_TYPE_STRINGREF | 0x6)
#define TAG_COMMAND_LINE        (TAG_TYPE_STRINGREF | 0x8)
#define TAG_COMPANY_NAME        (TAG_TYPE_STRINGREF | 0x9)
#define TAG_DLLFILE             (TAG_TYPE_STRINGREF | 0xA)
#define TAG_WILDCARD_NAME       (TAG_TYPE_STRINGREF | 0xB)
#define TAG_PRODUCT_NAME        (TAG_TYPE_STRINGREF | 0x10)
#define TAG_PRODUCT_VERSION     (TAG_TYPE_STRINGREF | 0x11)
#define TAG_FILE_DESCRIPTION    (TAG_TYPE_STRINGREF | 0x12)
#define TAG_FILE_VERSION        (TAG_TYPE_STRINGREF | 0x13)
#define TAG_ORIGINAL_FILENAME   (TAG_TYPE_STRINGREF | 0x14)
#define TAG_INTERNAL_NAME       (TAG_TYPE_STRINGREF | 0x15)
#define TAG_LEGAL_COPYRIGHT     (TAG_TYPE_STRINGREF | 0x16)
#define TAG_16BIT_DESCRIPTION   (TAG_TYPE_STRINGREF | 0x17)
#define TAG_APPHELP_DETAILS     (TAG_TYPE_STRINGREF | 0x18)
#define TAG_LINK_URL            (TAG_TYPE_STRINGREF | 0x19)
#define TAG_LINK_TEXT           (TAG_TYPE_STRINGREF | 0x1A)
#define TAG_APPHELP_TITLE       (TAG_TYPE_STRINGREF | 0x1B)
#define TAG_APPHELP_CONTACT     (TAG_TYPE_STRINGREF | 0x1C)
#define TAG_SXS_MANIFEST        (TAG_TYPE_STRINGREF | 0x1D)
#define TAG_DATA_STRING         (TAG_TYPE_STRINGREF | 0x1E)
#define TAG_MSI_TRANSFORM_FILE  (TAG_TYPE_STRINGREF | 0x1F)
#define TAG_16BIT_MODULE_NAME   (TAG_TYPE_STRINGREF | 0x20)
#define TAG_LAYER_DISPLAYNAME   (TAG_TYPE_STRINGREF | 0x21)
#define TAG_COMPILER_VERSION    (TAG_TYPE_STRINGREF | 0x22)
#define TAG_ACTION_TYPE         (TAG_TYPE_STRINGREF | 0x23)
#define TAG_EXPORT_NAME         (TAG_TYPE_STRINGREF | 0x24)

#define TAG_DATABASE            (TAG_TYPE_LIST | 0x1)
#define TAG_LIBRARY             (TAG_TYPE_LIST | 0x2)
#define TAG_INEXCLUDE           (TAG_TYPE_LIST | 0x3)
#define TAG_SHIM                (TAG_TYPE_LIST | 0x4)
#define TAG_PATCH               (TAG_TYPE_LIST | 0x5)
#define TAG_APP                 (TAG_TYPE_LIST | 0x6)
#define TAG_EXE                 (TAG_TYPE_LIST | 0x7)
#define TAG_MATCHING_FILE       (TAG_TYPE_LIST | 0x8)
#define TAG_SHIM_REF            (TAG_TYPE_LIST | 0x9)
#define TAG_PATCH_REF           (TAG_TYPE_LIST | 0xA)
#define TAG_LAYER               (TAG_TYPE_LIST | 0xB)
#define TAG_FILE                (TAG_TYPE_LIST | 0xC)
#define TAG_APPHELP             (TAG_TYPE_LIST | 0xD)
#define TAG_LINK                (TAG_TYPE_LIST | 0xE)
#define TAG_DATA                (TAG_TYPE_LIST | 0xF)
#define TAG_MSI_TRANSFORM       (TAG_TYPE_LIST | 0x10)
#define TAG_MSI_TRANSFORM_REF   (TAG_TYPE_LIST | 0x11)
#define TAG_MSI_PACKAGE         (TAG_TYPE_LIST | 0x12)
#define TAG_FLAG                (TAG_TYPE_LIST | 0x13)
#define TAG_MSI_CUSTOM_ACTION   (TAG_TYPE_LIST | 0x14)
#define TAG_FLAG_REF            (TAG_TYPE_LIST | 0x15)
#define TAG_ACTION              (TAG_TYPE_LIST | 0x16)
#define TAG_LOOKUP              (TAG_TYPE_LIST | 0x17)
#define TAG_STRINGTABLE         (TAG_TYPE_LIST | 0x801)
#define TAG_INDEXES             (TAG_TYPE_LIST | 0x802)
#define TAG_INDEX               (TAG_TYPE_LIST | 0x803)

#define TAG_PATCH_BITS          (TAG_TYPE_BINARY | 0x2)
#define TAG_FILE_BITS           (TAG_TYPE_BINARY | 0x3)
#define TAG_EXE_ID              (TAG_TYPE_BINARY | 0x4)
#define TAG_DATA_BITS           (TAG_TYPE_BINARY | 0x5)
#define TAG_MSI_PACKAGE_ID      (TAG_TYPE_BINARY | 0x6)
#define TAG_DATABASE_ID         (TAG_TYPE_BINARY | 0x7)
#define TAG_INDEX_BITS          (TAG_TYPE_BINARY | 0x801)
#define TAG_PATCH_BITS          (TAG_TYPE_BINARY | 0x2)
#define TAG_FILE_BITS           (TAG_TYPE_BINARY | 0x3)
#define TAG_EXE_ID              (TAG_TYPE_BINARY | 0x4)
#define TAG_DATA_BITS           (TAG_TYPE_BINARY | 0x5)
#define TAG_MSI_PACKAGE_ID      (TAG_TYPE_BINARY | 0x6)
#define TAG_DATABASE_ID         (TAG_TYPE_BINARY | 0x7)
#define TAG_INDEX_BITS          (TAG_TYPE_BINARY | 0x801)

#define TAG_STRINGTABLE_ITEM    (TAG_TYPE_STRING | 0x801)
