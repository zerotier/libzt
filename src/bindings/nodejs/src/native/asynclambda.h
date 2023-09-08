#include "ZeroTierSockets.h"

#include <napi.h>
#include "macros.h"


/**
 * Runs lambda asynchronously, calls callback with (null, lambda return value) or (error)
 */
class AsyncLambda : public Napi::AsyncWorker {
  public:
    AsyncLambda(Napi::Function& callback, std::string name, std::function<int()> lambda, std::function<void()> on_destroy)
        : AsyncWorker(callback)
        , ret(0)
        , err_no(0)
        , name(name)
        , lambda(lambda)
        , on_destroy(on_destroy)
    {
    }

    ~AsyncLambda()
    {
    }

    void Execute() override
    {
        ret = lambda();
        if (ret < 0) {
            err_no = zts_errno;

            char err[200];
            snprintf(err, 200, "Error during operation %s, ret: %d, errno: %d", name.c_str(), ret, zts_errno);
            SetError(std::string(err));
        }
    }

    void OnOK() override
    {
        HANDLE_SCOPE();

        Callback().Call({ Env().Null(), NUMBER(ret) });
    }

    void OnError(const Napi::Error& e) override
    {
        HANDLE_SCOPE();
        
        e.Set(STRING("code"), NUMBER(ret));
        e.Set(STRING("errno"), NUMBER(err_no));

        Callback().Call({ e.Value() });
    }

    void Destroy() override
    {
        on_destroy();
    }

  private:
    int ret;
    int err_no;
    std::string name;
    std::function<int()> lambda;
    std::function<void()> on_destroy;
};