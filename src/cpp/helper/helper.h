//
// Created by ubit-gng on 13.07.2020.
//

#ifndef NATIVE_ADVAPI_WIN_JS_HELPER_H
#define NATIVE_ADVAPI_WIN_JS_HELPER_H

#include <napi.h>
#include <Windows.h>

std::wstring to_wstring(Napi::Value value);
Napi::Error createWindowsError(napi_env env, HRESULT hr, const char* syscall);

#endif //NATIVE_ADVAPI_WIN_JS_HELPER_H
