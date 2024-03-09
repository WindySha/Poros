//
// Created by windysha on 2023/2/6.
//

#include "got_hook.h"
#include "../dlfcn/fake_dlfcn.h"
#include <jni.h>
#include <memory>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <android/log.h>
#include <linux/elf.h>
#include <sys/mman.h>
#include"utils.h"


#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym  Elf64_Sym
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym  Elf32_Sym
#endif

namespace poros {

    int startGotHook(const char* callee_elf, const char* caller_elf, const char* symbol, void* replace, void** result) {
        void* libbinder_handle = fake_dlopen(callee_elf);
        void* original_func_addr = fake_dlsym(
            libbinder_handle, symbol);
        fake_dlclose(libbinder_handle);
        *result = original_func_addr;

        if (original_func_addr == nullptr) {
            return -1;
        }

        void* base_addr = poros::GetModuleBaseAddress(-1, caller_elf);

        int fd;
        fd = open(caller_elf, O_RDONLY);
        if (-1 == fd) {
            return -1;
        }
        Elf_Ehdr ehdr;
        read(fd, &ehdr, sizeof(Elf_Ehdr));

        unsigned long shdr_addr = ehdr.e_shoff;
        int shnum = ehdr.e_shnum;
        int shent_size = ehdr.e_shentsize;
        unsigned long stridx = ehdr.e_shstrndx;

        Elf_Shdr shdr;
        lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
        read(fd, &shdr, shent_size);

        char* string_table = (char*)malloc(shdr.sh_size);
        lseek(fd, shdr.sh_offset, SEEK_SET);
        read(fd, string_table, shdr.sh_size);
        lseek(fd, shdr_addr, SEEK_SET);

        int i;
        uintptr_t out_addr = 0;
        uintptr_t out_size = 0;
        uintptr_t got_item = 0;
        int32_t got_found = 0;

        for (i = 0; i < shnum; i++) {
            read(fd, &shdr, shent_size);
            if (shdr.sh_type == SHT_PROGBITS) {
                int name_idx = shdr.sh_name;
                auto sh_name_str = &(string_table[name_idx]);
                if (strcmp(sh_name_str, ".got.plt") == 0 || strcmp(sh_name_str, ".got") == 0) {
                    out_addr = (uintptr_t)base_addr + shdr.sh_addr;
                    out_size = shdr.sh_size;

                    for (int j = 0; j < out_size; j += 4) {
                        auto addr = (uintptr_t)(out_addr + j);
                        got_item = *(uintptr_t*)(addr);
                        if (got_item == (uintptr_t)original_func_addr) {
                            got_found = 1;

                            void* address_page = (void*)(addr & PAGE_MASK);
                            mprotect(address_page, PAGE_SIZE, PROT_READ | PROT_WRITE);

                            *(void**)(addr) = (void*)replace;
                            break;
                        }
                    }
                    if (got_found)
                        break;
                }
            }
        }
        if (string_table) {
            free(string_table);
            string_table = nullptr;
        }
        close(fd);

        return 0;
    }

}