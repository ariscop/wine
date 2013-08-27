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
#include <stdint.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/test.h"

HMODULE hntdll;

static DWORD getProcess(PHANDLE handle) {
	STARTUPINFO startup = {};
    PROCESS_INFORMATION info = {};

	if(!CreateProcessA(NULL, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info)) {
        ok(FALSE, "CreateProcess: %x\n", GetLastError());
		return 0;
    }

	CloseHandle(info.hThread);
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
static NTSTATUS (WINAPI *pNtIsProcessInJob)( HANDLE process, HANDLE job );
static NTSTATUS (WINAPI *pNtTerminateJobObject)( HANDLE job, NTSTATUS status );

static void test_completion_response(HANDLE IOPort, DWORD eKey, ULONG_PTR eVal, LPOVERLAPPED eOverlap)
{
    DWORD CompletionKey, ret;
	ULONG_PTR CompletionValue;
	LPOVERLAPPED Overlapped;

    ret = GetQueuedCompletionStatus(IOPort, &CompletionKey, &CompletionValue, &Overlapped, 0);
    ok(ret, "GetQueuedCompletionStatus: %x\n", GetLastError());
    if(ret)
        ok(eKey == CompletionKey &&
            eVal == CompletionValue &&
            eOverlap == Overlapped,
        "Unexpected completion event: %x, %p, %p\n", CompletionKey, (void*)CompletionValue, (void*)Overlapped);
}

static void test_completion(void) {
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
    intptr_t process[4];
    HANDLE hprocess[4];
    HANDLE JobObject;
    HANDLE IOPort;
    NTSTATUS ret;

    IOPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(IOPort != INVALID_HANDLE_VALUE, "CreateIoCompletionPort: %x", GetLastError());

    ret = pNtCreateJobObject(&JobObject, GENERIC_ALL, NULL);
    ok(ret == STATUS_SUCCESS, "NtCreateJobObject: %x\n", ret);

    Port.CompletionKey = JobObject;
    Port.CompletionPort = IOPort;
    ret = pNtSetInformationJobObject(JobObject, JobObjectAssociateCompletionPortInformation,	&Port, sizeof(Port));
    ok(ret == STATUS_SUCCESS, "NtCreateJobObject: %x\n", ret);

    process[0] = getProcess(&hprocess[0]);
    process[1] = getProcess(&hprocess[1]);

    ret = pNtIsProcessInJob(hprocess[0], JobObject);
    ok(ret == STATUS_PROCESS_NOT_IN_JOB, "NtIsProcessInJob: expected STATUS_PROCESS_IN_JOB, got %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, hprocess[0]);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ret = pNtIsProcessInJob(hprocess[0], JobObject);
    ok(ret == STATUS_PROCESS_IN_JOB, "NtIsProcessInJob: expected STATUS_PROCESS_IN_JOB, got %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, hprocess[1]);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ok(TerminateProcess(hprocess[0], 1), "TerminateProcess: %x\n", GetLastError());
    Sleep(1000);
    ok(TerminateProcess(hprocess[1], 2), "TerminateProcess: %x\n", GetLastError());
    Sleep(1000);

    ret = pNtIsProcessInJob(hprocess[0], JobObject);
    todo_wine ok(ret == STATUS_PROCESS_IN_JOB, "NtIsProcessInJob: expected STATUS_PROCESS_IN_JOB, got %x\n", ret);

    process[2] = getProcess(&hprocess[2]);
    process[3] = getProcess(&hprocess[3]);

    ret = pNtAssignProcessToJobObject(JobObject, hprocess[2]);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, hprocess[3]);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ret = pNtTerminateJobObject( JobObject, 5 );
    ok(ret == STATUS_SUCCESS, "NtTerminateJobObject: %x\n", GetLastError());

    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[0]);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[1]);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[0]);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[1]);
    test_completion_response(IOPort, 0x4, (ULONG_PTR)JobObject, 0);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[2]);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)process[3]);
    test_completion_response(IOPort, 0x4, (ULONG_PTR)JobObject, 0);

}

static void is_zombie(void)
{
    WCHAR val[32] = {0};
    static const WCHAR envName[] = { 'T','E','S','T',0 };
    static const WCHAR envVal[] = { 'J','o','b','O','b','j','e','c','t',0 };

    GetEnvironmentVariableW(envName, val, 32);

    if(lstrcmpW(envVal, val) == 0)
        /* Wait forever, we've been created for a process handle */
        while(1) Sleep(INFINITE);

    SetEnvironmentVariableW(envName, envVal);
}


START_TEST(job)
{
    HMODULE hntdll;

    is_zombie();

    hntdll = GetModuleHandleA("ntdll.dll");
    pNtCreateJobObject = (void*)GetProcAddress(hntdll, "NtCreateJobObject");
	pNtSetInformationJobObject = (void*)GetProcAddress(hntdll, "NtSetInformationJobObject");
	pNtAssignProcessToJobObject = (void*)GetProcAddress(hntdll, "NtAssignProcessToJobObject");
	pNtIsProcessInJob = (void*)GetProcAddress(hntdll, "NtIsProcessInJob");
	pNtTerminateJobObject = (void*)GetProcAddress(hntdll, "NtTerminateJobObject");

    test_completion();
}
