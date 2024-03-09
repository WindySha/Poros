// Copyright (c) 2016 avs333
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//		of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//		copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//		The above copyright notice and this permission notice shall be included in all
//		copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>
#include <android/log.h>
#include <android/api-level.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <jni.h>
#include "fake_dlfcn.h"


#define ANDROID_N                       24
#define ANDROID_N_1                     25
#define ANDROID_P                       28
#define ANDROID_Q                       29
#define ANDROID_R                       30

#define EXPORTFUNC  __attribute__ ((visibility ("default")))

#define  LOG_TAG "fake_dlfcn"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  , LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)

#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym  Elf64_Sym

static const char *const kSystemLibDir = "/system/lib64/";
static const char *const kOdmLibDir = "/odm/lib64/";
static const char *const kVendorLibDir = "/vendor/lib64/";
static const char *const kApexLibDir = "/apex/com.android.runtime/lib64/";
static const char *const kApexArtNsLibDir = "/apex/com.android.art/lib64/";
static const char *const kLinkerFilePath = "/system/bin/linker64";

#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym  Elf32_Sym

static const char *const kSystemLibDir = "/system/lib/";
static const char *const kOdmLibDir = "/odm/lib/";
static const char *const kVendorLibDir = "/vendor/lib/";
static const char *const kApexLibDir = "/apex/com.android.runtime/lib/";
static const char *const kApexArtNsLibDir = "/apex/com.android.art/lib/";
static const char *const kLinkerFilePath = "/system/bin/linker";
#endif


struct ctx {
    void *load_addr;
    void *dynstr;
    void *dynsym;
    int nsyms;
    off_t bias;
    void *symtab;
    void *strtab;
    int nsymtab;
};

static void *fake_dlopen_internal(const char *libpath, int flags) {
    struct ctx *ctx = 0;
    off_t size;
    int k, fd = -1;
    void *shoff;
    Elf_Ehdr *elf = MAP_FAILED;

#define fatal(fmt, args...) do { LOGE(fmt,##args); goto err_exit; } while(0)

    void *load_addr = get_base_address(libpath);

    if (load_addr == NULL) {
        LOGE("cannot found base address of %s", libpath);
        return NULL;
    }

    /* Now, mmap the same library once again */
    fd = open(libpath, O_RDONLY);
    if (fd < 0) fatal("failed to open %s", libpath);

    size = lseek(fd, 0, SEEK_END);
    if (size <= 0) fatal("lseek() failed for %s", libpath);

    elf = (Elf_Ehdr *) mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    fd = -1;

    if (elf == MAP_FAILED) fatal("mmap() failed for %s", libpath);

    ctx = (struct ctx *) calloc(1, sizeof(struct ctx));
    if (!ctx) fatal("no memory for %s", libpath);

    ctx->load_addr = (void *) load_addr;
    shoff = ((void *) elf) + elf->e_shoff;

    char *section_str = (char *) (((Elf_Shdr *) shoff)[elf->e_shstrndx].sh_offset +
                                  ((size_t) elf));

    off_t bias = -100001;

    for (k = 0; k < elf->e_shnum; k++, shoff += elf->e_shentsize) {

        Elf_Shdr *sh = (Elf_Shdr *) shoff;
        char *sname = sh->sh_name + section_str;
//        LOGD("%s: k=%d shdr=%p type=%x  sname = %s", __func__, k, sh, sh->sh_type, sname);

        switch (sh->sh_type) {

            case SHT_DYNSYM:
                if (ctx->dynsym) fatal("%s: duplicate DYNSYM sections", libpath); /* .dynsym */
                ctx->dynsym = malloc(sh->sh_size);
                if (!ctx->dynsym) fatal("%s: no memory for .dynsym", libpath);
                memcpy(ctx->dynsym, ((void *) elf) + sh->sh_offset, sh->sh_size);
                ctx->nsyms = (sh->sh_size / sh->sh_entsize);
                break;
            case SHT_SYMTAB:
                if (strcmp(sname, ".symtab") == 0) {
                    if (ctx->symtab) fatal("%s: duplicate SYMTAB sections", libpath); /* .dynsym */
                    ctx->symtab = malloc(sh->sh_size);
                    if (!ctx->symtab) fatal("%s: no memory for .symtab", libpath);
                    memcpy(ctx->symtab, ((void *) elf) + sh->sh_offset, sh->sh_size);
                    ctx->nsymtab = (sh->sh_size / sh->sh_entsize);
                }
                break;

            case SHT_STRTAB:
                if (strcmp(sname, ".dynstr") == 0) {
                    if (ctx->dynstr) break;
                    ctx->dynstr = malloc(sh->sh_size);
                    if (!ctx->dynstr) fatal("%s: no memory for .dynstr", libpath);
                    memcpy(ctx->dynstr, ((void *) elf) + sh->sh_offset, sh->sh_size);
                } else if (strcmp(sname, ".strtab") == 0) {
                    if (ctx->strtab) break;
                    ctx->strtab = malloc(sh->sh_size);
                    if (!ctx->strtab) fatal("%s: no memory for .dynstr", libpath);
                    memcpy(ctx->strtab, ((void *) elf) + sh->sh_offset, sh->sh_size);
                }
                break;

            case SHT_PROGBITS:
                if (!ctx->dynstr || !ctx->dynsym) break;
                if (bias == -100001) {
                    bias = (off_t) sh->sh_addr - (off_t) sh->sh_offset;
                    ctx->bias = bias;
                }
                break;
        }
    }
    munmap(elf, size);
    elf = 0;

    if (!ctx->dynstr || !ctx->dynsym) fatal("dynamic sections not found in %s", libpath);

#undef fatal
    return ctx;

    err_exit:
    if (fd >= 0) close(fd);
    if (elf != MAP_FAILED) munmap(elf, size);
    fake_dlclose(ctx);
    return 0;
}

