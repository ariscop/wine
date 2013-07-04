/*
 * Unit test suite for job objects
 *
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

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/test.h"

HMODULE hntdll;

DWORD getProcess(PHANDLE handle) {
	STARTUPINFO startup = {};
    PROCESS_INFORMATION info = {};

	if(!CreateProcessA("C:\\windows\\notepad.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info))
		return 0;

	CloseHandle(info.hThread);
    if(handle)
        *handle = info.hProcess;
    
	return info.dwProcessId;
};

typedef struct _JOBOBJECT_ASSOCIATE_COMPLETION_PORT {
  PVOID  CompletionKey;
  HANDLE CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT, *PJOBOBJECT_ASSOCIATE_COMPLETION_PORT;

static NTSTATUS (WINAPI *pNtCreateJobObject)( PHANDLE handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr );
static NTSTATUS (WINAPI *pNtSetInformationJobObject)( HANDLE handle, JOBOBJECTINFOCLASS klass, PVOID info, ULONG len );
static NTSTATUS (WINAPI *pNtAssignProcessToJobObject)( HANDLE job, HANDLE process );
//NTSTATUS (WINAPI *pNtTerminateJobObject)( HANDLE job, NTSTATUS status );

static void test_completion_response(HANDLE IOPort, DWORD eKey, ULONG_PTR eVal, LPOVERLAPPED eOverlap)
{
    DWORD CompletionKey, ret;
	ULONG_PTR CompletionValue;
	LPOVERLAPPED Overlapped;
    
    ret = GetQueuedCompletionStatus(IOPort, &CompletionKey, &CompletionValue, &Overlapped, 0);
    ok(ret, "GetQueuedCompletionStatus failed: %x\n", GetLastError());
    ok(eKey == CompletionKey &&
       eVal == CompletionValue &&
       eOverlap == Overlapped,
        "Unexpected completion event: %x, %p, %p\n", CompletionKey, (void*)CompletionValue, (void*)Overlapped);
}

static void test_completion(void) {
	JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
	DWORD process_1;
	DWORD process_2;
	HANDLE hprocess_1;
	HANDLE hprocess_2;
	HANDLE JobObject;
	HANDLE IOPort;

	NTSTATUS ret;

	IOPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	ok(IOPort != INVALID_HANDLE_VALUE, "CreateIoCompletionPort failed: %x", GetLastError());

	ret = pNtCreateJobObject(&JobObject, GENERIC_ALL, NULL);
	ok(ret == STATUS_SUCCESS, "NtCreateJobObject failed: %x\n", ret);

	Port.CompletionKey = JobObject;
	Port.CompletionPort = IOPort;
	ret = pNtSetInformationJobObject(JobObject, JobObjectAssociateCompletionPortInformation,	&Port, sizeof(Port));
	ok(ret == STATUS_SUCCESS, "NtCreateJobObject failed: %x\n", ret);
	
	process_1 = getProcess(&hprocess_1);
    ok(process_1, "CreateProcess failed: %x\n", GetLastError());
    
    process_2 = getProcess(&hprocess_2);
    ok(process_2, "CreateProcess failed: %x\n", GetLastError());
    
	ret = pNtAssignProcessToJobObject(JobObject, hprocess_1);
	ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject failed: %x\n", ret);
    
	ret = pNtAssignProcessToJobObject(JobObject, hprocess_2);
	ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject failed: %x\n", ret);
	
    ok(TerminateProcess(hprocess_1, 1), "TerminateProcess failed: %x\n", GetLastError());
    ok(TerminateProcess(hprocess_2, 2), "TerminateProcess failed: %x\n", GetLastError());
    
    Sleep(1000);
    
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process_1);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process_2);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)process_1);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)process_2);
    test_completion_response(IOPort, 0x4, (ULONG_PTR)JobObject, 0);

}

START_TEST(job)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    
    pNtCreateJobObject = (void*)GetProcAddress(hntdll, "NtCreateJobObject");
	pNtSetInformationJobObject = (void*)GetProcAddress(hntdll, "NtSetInformationJobObject");
	pNtAssignProcessToJobObject = (void*)GetProcAddress(hntdll, "NtAssignProcessToJobObject");
	//pNtTerminateJobObject = (void*)GetProcAddress(hntdll, "NtTerminateJobObject");
    
    test_completion();
}
