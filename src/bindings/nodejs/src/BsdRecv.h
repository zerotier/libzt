#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;

class BsdRecvWorker : public AsyncWorker {
  public:
    BsdRecvWorker(Function& callback, int fd, size_t n, int flags) : AsyncWorker(callback), fd(fd), n(n), flags(flags)
    {
    }

    ~BsdRecvWorker()
    {
    }
    // This code will be executed on the worker thread
    void Execute() override
    {
        data.reserve(n);

        int bytes_received;
        if ((bytes_received = zts_recv(fd, data.data(), n, flags)) < 0) {
            SetError("Error when receiving.");
            return;
        }
        n = bytes_received;
    }

    void OnOK() override
    {
        HandleScope scope(Env());

        Callback().Call({ Buffer<char>::Copy(Env(), data.data(), n) });
    }

  private:
    int fd;
    size_t n;
    std::vector<char> data;
    int flags;
};