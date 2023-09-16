#ifndef LWIP_MACROS
#define LWIP_MACROS

// https://www.scs.stanford.edu/~dm/blog/va-opt.html#the-for_each-macro
#define PARENS ()

#define EXPAND(...)  EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)            __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH_AGAIN()                FOR_EACH_HELPER

#define TYPEOFARG(ARG) decltype(ARG) __##ARG;

#define ARGTWICE(ARG)                                                                                                  \
    __##ARG                                                                                                            \
        : ARG                                                                                                          \
        ,

#define TAKEOUT(ARG) auto ARG = __data->__##ARG;

/**
 * BODY is run in a tcpip_callback, with args coming after it similar to a lambda capture list but all copy
*/
#define TCPIP_CALLBACK(BODY, ...)                                                                                \
    [&]() {                                                                                                            \
        struct __data_t {                                                                                              \
            FOR_EACH(TYPEOFARG, __VA_ARGS__)                                                                           \
        };                                                                                                             \
        auto __data = new __data_t { FOR_EACH(ARGTWICE, __VA_ARGS__) };                                                \
        tcpip_callback(                                                                                                \
            [](void* ctx) {                                                                                            \
                auto __data = (__data_t*)ctx;                                                                          \
                FOR_EACH(TAKEOUT, __VA_ARGS__)                                                                         \
                BODY;                                                                                                  \
                delete __data;                                                                                         \
            },                                                                                                         \
            __data);                                                                                                   \
    }();

#endif