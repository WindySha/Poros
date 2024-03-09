#pragma once

#include <sys/types.h>

namespace poros {

    static inline bool Is64BitRuntime() {
        return (sizeof(void*) == 8);
    }

    int GetDeviceApiLevel();

    bool IsValidPtr(const void* p);

    void* GetModuleBaseAddress(pid_t pid, const char* module_name);


}