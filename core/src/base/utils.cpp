#include "utils.h"
#include <android/api-level.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

namespace poros {

    static int android_api_level = -1;

    int GetDeviceApiLevel() {
        if (android_api_level < 0) {
            android_api_level = android_get_device_api_level();
        }
        return android_api_level;
    }

    bool IsValidPtr(const void* p) {
        if (!p) {
            return false;
        }
        size_t len = sizeof(int32_t);
        bool ret = true;
        int fd = open("/dev/random", O_WRONLY);
        if (fd != -1) {
            if (write(fd, p, len) < 0) {
                ret = false;
            }
            close(fd);
        }
        else {
            ret = false;
        }
        return ret;
    }

    void* GetModuleBaseAddress(pid_t pid, const char* module_name) {
        FILE* fp;
        long addr = 0;
        char* pch;
        char filename[32];
        char line[1024];

        if (pid < 0) {
            snprintf(filename, sizeof(filename), "/proc/self/maps");
        } else {
            snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
        }

        fp = fopen(filename, "r");

        if (fp != NULL) {
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, module_name)) {
                    pch = strtok(line, "-");
                    addr = strtoul(pch, NULL, 16);
                    if (addr == 0x8000)
                        addr = 0;
                    break;
                }
            }

            fclose(fp);
        }

        return reinterpret_cast<void*>(addr);
    }


}