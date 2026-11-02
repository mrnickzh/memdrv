#include "simple.hpp"
#include "shared.hpp"
#include "Util/Memory.hpp"
#include "Impl/WDKImpl.hpp"

#define MY_DRIVER_TAG 'agin'

char hex[] = "0123456789ABCDEF";

namespace main {
    HANDLE pid;
    bool loadingDone = false;
    int loadedModules = 0;
    PVOID baseAddress;
}

PVOID       g_SharedView   = NULL;
PKEVENT     g_Event        = NULL;
HANDLE      g_ThreadHandle = NULL;
PSHARED_MEMORY g_Shared    = NULL;

PEPROCESS g_sourceProc = NULL;

PVOID GetDefaultHeapBase(PEPROCESS Process) {
    PPEB Peb = PsGetProcessPeb(Process);
    if (!Peb) return NULL;

    PVOID heapBase = NULL;
    SIZE_T bytes = 0;

    NTSTATUS status = MmCopyVirtualMemory(
        Process,                       // source process
        (PVOID)((ULONG_PTR)Peb + 0x30),// &Peb->ProcessHeap (x64)
        PsGetCurrentProcess(),         // destination process
        &heapBase,                     // local buffer
        sizeof(PVOID),
        KernelMode,
        &bytes);

    return NT_SUCCESS(status) ? heapBase : NULL;
}

void dmpmem(int len) {
    DWORD64 baseAddr = (DWORD64)main::baseAddress;

    unsigned char *values;
    PVOID pBuffer = ExAllocatePoolZero(NonPagedPoolNx, len, MY_DRIVER_TAG);
    values = (unsigned char*)pBuffer;

    for (int i = 0; i < len; ++i) {
        unsigned char buff;
        DWORD64 addr = baseAddr + i;
        NTSTATUS Status = Memory::ReadProcessMemory(main::pid, (PVOID)addr, &buff, 1);
        if (NT_SUCCESS(Status)) {
            memcpy(&values[i], &buff, 1);
            Log(INFO, "Address [0x%p]: 0x%c%c", addr, hex[buff>>4], hex[buff&0xf]);
        }
        else {
            Log(INFO, "EOP");
            // break;
        }
    }

    ExFreePoolWithTag(pBuffer, MY_DRIVER_TAG);
}

