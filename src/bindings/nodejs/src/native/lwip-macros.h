#ifndef LWIP_MACROS
#define LWIP_MACROS

#include "lwip/tcpip.h"

#include <functional>

err_t typed_tcpip_callback(std::function<void()> callback)
{
    auto cb = new std::function<void()>;
    *cb = callback;

    return tcpip_callback(
        [](void* ctx) {
            auto cb = reinterpret_cast<std::function<void()>*>(ctx);
            (*cb)();
            delete cb;
        },
        cb);
}

void ts_pbuf_free(pbuf* p)
{
    tcpip_callback([](void* p) { pbuf_free(reinterpret_cast<pbuf*>(p)); }, p);
}

#endif