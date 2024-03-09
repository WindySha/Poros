//
// Created by Windysha.
//

#pragma once

typedef void (*OnAtraceUpdateCallback)();

namespace poros {
    void HookAtraceFunctions(OnAtraceUpdateCallback callback);
}