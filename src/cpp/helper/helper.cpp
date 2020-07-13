//
// Created by ubit-gng on 13.07.2020.
//

#include "helper.h"

using namespace Napi;

// like unique_ptr, but takes a deleter value, not type.
template <typename T, auto Deleter>
struct Ptr {
    T* value = NULL;

    Ptr() = default;
    // No copies
    Ptr(const Ptr&) = delete;
    Ptr& operator=(const Ptr&) = delete;
    // Moves
    Ptr(Ptr&& other) : value{ other.release() } {}
    Ptr& operator=(Ptr&& other) {
        assign(other.release());
    }

    ~Ptr() { clear(); }

    operator T*() const { return value; }
    T* operator ->() const { return value; }

    T* assign(T* newValue) {
        clear();
        return value = newValue;
    }

    T* release() {
        auto result = value;
        value = nullptr;
        return result;
    }

    void clear() {
        if (value) {
            Deleter(value);
            value = nullptr;
        }
    }
};

template <typename T>
using Win32Local = Ptr<T, LocalFree>;

/***
 * Converting Napi Value to w_string
 * @param value
 * @return
 */
std::wstring to_wstring(Value value) {
    size_t length;
    napi_status status = napi_get_value_string_utf16(
            value.Env(),
            value,
            nullptr,
            0,
            &length);
    NAPI_THROW_IF_FAILED_VOID(value.Env(), status);

    std::wstring result;
    result.reserve(length + 1);
    result.resize(length);
    status = napi_get_value_string_utf16(
            value.Env(),
            value,
            reinterpret_cast<char16_t*>(result.data()),
            result.capacity(),
            nullptr);
    NAPI_THROW_IF_FAILED_VOID(value.Env(), status);
    return result;
}

/***
 * Converting w_string to Napi Value
 * @param env
 * @param str
 * @return
 */
Value to_value(Env env, std::wstring_view str) {
    return String::New(
            env,
            reinterpret_cast<const char16_t*>(str.data()),
            str.size());
}

/***
 * Formatting system Error Message
 * @param hr
 * @return
 */
std::wstring formatSystemError(HRESULT hr) {
    Win32Local<WCHAR> message_ptr;
    FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            hr,
            LANG_USER_DEFAULT,
            (LPWSTR)&message_ptr,
            0,
            nullptr);

    if (!message_ptr) {
        return L"Unknown Error";
    }

    std::wstring message(message_ptr);

    // Windows ends it's system messages with "\r\n", which is bad formatting for us.
    if (auto last_not_newline = message.find_last_not_of(L"\r\n");
            last_not_newline != std::wstring::npos) {
        message.erase(last_not_newline + 1);
    }

    return message;
}

/***
 * Creating Windows Error Object
 * @param env
 * @param hr
 * @param syscall
 * @return
 */
Error createWindowsError(napi_env env, HRESULT hr, const char* syscall) {
    napi_value error_value = nullptr;

    napi_status status = napi_create_error(
            env,
            nullptr,
            to_value(env, formatSystemError(hr)),
            &error_value);
    if (status != napi_ok) {
        throw Error::New(env);
    }

    auto error = Error(env, error_value);
    error.Value().DefineProperties({
                                           PropertyDescriptor::Value("errno", Number::New(env, hr)),
                                           PropertyDescriptor::Value("name", String::New(env, "WindowsError")),
                                           PropertyDescriptor::Value("syscall", String::New(env, syscall)),
                                   });
    return error;
}

