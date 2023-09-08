#include "ZeroTierSockets.h"
#include "macros.h"

#include <napi.h>

class BsdRecvWorker : public Napi::AsyncWorker {
  public:
    BsdRecvWorker(Napi::Function& callback, int fd, size_t n, int flags)
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
        HANDLE_SCOPE();

        Callback().Call({ Env().Null(), Napi::Buffer<char>::Copy(Env(), data.data(), ret) });
    }

    void OnError(const Napi::Error& e) override
    {
        HANDLE_SCOPE();

        e.Set(STRING("code"), NUMBER(ret));
        e.Set(STRING("errno"), NUMBER(err_no));

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

class BsdRecvFromWorker : public Napi::AsyncWorker {
  public:
    BsdRecvFromWorker(Napi::Function& callback, int fd, size_t n, int flags)
        : AsyncWorker(callback)
        , ret(0)
        , err_no(0)
        , addrlen(sizeof(zts_sockaddr_storage))
        , fd(fd)
        , n(n)
        , flags(flags)
    {
    }

    ~BsdRecvFromWorker()
    {
    }
    // This code will be executed on the worker thread
    void Execute() override
    {
        data.reserve(n);

        ret = zts_bsd_recvfrom(fd, data.data(), n, flags, (struct zts_sockaddr*)&sockaddr, &addrlen);
        if (ret < 0) {
            err_no = zts_errno;
            SetError("Error when receiving.");
        }
    }

    void OnOK() override
    {
        HANDLE_SCOPE();

        char address[ZTS_IP_MAX_STR_LEN];
        unsigned short port;
        zts_util_ntop((struct zts_sockaddr*)&sockaddr, addrlen, address, ZTS_IP_MAX_STR_LEN, &port);

        Callback().Call({ Env().Null(),
                          Napi::Buffer<char>::Copy(Env(), data.data(), ret),
                          STRING(address),
                          NUMBER(port) });
    }

    void OnError(const Napi::Error& e) override
    {
        HANDLE_SCOPE();

        e.Set(STRING("code"), NUMBER(ret));
        e.Set(STRING("errno"), NUMBER(err_no));

        Callback().Call({ e.Value() });
    }

  private:
    int ret;
    int err_no;

    zts_sockaddr_storage sockaddr;
    zts_socklen_t addrlen;

    int fd;
    size_t n;
    std::vector<char> data;
    int flags;
};