#include "lwip/udp.h"

#include "ZeroTierSockets.h"
#include "lwip/tcpip.h"
#include "macros.h"

#include <iostream>
#include <napi.h>

namespace UDP {

FUNC_REF(constructor);

struct recv_data {
    pbuf* p;
    char addr[ZTS_IP_MAX_STR_LEN];
    u16_t port;
};
void tsfnOnRecv(TSFN_ARGS, nullptr_t* ctx, recv_data* rd);

using OnRecvTSFN = Napi::TypedThreadSafeFunction<nullptr_t, recv_data, tsfnOnRecv>;

CLASS(Socket)
{
  public:
    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

    ~Socket()
    {
        // if(pcb) tcpip_callback((tcpip_callback_fn)udp_remove, pcb);
        // recv_cb->Release();
    }

    OnRecvTSFN* onRecv = new OnRecvTSFN();

  private:
    bool ipv6;
    udp_pcb* pcb;

    METHOD(send_to);
    METHOD(bind);
    METHOD(address);
    METHOD(close);
};

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(
        Socket,
        { CLASS_INSTANCE_METHOD(Socket, send_to),
          CLASS_INSTANCE_METHOD(Socket, bind),
          CLASS_INSTANCE_METHOD(Socket, address),
          CLASS_INSTANCE_METHOD(Socket, close) });

    *constructor = Napi::Persistent(func);

    exports["UDP"] = func;
    return exports;
}

void lwip_recv_cb(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    auto thiz = (Socket*)arg;

    recv_data* rd = new recv_data { p : p, port : port };
    ipaddr_ntoa_r(addr, rd->addr, ZTS_IP_MAX_STR_LEN);

    thiz->onRecv->BlockingCall(rd);
}

#define FREE_PBUF(PTR) tcpip_callback([](void* p) { pbuf_free((pbuf*)p); }, PTR);

void tsfnOnRecv(TSFN_ARGS, nullptr_t* ctx, recv_data* rd)
{
    if (env == NULL) {
        FREE_PBUF(rd->p);
        delete rd;

        return;
    }
    pbuf* p = rd->p;

    auto data =
        Napi::Buffer<char>::NewOrCopy(env, (char*)p->payload, p->len, [p](Napi::Env env, char* data) { FREE_PBUF(p) });

    auto addr = STRING(rd->addr);
    auto port = NUMBER(rd->port);

    delete rd;

    jsCallback.Call({ data, addr, port });
}

/**
 * @param ipv6 { bool } sets the type of the udp socket
 * @param recvCallback { (data: Buffer, addr: string, port: number)=>void } called when receiving data
 */
CONSTRUCTOR_IMPL(Socket)
{
    NB_ARGS(2)
    auto ipv6 = ARG_BOOLEAN(0);
    auto recvCallback = ARG_FUNC(1);

    this->ipv6 = ipv6;

    *onRecv = OnRecvTSFN::New(env, recvCallback, "recvCallback", 0, 1, nullptr);

    tcpip_callback(
        [](void* ctx) {
            auto thiz = (Socket*)ctx;

            thiz->pcb = udp_new_ip_type(thiz->ipv6 ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4);

            udp_recv(thiz->pcb, lwip_recv_cb, thiz);
        },
        this);
}

CLASS_METHOD_IMPL(Socket, send_to)
{
    NB_ARGS(4)
    auto data = ARG_UINT8ARRAY(0);
    std::string addr = ARG_STRING(1);
    auto port = ARG_NUMBER(2);
    auto callback = ARG_FUNC(3);

    struct send_data {
        Napi::ThreadSafeFunction* onSent;

        u16_t len;
        uint8_t* data;
        Napi::Reference<Napi::Uint8Array>* dataRef;

        udp_pcb* pcb;
        ip_addr_t ip_addr;
        u16_t port;
    };

    auto sd = new send_data {
        onSent : TSFN_ONCE(callback, "udpOnSent"),

        len : data.ByteLength(),
        data : data.Data(),
        dataRef : new Napi::Reference<Napi::Uint8Array>(),

        pcb : pcb,
        port : port.Int32Value()
    };

    *sd->dataRef = Napi::Persistent(data);

    ipaddr_aton(addr.c_str(), &sd->ip_addr);

    tcpip_callback(
        [](void* ctx) {
            auto sd = (send_data*)ctx;

            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, sd->len, PBUF_RAM);
            p->payload = sd->data;

            auto err = udp_sendto(sd->pcb, p, &sd->ip_addr, sd->port);

            auto dataRef = sd->dataRef;
            sd->onSent->BlockingCall([err, dataRef](TSFN_ARGS) {
                delete dataRef;   // allow data to be garbage collected

                if (err != ERR_OK) {
                    auto error = Napi::Error::New(env, "send error");
                    error.Set("code", NUMBER(err));
                    jsCallback.Call({ error.Value() });
                }
                else
                    jsCallback.Call({});
            });
            sd->onSent->Release();

            pbuf_free(p);
            delete sd;
        },
        sd);

    return VOID;
}

CLASS_METHOD_IMPL(Socket, bind)
{
    NB_ARGS(2)
    std::string addr = ARG_STRING(0);
    auto port = ARG_NUMBER(1);

    struct bind_data {
        udp_pcb* pcb;
        ip_addr_t addr;
        uint16_t port;
    };

    auto bd = new bind_data { pcb : pcb, port : port.Int32Value() };

    if (addr.size() == 0)
        bd->addr = ip6_addr_any;
    else
        ipaddr_aton(addr.c_str(), &bd->addr);

    tcpip_callback(
        [](void* ctx) {
            auto bd = (bind_data*)ctx;

            int err = udp_bind(bd->pcb, &bd->addr, bd->port);

            delete bd;
        },
        bd);

    return VOID;
}

CLASS_METHOD_IMPL(Socket, address)
{
    NO_ARGS();

    char addr[ZTS_IP_MAX_STR_LEN];
    ipaddr_ntoa_r(&pcb->local_ip, addr, ZTS_IP_MAX_STR_LEN);

    return OBJECT(ADD_FIELD(address, STRING(addr)); ADD_FIELD(port, NUMBER(pcb->local_port));
                  ADD_FIELD(family, IP_IS_V6(&pcb->local_ip) ? STRING("udp6") : STRING("udp4")));
}

CLASS_METHOD_IMPL(Socket, close)
{
    NB_ARGS(1)
    auto callback = ARG_FUNC(0);

    if (pcb) {
        // struct cd {

        // }

        tcpip_callback([](void* ctx) { udp_remove((udp_pcb*)ctx); }, pcb);
        pcb = nullptr;

        onRecv->Release();
    }

    return VOID;
}

}   // namespace UDP