//
// Created by ubit-gng on 13.07.2020.
//

#ifndef NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H
#define NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H

#include <napi.h>
#include <Windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>

#include <codecvt>
#include <vector>
#include <sstream>
#include <map>

#include "../helper/helper.h"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "userenv.lib")

class FolderPermissionsManager : public Napi::ObjectWrap<FolderPermissionsManager> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        static Napi::Object NewInstance(Napi::Env env, Napi::Value arg0);
        FolderPermissionsManager(const Napi::CallbackInfo& info);
        void AddRight(const Napi::CallbackInfo& info);
        void ApplyRights(const Napi::CallbackInfo& info);
        void ClearExplicitAccessList(const Napi::CallbackInfo& info);
    private:
        DWORD ConvertStringToAccessMask(std::wstring accessString);
        HRESULT GetSid(std::wstring domain, std::wstring name, PSID* ppSid);

        std::vector<EXPLICIT_ACCESS_W> explicitAccessList_;
        std::wstring folderPath_;

        typedef std::map<std::wstring, uint32_t> AccessMask;
        static AccessMask accessMask_;
};


#endif //NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H
