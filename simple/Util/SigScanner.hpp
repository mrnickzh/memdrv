#pragma once

#include <cstdint>
#include <cstddef>
#include <minwindef.h>
#include "../driver.hpp"
#include "../log.hpp"
#include "../Impl/WDKImpl.hpp"
#include "Memory.hpp"

namespace SigScanner{
    DWORD64 FindPattern(DWORD64 base, SIZE_T size, const char* pattern, const char* mask) {
        for (SIZE_T i = 0; i < size; i++) {
            bool found = true;
            for (SIZE_T j = 0; mask[j]; j++) {
                if (mask[j] == 'x' && (BYTE)Memory::readMemory(base + i + j) != (BYTE)pattern[j]) {
                    found = false;
                    if(j > 0) DbgPrint("not equal %d: 0x%llx", j, base + i + j);
                    break;
                }
            }
            if (found) return base + i;
        }
        return 0;
    }

    DWORD64 ScanRegion(DWORD64 base, SIZE_T regionSize, const char* sig, const char* mask) {
        DWORD64 result = FindPattern(base, regionSize, sig, mask);
        if (result) {
            Log(INFO, "0x%llx", result);
        } else {
            Log(INFO, "not found");
        }
        return result;
    }
}