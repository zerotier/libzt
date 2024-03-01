#include "lwip/udp.h"

#include "ZeroTierSockets.h"
#include "lwip-macros.h"
#include "lwip/tcpip.h"
#include "macros.h"

#include <iostream>
#include <napi.h>

namespace UDP {

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
    static Napi::FunctionReference* constructor;

    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

    OnRecvTSFN onRecv;

  private:
    udp_pcb* pcb;

    METHOD(send);
    METHOD(bind);
    METHOD(close);

    METHOD(address);
    METHOD(remoteAddress);

    METHOD(connect);
    METHOD(disconnect);
    METHOD(ref)
    {
        NO_ARGS();
        onRecv.Ref(env);
        return VOID;
    }
    METHOD(unref)
    {
        NO_ARGS();
        onRecv.Unref(env);
        return VOID;
    }
};

Napi::FunctionReference* Socket::constructor = new Napi::FunctionReference;

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(
        Socket,
        { CLASS_INSTANCE_METHOD(Socket, send),
          CLASS_INSTANCE_METHOD(Socket, bind),
          CLASS_INSTANCE_METHOD(Socket, close),
          CLASS_INSTANCE_METHOD(Socket, address),
          CLASS_INSTANCE_METHOD(Socket, remoteAddress),
          CLASS_INSTANCE_METHOD(Socket, connect),
          CLASS_INSTANCE_METHOD(Socket, disconnect),
          CLASS_INSTANCE_METHOD(Socket, ref),
          CLASS_INSTANCE_METHOD(Socket, unref) });

    *constructor = Napi::Persistent(func);

    exports["UDP"] = func;
    return exports;
}

void lwip_recv_cb(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    auto thiz = reinterpret_cast<Socket*>(arg);

    recv_data* rd = new recv_data { p : p, addr : 0, port : port };
    ipaddr_ntoa_r(addr, rd->addr, ZTS_IP_MAX_STR_LEN);

    thiz->onRecv.BlockingCall(rd);
}

void tsfnOnRecv(TSFN_ARGS, nullptr_t* ctx, recv_data* rd)
{
    if (env == NULL) {
        ts_pbuf_free(rd->p);
        delete rd;

        return;
    }
    pbuf* p = rd->p;

    auto data =
        Napi::Buffer<char>::NewOrCopy(env, reinterpret_cast<char*>(p->payload), p->len, [p](Napi::Env env, char* data) {
            ts_pbuf_free(p);
        });

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
    NB_ARGS(2);
    bool ipv6 = ARG_BOOLEAN(0);
    auto recvCallback = ARG_FUNC(1);

    onRecv = OnRecvTSFN::New(env, recvCallback, "recvCallback", 0, 1, nullptr);

    typed_tcpip_callback([this, ipv6]() {
        this->pcb = udp_new_ip_type(ipv6 ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4);

        udp_recv(this->pcb, lwip_recv_cb, this);
    });
}

CLASS_METHOD_IMPL(Socket, send)
{
    NB_ARGS(4);
    auto data = ARG_UINT8ARRAY(0);
    std::string addr = ARG_STRING(1);
    int port = ARG_NUMBER(2);
    auto callback = ARG_FUNC(3);

    auto dataRef = NEW_REF_UINT8ARRAY(data);
    auto onSent = TSFN_ONCE(
        callback,
        "udpOnSent",
        /* unref data when sending complete */ { delete dataRef; });
    auto len = data.ByteLength();
    auto buffer = data.Data();

    ip_addr_t ip_addr;
    if (port)
        ipaddr_aton(addr.c_str(), &ip_addr);

    typed_tcpip_callback([this, onSent, port, len, buffer, ip_addr]() {
        struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_REF);
        p->payload = buffer;

        auto err = port ? udp_sendto(this->pcb, p, &ip_addr, port) : udp_send(this->pcb, p);

        onSent->BlockingCall([err](TSFN_ARGS) {
            if (err != ERR_OK)
                jsCallback.Call({ MAKE_ERROR("send error", { ERR_FIELD("code", NUMBER(err)); }).Value() });
            else
                jsCallback.Call({});
        });
        onSent->Release();

        pbuf_free(p);
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, bind)
{
    NB_ARGS(3);
    std::string addr = ARG_STRING(0);
    int port = ARG_NUMBER(1);
    auto callback = ARG_FUNC(2);

    auto bindCb = TSFN_ONCE(callback, "udpBindCb", );
    ip_addr_t ip_addr;

    if (addr.size() == 0)
        ip_addr = ip6_addr_any;
    else
        ipaddr_aton(addr.c_str(), &ip_addr);

    typed_tcpip_callback([=]() {
        auto err = udp_bind(pcb, &ip_addr, port);

        bindCb->BlockingCall([err](TSFN_ARGS) {
            if (err != ERR_OK)
                jsCallback.Call({ MAKE_ERROR("Bind error", { ERR_FIELD("code", NUMBER(err)); }).Value() });
            else
                jsCallback.Call({});
        });
        bindCb->Release();
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, close)
{
    NB_ARGS(1);
    auto callback = ARG_FUNC(0);

    if (pcb) {
        auto onClose = TSFN_ONCE(callback, "udpOnClose", { this->onRecv.Abort(); });

        auto old_pcb = pcb;
        typed_tcpip_callback([old_pcb, onClose]() {
            udp_remove(old_pcb);

            onClose->BlockingCall();
            onClose->Release();
        });

        pcb = nullptr;
    }

    return VOID;
}

CLASS_METHOD_IMPL(Socket, address)
{
    NO_ARGS();

    char addr[ZTS_IP_MAX_STR_LEN];
    ipaddr_ntoa_r(&pcb->local_ip, addr, ZTS_IP_MAX_STR_LEN);

    return OBJECT({
        ADD_FIELD("address", STRING(addr));
        ADD_FIELD("port", NUMBER(pcb->local_port));
        ADD_FIELD("family", IP_IS_V6(&pcb->local_ip) ? STRING("udp6") : STRING("udp4"))
    });
}

CLASS_METHOD_IMPL(Socket, remoteAddress)
{
    NO_ARGS();

    char addr[ZTS_IP_MAX_STR_LEN];
    ipaddr_ntoa_r(&pcb->remote_ip, addr, ZTS_IP_MAX_STR_LEN);

    return OBJECT({
        ADD_FIELD("address", STRING(addr));
        ADD_FIELD("port", NUMBER(pcb->remote_port));
        ADD_FIELD("family", IP_IS_V6(&pcb->remote_ip) ? STRING("udp6") : STRING("udp4"))
    });
}

CLASS_METHOD_IMPL(Socket, connect)
{
    NB_ARGS(3);

    std::string address = ARG_STRING(0);
    int port = ARG_NUMBER(1);
    auto callback = ARG_FUNC(2);

    auto onConnect = TSFN_ONCE(callback, "udpConnectCb", );
    ip_addr_t addr;
    ipaddr_aton(address.c_str(), &addr);

    typed_tcpip_callback([=]() {
        auto err = udp_connect(pcb, &addr, port);

        onConnect->BlockingCall([err](TSFN_ARGS) {
            if (err != ERR_OK)
                jsCallback.Call({ MAKE_ERROR("connect error", { ERR_FIELD("code", NUMBER(err)); }).Value() });
            else
                jsCallback.Call({});
        });
        onConnect->Release();
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, disconnect)
{
    NO_ARGS();

    typed_tcpip_callback([=]() { udp_disconnect(pcb); });

    return VOID;
}

}   // namespace UDP
