#pragma once

#define SHARED_SECTION_NAME L"\\BaseNamedObjects\\MySharedSection"
#define SHARED_EVENT_NAME   L"\\BaseNamedObjects\\MyKernelEvent"

typedef enum _SHARED_OP {
    OpPing = 0,
    OpReadMemory,
    OpWriteMemory,
    OpDumpMemory,
    OpComplete
} SHARED_OP;

typedef struct _SHARED_MEMORY {
    volatile SHARED_OP Operation;
    ULONG TargetPid;
    PVOID Address;
    SIZE_T Size;
    NTSTATUS Status;
    UCHAR Payload[1024];
} SHARED_MEMORY, *PSHARED_MEMORY;