#include "lwip/tcp.h"

#include <napi.h>
#include "macros.h"

namespace TCP {

// TODO destructor frees pcb block?
Napi::FunctionReference* constructor = new Napi::FunctionReference();

CLASS(Socket)
{
  public:
    CLASS_INIT_DECL();
    CONSTRUCTOR_DECL(Socket);

  private:
    struct tcp_pcb* pcb;
    
};

CONSTRUCTOR_IMPL(Socket) { NO_ARGS() }

CLASS_INIT_IMPL(Socket)
{
    auto func = CLASS_DEFINE(Socket, {
         
         });

    *constructor = Napi::Persistent(func);
    exports["TCP"] = func;
    return exports;
}



}   // namespace TCP