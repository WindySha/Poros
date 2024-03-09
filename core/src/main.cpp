//
// Created by Windysha.
//

#include "injection.h"

__attribute__((constructor))
extern "C" int entry_function() {
    poros::DoInjection(nullptr);
    return 0;
}