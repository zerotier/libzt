#ifndef NAPI_MACROS
#define NAPI_MACROS

// INITIALISATION

#define INIT_ADDON(NAME) \
    void Init(Napi::Env env, Napi::Object exports); \
    Napi::Object Init_Wrap(Napi::Env env, Napi::Object exports) { \
        Init(env, exports); \
        return exports; \
    } \
    NODE_API_MODULE(NAME, Init_Wrap); \
    void Init(Napi::Env env, Napi::Object exports)

#define EXPORT(F) exports[#F] = Napi::Function::New(env, F);

// METHOD 
 
#define CALLBACKINFO const Napi::CallbackInfo& info

#define METHOD(NAME) Napi::Value NAME(CALLBACKINFO)

#define HANDLE_SCOPE() \
    Napi::HandleScope scope(Env()); \
    Napi::Env env = Env();


// ENVIRONMENT AND ARGUMENTS

#define NO_ARGS() Napi::Env env = info.Env();

#define NB_ARGS(N)                                                                                                     \
    Napi::Env env = info.Env();                                                                                              \
    if (info.Length() < N) {                                                                                           \
        throw Napi::TypeError::New(env, "Wrong number of arguments. Expected: " #N);                                         \
    }

#define ARG_FUNC(POS, NAME)                                                                                            \
    if (! info[POS].IsFunction()) {                                                                                    \
        throw Napi::TypeError::New(env, "Argument at position " #POS "should be a function.");                               \
    }                                                                                                                  \
    auto NAME = info[POS].As<Napi::Function>();

#define ARG_INT32(POS, NAME)                                                                                           \
    if (! info[POS].IsNumber()) {                                                                                      \
        throw Napi::TypeError::New(env, "Argument at position " #POS "should be a number.");                                 \
    }                                                                                                                  \
    auto NAME = info[POS].As<Napi::Number>().Int32Value();

#define ARG_STRING(POS, NAME)  auto NAME = std::string(info[POS].ToString());

#define ARG_BOOLEAN(POS, NAME) auto NAME = info[POS].ToBoolean();

#define ARG_UINT8ARRAY(POS, NAME)                                                                                      \
    if (! info[POS].IsTypedArray()) {                                                                                  \
        throw Napi::TypeError::New(env, "Argument at position " #POS "should be a Uint8Array.");                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Napi::Uint8Array>();


// WRAP DATA

#define VOID env.Undefined();

#define BOOL(VALUE) Napi::Boolean::New(info.Env(), VALUE)

#define STRING(VALUE) Napi::String::New(env, VALUE)

#define NUMBER(VALUE) Napi::Number::New(env, VALUE)

// use inside of an OBJECT def macro
#define ADD_FIELD(NAME, VALUE) ret_obj[#NAME] = VALUE;

#define OBJECT(...)                                                                                             \
    [&] { \
        auto ret_obj = Napi::Object::New(env);                                                                                   \
        __VA_ARGS__ \
        return ret_obj; \
    }()


#endif // NAPI_MACROS