VOID SharedWorkerThread(_In_ PVOID context) {
    UNICODE_STRING sectionName = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\MySharedSection");
    OBJECT_ATTRIBUTES attr;
    InitializeObjectAttributes(&attr, &sectionName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hSection = NULL;
    NTSTATUS status = ZwOpenSection(&hSection, SECTION_ALL_ACCESS, &attr);
    if (!NT_SUCCESS(status)) { Log(INFO, "[-] ZwOpenSection 0x%X", status); }
    
    SIZE_T viewSize = sizeof(SHARED_MEMORY);
    status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &g_SharedView,
                                0, sizeof(SHARED_MEMORY), NULL, &viewSize,
                                ViewUnmap, 0, PAGE_READWRITE);
    ZwClose(hSection);
    if (!NT_SUCCESS(status)) { Log(INFO, "[-] ZwMapViewOfSection 0x%X", status); }

    g_Shared = (PSHARED_MEMORY)g_SharedView;
    if (!g_Shared) { Log(INFO, "[-] SharedView failed"); }

    UNICODE_STRING eventName = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\MyKernelEvent");
    HANDLE hEventHandle = NULL;
    g_Event = IoCreateNotificationEvent(&eventName, &hEventHandle);
    if (!g_Event) { Log(INFO, "[-] IoCreateNotificationEvent 0x%X", status); }

    LARGE_INTEGER timeout;
    timeout.QuadPart = -10 * 1000 * 1000;

    while (true) {
        NTSTATUS waitStatus = KeWaitForSingleObject(
            g_Event,
            Executive,
            KernelMode,
            FALSE,
            &timeout
        );

        if (waitStatus == STATUS_TIMEOUT) {
            continue;
        }

        if (g_Shared->Operation != OpComplete) {
            switch (g_Shared->Operation) {
                case OpPing:
                    g_Shared->Status = STATUS_SUCCESS;
                    break;

                case OpDumpMemory: {
                    if (g_sourceProc == NULL) {
                        PsLookupProcessByProcessId(ULongToHandle(g_Shared->TargetPid), &g_sourceProc);
                    }
                    if (g_sourceProc) {
                        main::pid = ULongToHandle(g_Shared->TargetPid);
                        main::baseAddress = PsGetProcessSectionBaseAddress(g_sourceProc);
                        // DbgPrint("[DUMP] pid: %d baseaddr: %p\n", main::pid, main::baseAddress);
                        dmpmem(1024);
                    } else {
                        g_Shared->Status = STATUS_INVALID_PARAMETER;
                    }
                    break;
                }

                case OpReadMemory: {
                    if (g_sourceProc == NULL) {
                        PsLookupProcessByProcessId(ULongToHandle(g_Shared->TargetPid), &g_sourceProc);
                    }
                    if (g_Shared->Size > 1024) { DbgPrint("[ERROR] cant read more than 1kb\n", main::pid, main::baseAddress); break; }
                    if (g_sourceProc) {
                        main::pid = ULongToHandle(g_Shared->TargetPid);
                        // main::baseAddress = PsGetProcessSectionBaseAddress(sourceProc);
                        main::baseAddress = GetDefaultHeapBase(g_sourceProc);
                        Log(INFO, "[READ] pid: %d addr: %p", main::pid, (PVOID)(g_Shared->Address));

                        // PVOID pBuffer = ExAllocatePoolZero(NonPagedPoolNx, g_Shared->Size, MY_DRIVER_TAG);
                        if (!NT_SUCCESS(Memory::ReadProcessMemory(main::pid, (PVOID)(g_Shared->Address), (PVOID)(g_Shared->Payload), g_Shared->Size))) {
                            Log(INFO, "[WARN] pid: %d, addr: %p, size %d !!! READ FAILED !!!", main::pid, (PVOID)(g_Shared->Address), g_Shared->Size);
                        }
                        // memcpy(g_Shared->Payload, pBuffer, g_Shared->Size);
                        // ExFreePoolWithTag(pBuffer, MY_DRIVER_TAG);

                        } else {
                            g_Shared->Status = STATUS_INVALID_PARAMETER;
                        }
                    break;
                }

                case OpWriteMemory: {
                    if (g_sourceProc == NULL) {
                        PsLookupProcessByProcessId(ULongToHandle(g_Shared->TargetPid), &g_sourceProc);
                    }
                    if (g_Shared->Size > 1024) { DbgPrint("[ERROR] cant write more than 1kb\n", main::pid, main::baseAddress); break; }
                    if (NT_SUCCESS(PsLookupProcessByProcessId(
                            ULongToHandle(g_Shared->TargetPid), &g_sourceProc))) {
                        main::pid = ULongToHandle(g_Shared->TargetPid);
                        // main::baseAddress = PsGetProcessSectionBaseAddress(sourceProc);
                        main::baseAddress = GetDefaultHeapBase(g_sourceProc);
                        Log(INFO, "[WRITE] pid: %d addr: %p", main::pid, (PVOID)(g_Shared->Address));


                        if (!NT_SUCCESS(Memory::WriteProcessMemory(main::pid, (PVOID)(g_Shared->Address), (PVOID)(g_Shared->Payload), g_Shared->Size))) {
                            Log(INFO, "[WARN] pid: %d, addr: %p, size %d !!! WRITE FAILED !!!", main::pid, (PVOID)(g_Shared->Address), g_Shared->Size);
                        }

                        } else {
                            g_Shared->Status = STATUS_INVALID_PARAMETER;
                        }
                    break;
                }

                default:
                    g_Shared->Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
            }

            g_Shared->Operation = OpComplete;
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS CustomDriverEntry(
	_In_ PDRIVER_OBJECT  kdmapperParam1,
	_In_ PUNICODE_STRING kdmapperParam2
)
{
	UNREFERENCED_PARAMETER(kdmapperParam1);
	UNREFERENCED_PARAMETER(kdmapperParam2);

    // ULONG clientPid = (ULONG)kdmapperParam1;

    Log(INFO, "Starting");
    // Log(INFO, "pid %d received", clientPid);

    if (KeGetCurrentIrql() != PASSIVE_LEVEL) return STATUS_INVALID_DEVICE_STATE;

    // if (!clientPid) return STATUS_INVALID_PARAMETER;

    // PEPROCESS clientProc = NULL;
    // NTSTATUS status = PsLookupProcessByProcessId(ULongToHandle(clientPid), &clientProc);
    // if (!NT_SUCCESS(status)) return status;

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    NTSTATUS status = PsCreateSystemThread(&g_ThreadHandle, THREAD_ALL_ACCESS, &objAttr,
                                  NULL, NULL, SharedWorkerThread, NULL);
    if (!NT_SUCCESS(status)) {
        // ObDereferenceObject(clientProc);
        return status;
    }
    ZwClose(g_ThreadHandle);

    Log(INFO, "Done");

	return STATUS_SUCCESS;
}