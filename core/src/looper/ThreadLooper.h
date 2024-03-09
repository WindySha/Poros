//
// Created by WindySha
//

#pragma once

#include <android/looper.h>
#include <stdint.h>
#include <memory>
#include <functional>

namespace poros {

    typedef std::function<int(void*)> MessageHandler;
    class ThreadLooper {
    public:
        ThreadLooper(MessageHandler handler);
        void SendMessage(void* param);
        void SendMessageDelay(void* param, uint64_t delay_millis);

        ~ThreadLooper();

    private:
        void InitAlooper();
        static int HandleMessage(int fd, int events, void* data);

        int f_pipe[2];
        ALooper* looper;
        const MessageHandler messageHandler;
    };
}