EXPORTFUNC void *fake_dlsym_symtab(void *handle, const char *name) {
    if (handle == NULL) return NULL;
    struct ctx *ctx = (struct ctx *) handle;
    Elf_Sym *sym = (Elf_Sym *) ctx->symtab;
    char *strings = (char *) ctx->strtab;

    //search symtab
    if (ctx->symtab != NULL && ctx->strtab != NULL) {
        for (int i = 0; i < ctx->nsymtab; i++, sym++) {
            if (strcmp(strings + sym->st_name, name) == 0) {
                void *ret = ctx->load_addr + sym->st_value - ctx->bias;
                LOGD("%s in .symtab found at %p", name, ret);
                return ret;
            }
        }
    }
    return 0;
}

EXPORTFUNC void *fake_dlopen(const char *filename) {
    void *handle = NULL;
    if (strlen(filename) > 0 && filename[0] == '/') {
        handle = fake_dlopen_internal(filename, 0);
        if (handle) return handle;
    }

    const char *file_path_prefix[] =
            {kApexArtNsLibDir, kApexLibDir, kSystemLibDir, kVendorLibDir, kOdmLibDir};
    const int size = sizeof(file_path_prefix) / sizeof(file_path_prefix[0]);
    char full_name[512];

    for (int i = 0; i < size; i++) {
        const char *prefix = file_path_prefix[i];
        memset(full_name, 0, sizeof(full_name));
        sprintf(full_name, "%s%s", prefix, filename);
        if (access(full_name, F_OK) != 0) continue;
        handle = fake_dlopen_internal(full_name, 0);
        if (handle) return handle;
    }
    return NULL;
}

EXPORTFUNC void *fake_dlsym(void *handle, const char *name) {
    if (handle == NULL) return NULL;
    int k;
    struct ctx *ctx = (struct ctx *) handle;
    Elf_Sym *sym = (Elf_Sym *) ctx->dynsym;
    char *strings = (char *) ctx->dynstr;

    for (k = 0; k < ctx->nsyms; k++, sym++)
        if (strcmp(strings + sym->st_name, name) == 0) {
            /*  NB: sym->st_value is an offset into the section for relocatables,
            but a VMA for shared libs or exe files, so we have to subtract the bias */
            void *ret = ctx->load_addr + sym->st_value - ctx->bias;
            LOGD("%s in .dynsym found at %p    bias: %ld", name, ret, ctx->bias);
            return ret;
        }

    return fake_dlsym_symtab(handle, name);
}

EXPORTFUNC void fake_dlclose(void *handle) {
    if (handle) {
        struct ctx *ctx = (struct ctx *) handle;
        if (ctx->dynsym) free(ctx->dynsym);    /* we're saving dynsym and dynstr */
        if (ctx->dynstr) free(ctx->dynstr);    /* from library file just in case */
        if (ctx->symtab) free(ctx->symtab);    /* from library file just in case */
        if (ctx->strtab) free(ctx->strtab);    /* from library file just in case */
        free(ctx);
    }
}

EXPORTFUNC void *get_base_address(const char *filepath) {
    FILE *maps;
    char buff[256];
    void *load_addr;
    int found = 0;
    maps = fopen("/proc/self/maps", "r");

    while (fgets(buff, sizeof(buff), maps)) {
        if ((strstr(buff, "r-xp") || strstr(buff, "r--p")) && strstr(buff, filepath)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        LOGE("failed to read load address for %s", filepath);
        fclose(maps);
        return NULL;
    }
    if (sscanf(buff, "%lx", &load_addr) != 1) {
        LOGE("failed to read load address for %s", filepath);
    }

    fclose(maps);
    LOGD("get module base %s: %p", filepath, load_addr);
    return load_addr;
}