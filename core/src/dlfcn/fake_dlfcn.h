//
// Created by windysha on 2022
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *fake_dlopen(const char *filename);

void *fake_dlsym(void *handle, const char *name);

void *fake_dlsym_symtab(void *handle, const char *name);

void fake_dlclose(void *handle);

void *get_base_address(const char* filepath);

#ifdef __cplusplus
}
#endif