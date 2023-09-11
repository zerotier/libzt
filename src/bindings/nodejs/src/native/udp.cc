#include "lwip/udp.h"

#include "ZeroTierSockets.h"
#include "lwip/tcpip.h"
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
        tcpip_callback((tcpip_callback_fn)udp_remove, pcb);
        // udp_remove(pcb);
        recv_cb->Release();
    }

  private:
    udp_pcb* pcb;

    Napi::ThreadSafeFunction* recv_cb = new Napi::ThreadSafeFunction;
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

    struct recv_data {
        pbuf* p;
        const ip_addr_t* addr;
        u16_t port;
    };
    auto rd = new recv_data;
    rd->p = p;
    rd->addr = addr;
    rd->port = port;

    auto cb = [](Napi::Env env, Napi::Function jsCallback, recv_data* rd) {
        char address[ZTS_IP_MAX_STR_LEN];
        
        ipaddr_ntoa_r(rd->addr, address, ZTS_IP_MAX_STR_LEN);


        jsCallback.Call({ Napi::Buffer<char>::Copy(env, (const char*)rd->p->payload, rd->p->len),
                          STRING(address),
                          NUMBER(rd->port) });

        pbuf_free(rd->p);
    };
    threadsafe->NonBlockingCall(rd, cb);
}

/**
 * @param ipv6 { bool } sets the type of the udp socket
 * @param recvCallback { (data: Buffer, addr: string, port: number)=>void } called when receiving data
 */
CONSTRUCTOR_IMPL(Socket)
{
    NB_ARGS(2)
    ARG_BOOLEAN(0, ipv6);
    ARG_FUNC(1, recvCallback);

    struct construct_data {
        udp_pcb** pcb_ptr;
        bool ipv6;
        Napi::ThreadSafeFunction* recv_cb;
    };

    auto cd = new construct_data;
    cd->pcb_ptr = &pcb;
    cd->ipv6 = ipv6;
    cd->recv_cb = recv_cb;

    *recv_cb = Napi::ThreadSafeFunction::New(env, recvCallback, "recvCallback", 0, 1);

    tcpip_callback(
        [](void* ctx) {
            auto cd = (construct_data*)ctx;

            *cd->pcb_ptr = udp_new_ip_type(cd->ipv6 ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4);

            udp_recv(*cd->pcb_ptr, lwip_recv_cb, cd->recv_cb);

            delete cd;
        },
        cd);

    // udp_recv(pcb, lwip_recv_cb, recv_cb);
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

    struct send_data {
        udp_pcb* pcb;
        ip_addr_t ip_addr;
        pbuf* p;
        u16_t port;
    };

    auto sd = new send_data;
    sd->p = p;
    sd->pcb = pcb;
    sd->port = port.Int32Value();

    ipaddr_aton(addr.c_str(), &sd->ip_addr);

    tcpip_callback(
        [](void* ctx) {
            auto sd = (send_data*)ctx;

            int err = udp_sendto(sd->pcb, sd->p, &sd->ip_addr, sd->port);

            pbuf_free(sd->p);
            delete sd;
        },
        sd);

    // if (err < 0) {
    //     auto e = Napi::Error::New(env, "Error while sending udp");
    //     e.Set(STRING("code"), NUMBER(err));
    //     throw e;
    // }

    return VOID;
}

CLASS_METHOD_IMPL(Socket, bind)
{
    NB_ARGS(2)
    ARG_STRING(0, addr)
    ARG_NUMBER(1, port)

    ip_addr_t ip_addr;
    ipaddr_aton(addr.c_str(), &ip_addr);

    struct bind_data {
        udp_pcb* pcb;
        uint16_t port;
    };

    auto bd = new bind_data;
    bd->pcb = pcb;
    bd->port = port.Int32Value();

    tcpip_callback(
        [](void* ctx) {
            auto bd = (bind_data*)ctx;

            int err = udp_bind(bd->pcb, IP6_ADDR_ANY, bd->port);

            delete bd;
        },
        bd);

    // if (err < 0) {
    //     auto e = Napi::Error::New(env, "Error while binding udp");
    //     e.Set(STRING("code"), NUMBER(err));
    //     throw e;
    // }

    return VOID;
}

}   // namespace UDP