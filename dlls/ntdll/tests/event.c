/*
 * Unit test suite for event functions
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2005 Vitaliy Margolen
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

#include "ntdll_test.h"
#include "winternl.h"
#include "winnt.h"

static NTSTATUS (WINAPI *pNtCreateEvent) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN, BOOLEAN);
static NTSTATUS (WINAPI *pNtPulseEvent)  ( HANDLE, PULONG );
static NTSTATUS (WINAPI *pNtQueryEvent)  ( HANDLE, EVENT_INFORMATION_CLASS, PVOID, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtClose)       ( HANDLE );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );

START_TEST(event)
{
    HANDLE Event;
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    EVENT_BASIC_INFORMATION info;
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    static const WCHAR eventName[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t','E','v','e','n','t',0};

    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pNtCreateEvent = (void *)GetProcAddress(hntdll, "NtCreateEvent");
    pNtQueryEvent  = (void *)GetProcAddress(hntdll, "NtQueryEvent");
    pNtPulseEvent  = (void *)GetProcAddress(hntdll, "NtPulseEvent");
    pNtClose       = (void *)GetProcAddress(hntdll, "NtClose");
    pRtlInitUnicodeString = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");

    pRtlInitUnicodeString(&str, eventName);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, 0, 0);
    ok( status == STATUS_SUCCESS, "NtCreateEvent failed %08x\n", status );

    status = pNtPulseEvent(Event, 0);
    todo_wine ok( status == STATUS_SUCCESS, "NtPulseEvent failed %08x\n", status );

    status = pNtQueryEvent(Event, EventBasicInformation, &info, sizeof(info), NULL);
    todo_wine ok( status == STATUS_SUCCESS, "NtQueryEvent failed %08x\n", status );

    status = pNtClose(Event);
    ok( status == STATUS_SUCCESS, "NtClose failed %08x\n", status );
}
