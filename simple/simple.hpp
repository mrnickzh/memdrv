#ifdef __cplusplus
extern "C"
{
#endif

#include <ntifs.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntstrsafe.h>
#include <ntimage.h>
#include <wdm.h>
#include <wsk.h>

#ifdef __cplusplus
}
#endif


namespace main {
    extern HANDLE pid;
    extern bool loadingDone;
    extern PVOID baseAddress;
    extern PVOID endAddress;
}