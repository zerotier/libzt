#include "lwip/tcp.h"

#include "ZeroTierSockets.h"
#include "lwip-macros.h"
#include "lwip/tcpip.h"
#include "macros.h"

#include <napi.h>

namespace TCP {

/* #########################################
 * ###############  SOCKET  ################
 * ######################################### */

CLASS(Socket)
{
  public:
    static Napi::FunctionReference* constructor;

    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

    void set_pcb(tcp_pcb * pcb)
    {
        this->pcb = pcb;
    }

  private:
    tcp_pcb* pcb = nullptr;

    Napi::ThreadSafeFunction emit;

    METHOD(connect);
    METHOD(init);
    METHOD(send);
    METHOD(ack);
    METHOD(shutdown_wr);
};

Napi::FunctionReference* Socket::constructor = new Napi::FunctionReference;

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(
        Socket,
        { CLASS_INSTANCE_METHOD(Socket, init),
          CLASS_INSTANCE_METHOD(Socket, connect),
          CLASS_INSTANCE_METHOD(Socket, send),
          CLASS_INSTANCE_METHOD(Socket, ack),
          CLASS_INSTANCE_METHOD(Socket, shutdown_wr) });

    *constructor = Napi::Persistent(func);

    exports["Socket"] = func;
    return exports;
}

CONSTRUCTOR_IMPL(Socket)
{
    NO_ARGS();
}

