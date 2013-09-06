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

static void test_completion(int argc, char **argv) {
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
    PROCESS_INFORMATION pi[4];
    STARTUPINFO si[4] = {{0}};
    HANDLE JobObject;
    HANDLE IOPort;
    NTSTATUS ret;
    char cmdline[MAX_PATH];

    sprintf(cmdline, "%s %s %s", argv[0], argv[1], "job_member");

    IOPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(IOPort != INVALID_HANDLE_VALUE, "CreateIoCompletionPort: %x", GetLastError());

    ret = pNtCreateJobObject(&JobObject, GENERIC_ALL, NULL);
    ok(ret == STATUS_SUCCESS, "NtCreateJobObject: %x\n", ret);

    Port.CompletionKey = JobObject;
    Port.CompletionPort = IOPort;
    ret = pNtSetInformationJobObject(JobObject, JobObjectAssociateCompletionPortInformation, &Port, sizeof(Port));
    ok(ret == STATUS_SUCCESS, "NtCreateJobObject: %x\n", ret);

    ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si[0], &pi[0]),
        "CreateProcess: %x\n", GetLastError());
    ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si[1], &pi[1]),
        "CreateProcess: %x\n", GetLastError());

    if(pNtIsProcessInJob)
        todo_wine ok(pNtIsProcessInJob(pi[0].hProcess, JobObject) == STATUS_PROCESS_NOT_IN_JOB, 
            "NtIsProcessInJob: expected STATUS_PROCESS_NOT_IN_JOB, got %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, pi[0].hProcess);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    if(pNtIsProcessInJob)
        todo_wine ok(pNtIsProcessInJob(pi[0].hProcess, JobObject) == STATUS_PROCESS_IN_JOB,
            "NtIsProcessInJob: expected STATUS_PROCESS_IN_JOB, got %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, pi[1].hProcess);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);
 
    ok(TerminateProcess(pi[0].hProcess, 0), "TerminateProcess: %x\n", GetLastError());
    winetest_wait_child_process(pi[0].hProcess);
    ok(TerminateProcess(pi[1].hProcess, 0), "TerminateProcess: %x\n", GetLastError());
    winetest_wait_child_process(pi[1].hProcess);

    if(pNtIsProcessInJob)
        todo_wine ok(pNtIsProcessInJob(pi[0].hProcess, JobObject) == STATUS_PROCESS_IN_JOB,
            "NtIsProcessInJob: expected STATUS_PROCESS_IN_JOB, got %x\n", ret);

    ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si[2], &pi[2]),
        "CreateProcess: %x\n", GetLastError());
    ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si[3], &pi[3]),
        "CreateProcess: %x\n", GetLastError());

    ret = pNtAssignProcessToJobObject(JobObject, pi[2].hProcess);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ret = pNtAssignProcessToJobObject(JobObject, pi[3].hProcess);
    ok(ret == STATUS_SUCCESS, "NtAssignProcessToJobObject: %x\n", ret);

    ret = pNtTerminateJobObject( JobObject, 5 );
    ok(ret == STATUS_SUCCESS, "NtTerminateJobObject: %x\n", GetLastError());

    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[0].dwProcessId);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[1].dwProcessId);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[0].dwProcessId);
    test_completion_response(IOPort, 0x7, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[1].dwProcessId);
    test_completion_response(IOPort, 0x4, (ULONG_PTR)JobObject, 0);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[2].dwProcessId);
    test_completion_response(IOPort, 0x6, (ULONG_PTR)JobObject, (LPOVERLAPPED)pi[3].dwProcessId);
    test_completion_response(IOPort, 0x4, (ULONG_PTR)JobObject, 0);

    /* in case TerminateJobObject is not implemented */
    TerminateProcess(pi[2].hProcess, 0);
    TerminateProcess(pi[3].hProcess, 0);
}

START_TEST(job)
{
    char **argv;
    int argc;
    HMODULE hntdll;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3) { /* child, wait till we're terminated */
        Sleep(30000);
        return;
    }

    hntdll = GetModuleHandleA("ntdll.dll");
    pNtCreateJobObject = (void*)GetProcAddress(hntdll, "NtCreateJobObject");
    pNtSetInformationJobObject = (void*)GetProcAddress(hntdll, "NtSetInformationJobObject");
    pNtAssignProcessToJobObject = (void*)GetProcAddress(hntdll, "NtAssignProcessToJobObject");
    pNtIsProcessInJob = (void*)GetProcAddress(hntdll, "NtIsProcessInJob");
    pNtTerminateJobObject = (void*)GetProcAddress(hntdll, "NtTerminateJobObject");

    test_completion(argc, argv);
}
