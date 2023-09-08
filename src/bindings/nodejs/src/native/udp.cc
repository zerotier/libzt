#include "lwip/udp.h"

#include "ZeroTierSockets.h"
#include "macros.h"

#include <iostream>
#include <napi.h>

namespace UDP {

FUNC_REF(constructor);

CLASS(Socket)
{
  public:
    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

    ~Socket()
    {
        udp_remove(pcb);
        recv_cb.Release();
    }

  private:
    udp_pcb* pcb;

    Napi::ThreadSafeFunction recv_cb;
    METHOD(send_to);
    METHOD(bind);
};

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(Socket, { CLASS_INSTANCE_METHOD(Socket, send_to), CLASS_INSTANCE_METHOD(Socket, bind) });

    *constructor = Napi::Persistent(func);

    exports["UDP"] = func;
    return exports;
}

void lwip_recv_cb(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    auto threadsafe = (Napi::ThreadSafeFunction*)arg;

    std::cout << "received data \n";

    auto cb = [&](Napi::Env env, Napi::Function jsCallback) {
        jsCallback.Call({ Napi::Buffer<char>::Copy(env, (const char*)p->payload, p->len),
                          STRING(ipaddr_ntoa(addr)),
                          NUMBER(port) });

        pbuf_free(p);
    };
    threadsafe->NonBlockingCall(cb);
}

/**
 * @param ipv6 { bool } sets the type of the udp socket
 * @param recvCallback { (data: Buffer, addr: string, port: number)=>void } called when receiving data
 */
CONSTRUCTOR_IMPL(Socket)
{
    NB_ARGS(2)
    ARG_BOOLEAN(0, ipv6);
    ARG_FUNC(1, recvCallback)

    pcb = udp_new_ip_type(ipv6 ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4);

    recv_cb = Napi::ThreadSafeFunction::New(env, recvCallback, "recvCallback", 0, 1);

    udp_recv(pcb, lwip_recv_cb, &recv_cb);
}

CLASS_METHOD_IMPL(Socket, send_to)
{
    NB_ARGS(3)
    ARG_UINT8ARRAY(0, data)
    ARG_STRING(1, addr)
    ARG_NUMBER(2, port)

    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, data.ByteLength(), PBUF_RAM);
    p->payload = data.Data();
    p->len = data.ByteLength();

    ip_addr_t ip_addr;
    ipaddr_aton(addr.c_str(), &ip_addr);

    int err = udp_sendto(pcb, p, &ip_addr, port.Int32Value());

    if (err < 0) {
        auto e = Napi::Error::New(env, "Error while sending udp");
        e.Set(STRING("code"), NUMBER(err));
        throw e;
    }

    return VOID;
}

CLASS_METHOD_IMPL(Socket, bind)
{
    NB_ARGS(2)
    ARG_STRING(0, addr)
    ARG_NUMBER(1, port)

    ip_addr_t ip_addr;
    ipaddr_aton(addr.c_str(), &ip_addr);

    int err = udp_bind(pcb, IP4_ADDR_ANY, port.Int32Value());

    if (err < 0) {
        auto e = Napi::Error::New(env, "Error while binding udp");
        e.Set(STRING("code"), NUMBER(err));
        throw e;
    }

    return VOID;
}

}   // namespace UDP