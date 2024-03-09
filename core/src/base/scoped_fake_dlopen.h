#pragma once

#include "../dlfcn/fake_dlfcn.h"

class ScopedFakeDlopen {
public:
    ScopedFakeDlopen(const char* path) {
        handle = fake_dlopen(path);
    }

    ~ScopedFakeDlopen() {
        if (handle != nullptr) {
            fake_dlclose(handle);
        }
    }

    void* get() const {
        return handle;
    }

private:
    void* handle;

    // Disallow copy and assignment.
    ScopedFakeDlopen(const ScopedFakeDlopen&);
    void operator=(const ScopedFakeDlopen&);
};