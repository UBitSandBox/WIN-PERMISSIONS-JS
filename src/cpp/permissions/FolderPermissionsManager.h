//
// Created by ubit-gng on 13.07.2020.
//

#ifndef NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H
#define NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H

#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <authz.h>
#include <iads.h>
#include <Adshlp.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlenc.h>


#include <codecvt>
#include <vector>
#include <sstream>
#include <map>

#include "../helper/helper.h"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "authz.lib")
#pragma comment(lib, "Activeds.lib")
#pragma comment(lib, "Adsiid.lib")

class FolderPermissionsManager : public Napi::ObjectWrap<FolderPermissionsManager> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    static Napi::Object NewInstance(Napi::Env env, Napi::Value arg0);

    FolderPermissionsManager(const Napi::CallbackInfo &info);

    void CheckUserPermissions(const Napi::CallbackInfo &info);

    void AddRight(const Napi::CallbackInfo &info);

    void ApplyRights(const Napi::CallbackInfo &info);

    void ClearExplicitAccessList(const Napi::CallbackInfo &info);

private:
    DWORD ConvertStringToAccessMask(Napi::Env env, std::wstring accessString);

    HRESULT GetSid(std::wstring domain, std::wstring name, PSID *ppSid);

    HRESULT GetSidFromString(std::wstring sidString, PSID *ppSid);

    HRESULT GetGuidFromSidString(std::wstring sidString, BSTR *guidValue);

    HRESULT GetObjectGuid(IDirectoryObject *pDO, BSTR *bsGuid);

    BOOL CheckIsMemberUsingSid(PSID groupSid, BSTR userGuid);

    BOOL CheckIsMemberUsingToken(PSID userSid, PSID groupSid);

    HRESULT GetUserGroups(PACL pDacl, std::wstring sidString, std::vector <std::wstring> *groupsSids);

    HRESULT AddUserGroupsToContext(PAUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient, PAUTHZ_CLIENT_CONTEXT_HANDLE newContext,
                                   std::vector <std::wstring> groupsSids);

    BOOL RecursiveIsMember(IADsGroup *pADsGroup, BSTR userGuid);

    HRESULT CheckUserPermissionsUsingAuthz(PSECURITY_DESCRIPTOR pFileSD, std::wstring userSidString, PSID userSID,
                                           ACCESS_MASK accessMask, std::vector <std::wstring> groupsSids);

    HRESULT UserHasEnoughRights(AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient, PSECURITY_DESCRIPTOR pFileSD,
                                ACCESS_MASK accessMask);

    void MapFileSystemGenericMask(PACCESS_MASK accessMask);

    std::vector <EXPLICIT_ACCESS_W> explicitAccessList_;
    std::wstring folderPath_;

    typedef std::map <std::wstring, uint32_t> AccessMask;
    static AccessMask accessMask_;
};


#endif //NATIVE_ADVAPI_WIN_JS_FOLDERPERMISSIONSMANAGER_H
