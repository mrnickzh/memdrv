#pragma once

#include <sal.h>
#define _DEBUG

enum LogLevel
{
    INFO,
    ERROR,
    WARN,
    DEBUG,
    DUMP
};

const char* GetLogLevelPrefix(LogLevel level)
{
    switch (level)
    {
        case INFO:
            return "[INFO] ";
        case ERROR:
            return "[ERROR] ";
        case WARN:
            return "[WARN] ";
        case DEBUG:
            return "[DEBUG] ";
        default:
            return "[INFO] ";
    }
}

void Log(LogLevel level, _In_z_ _Printf_format_string_ const char* format, ...)
{
    return;
#ifdef _DEBUG
    const char* levelPrefix = GetLogLevelPrefix(level);

    char msg[1024] = { 0 };
    memcpy(msg, levelPrefix, strlen(levelPrefix));

    va_list vl;
            va_start(vl, format);
    const int n = _vsnprintf(msg + strlen(levelPrefix), sizeof(msg) - strlen(levelPrefix) - 2, format, vl);
            va_end(vl);

    auto index = strlen(levelPrefix) + n;
    msg[index++] = '\r';
    msg[index++] = '\n';
    msg[index++] = '\0';

    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES objAttr;
    RtlInitUnicodeString(&FileName, L"\\DosDevices\\C:\\KernelLog.txt");
    InitializeObjectAttributes(&objAttr, &FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);

    HANDLE handle;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntstatus = ZwCreateFile(&handle,
                                     FILE_APPEND_DATA,
                                     &objAttr, &ioStatusBlock, NULL,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_WRITE | FILE_SHARE_READ,
                                     FILE_OPEN_IF,
                                     FILE_SYNCHRONOUS_IO_NONALERT,
                                     NULL, 0);
    if (NT_SUCCESS(ntstatus))
    {
        size_t cb;
        ntstatus = RtlStringCbLengthA(msg, sizeof(msg), &cb);
        if (NT_SUCCESS(ntstatus))
            ZwWriteFile(handle, NULL, NULL, NULL, &ioStatusBlock, msg, (ULONG)cb, NULL, NULL);
        ZwClose(handle);
    }
#endif
}


void DeleteLogFile()
{
#ifdef _DEBUG
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES objAttr;
    RtlInitUnicodeString(&FileName, L"\\DosDevices\\C:\\KernelLog.txt");
    InitializeObjectAttributes(&objAttr, &FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);

    NTSTATUS ntstatus = ZwDeleteFile(&objAttr);
    if (NT_SUCCESS(ntstatus))
    {

    }
#endif
}