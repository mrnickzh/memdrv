#pragma once

#include <ntddk.h>
#include "../simple.hpp"
#include "../log.hpp"
#include "../Impl/WDKImpl.hpp"

namespace Memory {
    NTSTATUS ReadProcessMemory(HANDLE ProcessPid, PVOID Address, PVOID Buffer, SIZE_T Size)
    {
        if (!ProcessPid || !Address || !Buffer || !Size)
            return STATUS_INVALID_PARAMETER;

        PEPROCESS Process = NULL;
        NTSTATUS status = PsLookupProcessByProcessId(ProcessPid, &Process);
        if (!NT_SUCCESS(status))
            return status;

        SIZE_T bytesRead = 0;
        status = MmCopyVirtualMemory(
            Process, Address,
            PsGetCurrentProcess(), Buffer,
            Size, KernelMode, &bytesRead);

        ObDereferenceObject(Process);
        return status;
    }

    NTSTATUS WriteProcessMemory(HANDLE ProcessPid, PVOID Address, PVOID Buffer, SIZE_T Size)
    {
        if (!ProcessPid || !Address || !Buffer || !Size)
            return STATUS_INVALID_PARAMETER;

        PEPROCESS Process = NULL;
        NTSTATUS status = PsLookupProcessByProcessId(ProcessPid, &Process);
        if (!NT_SUCCESS(status))
            return status;

        SIZE_T bytesWritten = 0;
        status = MmCopyVirtualMemory(
            PsGetCurrentProcess(), Buffer,
            Process, Address,
            Size, KernelMode, &bytesWritten);

        ObDereferenceObject(Process);
        return status;
    }
}
