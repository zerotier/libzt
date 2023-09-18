#ifndef LWIP_MACROS
#define LWIP_MACROS

#include <functional>
#include "lwip/tcpip.h"

err_t typed_tcpip_callback(std::function<void()> callback)
{
    auto cb = new std::function<void()>;
    *cb = callback;

    return tcpip_callback([](void* ctx) {
        auto cb = (std::function<void()>*) ctx;
        (*cb)();
        delete cb;
    }, cb);
}

#define FREE_PBUF(PTR)                                                                                                 \
    do {                                                                                                               \
        tcpip_callback([](void* p) { pbuf_free((pbuf*)p); }, PTR);                                                     \
    } while (0)

#endif