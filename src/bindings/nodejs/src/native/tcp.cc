#include "lwip/tcp.h"

#include "lwip/tcpip.h"
#include "macros.h"

#include <napi.h>

namespace TCP {

CLASS(Server)
{
  public:
    static Napi::FunctionReference* constructor;

    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Server);

  private:
    tcp_pcb* pcb;
};

Napi::FunctionReference* Server::constructor = new Napi::FunctionReference;

CONSTRUCTOR_IMPL(Server)
{
    NO_ARGS();

    tcpip_callback(
        [](void* ctx) {
            auto thiz = (Server*)ctx;

            thiz->pcb = tcp_new();
            tcp_arg(thiz->pcb, thiz);
        },
        this);
}

CLASS_INIT_IMPL(Server)
{
    auto func = CLASS_DEFINE(
        Server,
        {

        });

    *Server::constructor = Napi::Persistent(func);

    exports["Server"] = func;
    return exports;
}

// ############################

CLASS(Socket)
{
  public:
    static Napi::FunctionReference* constructor;

    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

  private:
    struct tcp_pcb* pcb;
};

Napi::FunctionReference* Socket::constructor = new Napi::FunctionReference;

CONSTRUCTOR_IMPL(Socket)
{
    NO_ARGS();
}

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(
        Socket,
        {

        });

    *constructor = Napi::Persistent(func);

    exports["Socket"] = func;
    return exports;
}

}   // namespace TCP