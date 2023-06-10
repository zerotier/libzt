#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;

class BsdRecvWorker : public AsyncWorker {
  public:
    BsdRecvWorker(Function& callback, int fd, size_t n, int flags)
        : AsyncWorker(callback)
        , ret(0)
        , err_no(0)
        , fd(fd)
        , n(n)
        , flags(flags)
    {
    }

    ~BsdRecvWorker()
    {
    }
    // This code will be executed on the worker thread
    void Execute() override
    {
        data.reserve(n);

        ret = zts_bsd_recv(fd, data.data(), n, flags);
        if (ret < 0) {
            err_no = zts_errno;
            SetError("Error when receiving.");
        }
    }

    void OnOK() override
    {
        HandleScope scope(Env());

        Callback().Call({ Env().Null(), Buffer<char>::Copy(Env(), data.data(), ret) });
    }

    void OnError(const Napi::Error& e) override
    {
        HandleScope scope(Env());
        e.Set(String::New(Env(), "code"), Number::New(Env(), ret));
        e.Set(String::New(Env(), "errno"), Number::New(Env(), err_no));

        Callback().Call({ e.Value() });
    }

  private:
    int ret;
    int err_no;

    int fd;
    size_t n;
    std::vector<char> data;
    int flags;
};