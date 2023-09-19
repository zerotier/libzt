#include "lwip/tcp.h"

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

    tcp_pcb* pcb;

  private:
    METHOD(connect);
};

Napi::FunctionReference* Socket::constructor = new Napi::FunctionReference;

CONSTRUCTOR_IMPL(Socket)
{
    NO_ARGS();
}

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(Socket, { CLASS_INSTANCE_METHOD(Socket, connect) });

    *constructor = Napi::Persistent(func);

    exports["Socket"] = func;
    return exports;
}

CLASS_METHOD_IMPL(Socket, connect)
{
    NB_ARGS(2);
    int port = ARG_NUMBER(0);
    std::string address = ARG_STRING(1);

    ip_addr_t ip_addr;
    ipaddr_aton(address.c_str(), &ip_addr);
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
        auto thiz = (Server*)arg;

        // delay accepting connection until callback has been set up.
        tcp_backlog_delayed(new_pcb);

        thiz->onConnection.BlockingCall([=](TSFN_ARGS) {
            if (err < 0) {
                jsCallback.Call({ MAKE_ERROR("accept error", { ERR_FIELD("code", NUMBER(err)); }).Value() });
                return;
            }

            auto socket = Socket::constructor->New({});
            Socket::Unwrap(socket)->pcb = new_pcb;

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