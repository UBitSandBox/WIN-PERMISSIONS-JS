//
// Created by ubit-gng on 08.07.2020.
//
#include <napi.h>
#include "permissions/FolderPermissionsManager.h"

Napi::Object CreateObject(const Napi::CallbackInfo& info) {
    return FolderPermissionsManager::NewInstance(info.Env(), info[0]);
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    Napi::Object new_exports =
            Napi::Function::New(env, CreateObject, "CreateObject");
    return FolderPermissionsManager::Init(env, new_exports);
}

NODE_API_MODULE(addon, InitAll)