CLASS_METHOD_IMPL(Socket, init)
{
    NB_ARGS(1);
    auto emit = ARG_FUNC(0);
    this->emit = Napi::ThreadSafeFunction::New(env, emit, "tcpEventEmitter", 0, 1);

    typed_tcpip_callback([=]() {
        if (! this->pcb)
            this->pcb = tcp_new();
        tcp_arg(this->pcb, this);

        tcp_recv(this->pcb, [](void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) -> err_t {
            auto thiz = reinterpret_cast<Socket*>(arg);
            if (thiz->emit.Acquire() != napi_ok) {
                if (p)
                    ts_pbuf_free(p);
                return ERR_OK;   // TODO: other return code?
            }
            thiz->emit.BlockingCall([=](TSFN_ARGS) {
                if (! p) {
                    jsCallback.Call({ STRING("data"), VOID });
                }
                else {
                    auto data = Napi::Buffer<char>::NewOrCopy(
                        env,
                        reinterpret_cast<char*>(p->payload),
                        p->len,
                        [p](Napi::Env env, char* data) { ts_pbuf_free(p); });
                    jsCallback.Call({ STRING("data"), data });
                }
            });
            thiz->emit.Release();
            return ERR_OK;
        });

        tcp_sent(this->pcb, [](void* arg, struct tcp_pcb* tpcb, u16_t len) -> err_t {
            auto thiz = reinterpret_cast<Socket*>(arg);
            thiz->emit.BlockingCall([=](TSFN_ARGS) { jsCallback.Call({ STRING("sent"), NUMBER(len) }); });
            return ERR_OK;
        });

        tcp_err(this->pcb, [](void* arg, err_t err) {
            auto thiz = reinterpret_cast<Socket*>(arg);
            thiz->emit.BlockingCall([=](TSFN_ARGS) {
                jsCallback.Call({ STRING("error"), MAKE_ERROR("TCP error", ERR_FIELD("code", NUMBER(err))).Value() });
            });
        });
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, connect)
{
    NB_ARGS(2);
    int port = ARG_NUMBER(0);
    std::string address = ARG_STRING(1);

    ip_addr_t ip_addr;
    ipaddr_aton(address.c_str(), &ip_addr);

    typed_tcpip_callback([=]() {
        tcp_connect(this->pcb, &ip_addr, port, [](void* arg, struct tcp_pcb* tpcb, err_t err) -> err_t {
            auto thiz = reinterpret_cast<Socket*>(arg);
            thiz->emit.BlockingCall([](TSFN_ARGS) { jsCallback.Call({ STRING("connect") }); });
            return ERR_OK;
        });
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, send)
{
    NB_ARGS(2);
    auto data = ARG_UINT8ARRAY(0);
    auto callback = ARG_FUNC(1);

    auto dataRef = NEW_REF_UINT8ARRAY(data);
    auto sendCallback = TSFN_ONCE(callback, "sendCallback", {
        // make sure that data is present in the callback
        delete dataRef;
    });

    auto bufLength = data.ByteLength();
    auto buffer = data.Data();

    typed_tcpip_callback([=]() {
        auto sndbuf = tcp_sndbuf(this->pcb);

        u16_t len = (sndbuf < bufLength) ? sndbuf : bufLength;
        tcp_write(this->pcb, buffer, len, TCP_WRITE_FLAG_COPY);

        sendCallback->BlockingCall([=](TSFN_ARGS) { jsCallback.Call({ NUMBER(len) }); });
        sendCallback->Release();
    });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, ack)
{
    NB_ARGS(1);
    int length = ARG_NUMBER(0);

    typed_tcpip_callback([=]() { tcp_recved(this->pcb, length); });

    return VOID;
}

CLASS_METHOD_IMPL(Socket, shutdown_wr)
{
    NO_ARGS();

    typed_tcpip_callback([=]() { tcp_shutdown(this->pcb, 0, 1); });

    return VOID;
}

/* #########################################
 * ###############  SERVER  ################
 * ######################################### */

CLASS(Server)
{
  public:
    static Napi::FunctionReference* constructor;

    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Server);

  private:
    tcp_pcb* pcb;

    Napi::ThreadSafeFunction onConnection;

    METHOD(listen);
    METHOD(address);

    tcp_accept_fn acceptCb;
};

Napi::FunctionReference* Server::constructor = new Napi::FunctionReference;

CLASS_INIT_IMPL(Server)
{
    auto func = CLASS_DEFINE(Server, { CLASS_INSTANCE_METHOD(Server, listen), CLASS_INSTANCE_METHOD(Server, address) });

    *Server::constructor = Napi::Persistent(func);

    exports["Server"] = func;
    return exports;
}

CONSTRUCTOR_IMPL(Server)
{
    NB_ARGS(1);
    auto onConnection = ARG_FUNC(0);
    this->onConnection = Napi::ThreadSafeFunction::New(env, onConnection, "TCP::onConnection", 0, 1);

    acceptCb = [](void* arg, tcp_pcb* new_pcb, err_t err) -> err_t {
        auto thiz = reinterpret_cast<Server*>(arg);

        // delay accepting connection until callback has been set up.
        tcp_backlog_delayed(new_pcb);

        thiz->onConnection.BlockingCall([=](TSFN_ARGS) {
            if (err < 0) {
                jsCallback.Call({ MAKE_ERROR("accept error", { ERR_FIELD("code", NUMBER(err)); }).Value() });
                return;
            }

            auto socket = Socket::constructor->New({});
            Socket::Unwrap(socket)->set_pcb(new_pcb);

            jsCallback.Call({ VOID, socket });

            // event handlers set in callback so now accept connection
            typed_tcpip_callback([=]() { tcp_backlog_accepted(new_pcb); });
        });

        return ERR_OK;
    };

    typed_tcpip_callback([=]() {
        this->pcb = tcp_new();
        tcp_arg(this->pcb, this);
    });
}

CLASS_METHOD_IMPL(Server, listen)
{
    NB_ARGS(3);
    int port = ARG_NUMBER(0);
    std::string address = ARG_STRING(1);
    auto callback = ARG_FUNC(2);

    auto onListening = TSFN_ONCE(callback, "onListening", );

    ip_addr_t ip_addr;
    if (address.size() == 0)
        ip_addr = ip_addr_any_type;
    else
        ipaddr_aton(address.c_str(), &ip_addr);

    typed_tcpip_callback([=]() {
        int err = tcp_bind(this->pcb, &ip_addr, port);
        if (err < 0) {
            onListening->BlockingCall([=](TSFN_ARGS) {
                jsCallback.Call({ MAKE_ERROR("failed to bind", { ERR_FIELD("code", NUMBER(err)); }).Value() });
            });
            onListening->Release();
            return;
        }
        this->pcb = tcp_listen(this->pcb);
        tcp_accept(this->pcb, this->acceptCb);
        onListening->BlockingCall([](TSFN_ARGS) { jsCallback.Call({}); });
        onListening->Release();
    });

    return VOID;
}

CLASS_METHOD_IMPL(Server, address)
{
    NO_ARGS();

    char addr[ZTS_IP_MAX_STR_LEN];
    ipaddr_ntoa_r(&pcb->local_ip, addr, ZTS_IP_MAX_STR_LEN);

    return OBJECT({
        ADD_FIELD("address", STRING(addr));
        ADD_FIELD("port", NUMBER(pcb->local_port));
        ADD_FIELD("family", IP_IS_V6(&pcb->local_ip) ? STRING("IPv6") : STRING("IPv4"))
    });
}

}   // namespace TCP