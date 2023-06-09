#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;

/**
 * Runs lambda asynchronously, calls callback with (null, lambda return value) or (error)
 */
class AsyncLambda : public AsyncWorker {
  public:
    AsyncLambda(Function& callback, std::function<int()> lambda, std::function<void()> on_destroy)
        : AsyncWorker(callback)
        , ret(0)
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
            SetError("Error during operation.");
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
    std::function<int()> lambda;
    std::function<void()> on_destroy;
};