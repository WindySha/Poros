//
// Created by WindySha
//

#include <unistd.h>
#include "ThreadLooper.h"
#include <thread>

namespace poros {
    ThreadLooper::ThreadLooper(MessageHandler handler) : messageHandler(handler) {
        pipe(f_pipe);
        InitAlooper();
        ALooper_addFd(looper, f_pipe[0], 1, ALOOPER_EVENT_INPUT, ThreadLooper::HandleMessage,
            this);
    }

    void ThreadLooper::InitAlooper() {
        looper = ALooper_forThread();
        if (looper == nullptr) {
            looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        }
    }

    int ThreadLooper::HandleMessage(int fd, int events, void* data) {
        auto threadLooper = reinterpret_cast<ThreadLooper*>(data);
        void* param;
        read(threadLooper->f_pipe[0], &param, sizeof(&param));
        return threadLooper->messageHandler(param);
    }

    void ThreadLooper::SendMessage(void* param) {
        write(f_pipe[1], &param, sizeof(&param));
    }

    void ThreadLooper::SendMessageDelay(void* param, uint64_t delay_millis) {
        if (delay_millis <= 0) {
            SendMessage(param);
        }
        else {
            std::thread thread([this, delay_millis, param]() {
                usleep(delay_millis * 1000);
                this->SendMessage(param);
                });
            thread.detach();
        }
    }

    ThreadLooper::~ThreadLooper() {
        if (looper != nullptr) {
            ALooper_removeFd(looper, f_pipe[0]);
            looper = nullptr;
        }
        if (f_pipe[0] != -1) {
            close(f_pipe[0]);
        }
        if (f_pipe[1] != -1) {
            close(f_pipe[1]);
        }
    }
}