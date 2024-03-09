//
// Created by WindySha
//

#pragma once

namespace poros {

    int startGotHook(const char* callee_elf, const char* caller_elf, const char* symbol, void* replace, void** result);

}