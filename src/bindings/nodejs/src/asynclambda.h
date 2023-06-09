#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;

/**
 * Runs lambda asynchronously, calls callback with (null, lambda return value) or (error)
 */
class AsyncLambda : public AsyncWorker {
  public:
    AsyncLambda(Function& callback, std::string name, std::function<int()> lambda, std::function<void()> on_destroy)
        : AsyncWorker(callback)
        , ret(0)
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
            char err[200];
            snprintf(err, 200, "Error during operation %s, ret: %d, errno: %d", name.c_str(), ret, errno);
            SetError(std::string(err));
        }
    }

    void OnOK() override
    {
        HandleScope scope(Env());

        Callback().Call({ Env().Null(), Number::New(Env(), ret) });
    }

    void Destroy() override
    {
        on_destroy();
    }

  private:
    int ret;
    std::string name;
    std::function<int()> lambda;
    std::function<void()> on_destroy;
};