//
// Created by ubit-gng on 13.07.2020.
//

#include "FolderPermissionsManager.h"

using namespace Napi;

/***
 * Initializing Folder Permissions Manager
 * @param env
 * @param exports
 * @return
 */
Napi::Object FolderPermissionsManager::Init(Napi::Env env, Napi::Object exports) {

    Napi::Function func =
            DefineClass(env,
                        "FolderPermissionsManager", {
                                InstanceMethod("addRight", &FolderPermissionsManager::AddRight),
                                InstanceMethod("applyRights", &FolderPermissionsManager::ApplyRights),
                                InstanceMethod("clearExplicitAccessList",
                                               &FolderPermissionsManager::ClearExplicitAccessList),
                                InstanceMethod("checkUserPermissions", &FolderPermissionsManager::CheckUserPermissions)
                        });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("FolderPermissionsManager", func);
    return exports;
}

/***
 * Factory
 * @param env
 * @param arg0
 * @return
 */
Napi::Object FolderPermissionsManager::NewInstance(Napi::Env env, Napi::Value arg0) {
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({arg0});
    return scope.Escape(napi_value(obj)).ToObject();
}


/***
 * Constructor
 * @param info
 */
FolderPermissionsManager::FolderPermissionsManager(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<FolderPermissionsManager>(info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length != 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "1 String parameter is expected!").ThrowAsJavaScriptException();
        return;
    }

    folderPath_ = to_wstring(info[0]);
}


/***
 * Initializing accessMask_
 */
FolderPermissionsManager::AccessMask FolderPermissionsManager::accessMask_ = {
        {L"D",    0x00010000},
        {L"RC",   0x00020000},
        {L"WDAC", 0x00040000},
        {L"WO",   0x00080000},
        {L"S",    0x00100000},
        {L"AS",   0x01000000},
        {L"MA",   0x02000000},
        {L"GR",   0x80000000},
        {L"GW",   0x40000000},
        {L"GE",   0x20000000},
        {L"GA",   0x10000000},
        {L"RD",   0x00000001},
        {L"WD",   0x00000002},
        {L"AD",   0x00000004},
        {L"REA",  0x00000008},
        {L"WEA",  0x00000010},
        {L"X",    0x00000020},
        {L"DC",   0x00000040},
        {L"RA",   0x00000080},
        {L"WA",   0x00000100}
};

/***
 * Converting sid string to PSID
 * @param sidString
 * @param ppSid
 * @return
 */
HRESULT FolderPermissionsManager::GetSidFromString(std::wstring sidString, PSID *ppSid) {

    const DWORD INITIAL_SIZE = 32;
    DWORD dwSidBufferSize = INITIAL_SIZE;

    // Create buffers for the SID and the domain name.
    // no worries there: memset does not allocate memory.
    *ppSid = (PSID)
    new BYTE[dwSidBufferSize];
    memset(*ppSid, 0, dwSidBufferSize);


    if (!ConvertStringSidToSidW(sidString.c_str(), ppSid)) {
        DWORD dwErrorCode = GetLastError();
        return HRESULT_FROM_WIN32(dwErrorCode);
    }

    //this shouldn't happen, but you never know...
    if (!IsValidSid(*ppSid)) {
        return HRESULT_FROM_WIN32(CO_E_INVALIDSID);
    }

    return S_OK;
}

HRESULT FolderPermissionsManager::GetGuidFromSidString(std::wstring sidString, BSTR *guidValue) {

    std::wstring pszSearchBase = L"LDAP://<SID=" + sidString + L">";
    wprintf_s(pszSearchBase.c_str());
    printf_s("\n");

    IADsUser *pIADsUserFromSid = NULL;
    CoInitialize(NULL);

    HRESULT hr = ADsGetObject(pszSearchBase.c_str(),
                              IID_IADsUser,
                              (void **) &pIADsUserFromSid);

    if (SUCCEEDED(hr)) {
        pIADsUserFromSid->get_GUID(guidValue);
        pIADsUserFromSid->Release();
        pIADsUserFromSid = NULL;
    }

    CoUninitialize();

    return hr;
}

BOOL FolderPermissionsManager::CheckIsMemberUsingSid(PSID groupSid, BSTR guidValue) {

    LPWSTR sidString = NULL;
    HRESULT hr = ConvertSidToStringSidW(groupSid, &sidString);

    if (FAILED(hr)) {
        return FALSE;
    }

    std::wstring sidString_w(sidString);
    std::wstring pszSearchBase = L"LDAP://<SID=" + sidString_w + L">";

    wprintf_s(pszSearchBase.c_str());
    printf_s("\n");

    IADs *groupFromSid;
    hr = ADsGetObject(pszSearchBase.c_str(), IID_IADs, (void **) &groupFromSid);

    if (SUCCEEDED(hr)) {
        BSTR adName;
        groupFromSid->get_Name(&adName);
        wprintf_s(adName);
        printf_s("\n");

        VARIANT dnVariant;
        hr = groupFromSid->Get(CComBSTR(L"distinguishedName"), &dnVariant);

        if (SUCCEEDED(hr)) {

            CComBSTR dNCBStr = dnVariant.bstrVal;

            std::wstring dncString_w(dNCBStr);
            std::wstring newSearchBase = L"LDAP://" + dncString_w;

            IADsGroup *iaDsGroup = NULL;
            hr = ADsGetObject(newSearchBase.c_str(), IID_IADsGroup, (void **) &iaDsGroup);

            if (SUCCEEDED(hr)) {
                BSTR adsPath;
                iaDsGroup->get_ADsPath(&adsPath);
                wprintf_s(adsPath);
                printf_s("\n");

                BOOL result = RecursiveIsMember(iaDsGroup, guidValue);

                iaDsGroup->Release();
                iaDsGroup = NULL;

                return result;
            }
        }

        groupFromSid->Release();
        groupFromSid = NULL;
    }

    LocalFree(sidString);
    return FALSE;
}


/***
 * Getting Sid for Given Account or Active Directory Group.
 * https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms707085(v=vs.85)
 * @param domain
 * @param name
 * @param ppSid
 * @return
 */
HRESULT FolderPermissionsManager::GetSid(std::wstring domain, std::wstring name, PSID *ppSid) {

    std::wstring wszAccNameW = domain + L"\\" + name;

    // Create buffers that may be large enough.
    const DWORD INITIAL_SIZE = 32;
    DWORD dwSidBufferSize = INITIAL_SIZE;
    DWORD dwDomainBufferSize = INITIAL_SIZE;
    WCHAR *wszDomainName = NULL;
    SID_NAME_USE eSidType;
    HRESULT hr = S_OK;

    // Create buffers for the SID and the domain name.
    *ppSid = (PSID)
    new BYTE[dwSidBufferSize];
    memset(*ppSid, 0, dwSidBufferSize);

    wszDomainName = new WCHAR[dwDomainBufferSize];
    memset(wszDomainName, 0, dwDomainBufferSize * sizeof(WCHAR));

    // Obtain the SID for the account name passed.
    for (;;) {

        // Set the count variables to the buffer sizes and retrieve the SID.
        DWORD cbSid = dwSidBufferSize;
        DWORD cchDomainName = dwDomainBufferSize;
        if (LookupAccountNameW(
                NULL,            // Computer name. NULL for the local computer
                wszAccNameW.c_str(),
                *ppSid,          // Pointer to the SID buffer. Use NULL to get the size needed,
                &cbSid,          // Size of the SID buffer needed.
                wszDomainName,   // wszDomainName,
                &cchDomainName,
                &eSidType
        )) {
            if (!IsValidSid(*ppSid)) {
                wprintf(L"The SID for %s is invalid.\n", wszAccNameW.c_str());
            }
            break;
        }

        DWORD dwErrorCode = GetLastError();
        wprintf(L"LookupAccountNameW failed. GetLastError returned: %d\n", dwErrorCode);
        hr = HRESULT_FROM_WIN32(dwErrorCode);
        break;
    }

    delete[] wszDomainName;
    return hr;
}

/***
 * Converting Access Mask as comma separated string to DWORD
 * @param accessString
 * @return Access Mask as DWORD
 */
DWORD FolderPermissionsManager::ConvertStringToAccessMask(Napi::Env env, std::wstring accessString) {
    DWORD maskValue = 0;
    std::wstring temp;
    std::wstringstream wss(accessString);
    while (std::getline(wss, temp, L',')) {

        std::map<std::wstring, uint32_t>::iterator it = accessMask_.find(temp);

        //we should make sure element exists in map, otherwise throw
        if (it == accessMask_.end()) {
            createWindowsError(env, HRESULT_FROM_WIN32(EVENT_E_QUERYSYNTAX),
                               "ConvertStringToAccessMask").ThrowAsJavaScriptException();
            return maskValue;
        }

        maskValue |= it->second;
    }

    return maskValue;
}

//parameters domain, name, accessString, isUser, propagate
void FolderPermissionsManager::AddRight(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length != 5 || (!info[0].IsString() && !info[1].IsString() &&
                        !info[2].IsString() && !info[3].IsBoolean() && !info[4].IsBoolean())) {
        Napi::TypeError::New(env, "Expecting 5 Parameters (3 Strings + 2 Booleans)").ThrowAsJavaScriptException();
        return;
    }

    std::wstring domain = to_wstring(info[0]);
    std::wstring name = to_wstring(info[1]);
    std::wstring accessString = to_wstring(info[2]);
    auto isUser = info[3].As<Napi::Boolean>();
    auto propagate = info[4].As<Napi::Boolean>();

    //After Input Variables Check, making sure access mask string is valid
    auto grfAccessPermissions = FolderPermissionsManager::ConvertStringToAccessMask(env, accessString);

    //Then check if Group or User exists and Get SID
    PSID currentSid;
    HRESULT hr = GetSid(domain, name, &currentSid);
    if (!SUCCEEDED(hr)) {
        createWindowsError(env, hr, "GetSid").ThrowAsJavaScriptException();
        return;
    }

    EXPLICIT_ACCESS_W ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS_W));

    ea.grfAccessPermissions = grfAccessPermissions;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = propagate ? CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : NO_INHERITANCE;

    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = isUser ? TRUSTEE_IS_USER : TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPWSTR) currentSid;

    explicitAccessList_.push_back(ea);
}

/***
 * Clearing explicit Access List Vector, freeing Sids.
 * @param info
 */
void FolderPermissionsManager::ClearExplicitAccessList(const Napi::CallbackInfo &info) {
    if (explicitAccessList_.size() == 0) {
        return;
    }

    for (int i = 0; i < explicitAccessList_.size(); i++) {
        FreeSid(explicitAccessList_[i].Trustee.ptstrName);
    }

    std::vector<EXPLICIT_ACCESS_W>().swap(explicitAccessList_);
}

void FolderPermissionsManager::MapFileSystemGenericMask(PACCESS_MASK accessMask) {
    PGENERIC_MAPPING currentMapping = new GENERIC_MAPPING();

    currentMapping->GenericAll = STANDARD_RIGHTS_ALL;
    currentMapping->GenericRead = FILE_READ_ATTRIBUTES | FILE_READ_ACCESS | FILE_READ_EA | READ_CONTROL | SYNCHRONIZE;
    currentMapping->GenericWrite =
            FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | FILE_WRITE_EA | READ_CONTROL | SYNCHRONIZE;
    currentMapping->GenericExecute = FILE_EXECUTE | FILE_READ_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;

    MapGenericMask(accessMask, currentMapping);
}

HRESULT
FolderPermissionsManager::UserHasEnoughRights(AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient, PSECURITY_DESCRIPTOR pFileSD,
                                              ACCESS_MASK accessMask) {

    AUTHZ_ACCESS_REQUEST AccessRequest = {0};
    AUTHZ_ACCESS_REPLY AccessReply = {0};
    BYTE Buffer[1024];// assume error

    FolderPermissionsManager::MapFileSystemGenericMask(&accessMask);

    //  Do AccessCheck.
    AccessRequest.DesiredAccess = accessMask;
    AccessRequest.PrincipalSelfSid = NULL;
    AccessRequest.ObjectTypeList = NULL;
    AccessRequest.ObjectTypeListLength = 0;
    AccessRequest.OptionalArguments = NULL;

    RtlZeroMemory(Buffer, sizeof(Buffer));
    AccessReply.ResultListLength = 1;
    AccessReply.GrantedAccessMask = (PACCESS_MASK)(Buffer);
    AccessReply.Error = (PDWORD)(Buffer + sizeof(ACCESS_MASK));


    if (!AuthzAccessCheck(0,
                          hAuthzClient,
                          &AccessRequest,
                          NULL,
                          pFileSD,
                          NULL,
                          0,
                          &AccessReply,
                          NULL)) {

        printf_s("Auth Access Check failed.\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (*AccessReply.GrantedAccessMask & accessMask) {
        printf_s("Access Granted!\n");
        return S_OK;
    }

    return HRESULT_FROM_WIN32(SE_ERR_ACCESSDENIED);
}

HRESULT FolderPermissionsManager::CheckUserPermissionsUsingAuthz(PSECURITY_DESCRIPTOR pFileSD,
                                                                 std::wstring userSidString,
                                                                 PSID userSID,
                                                                 ACCESS_MASK accessMask,
                                                                 std::vector <std::wstring> groupsSids) {
    AUTHZ_RESOURCE_MANAGER_HANDLE hManager;

    BOOL bResult = AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT, NULL, NULL, NULL, NULL, &hManager);
    if (!bResult) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LUID unusedId = {0};
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;
    HRESULT hr = S_OK;

    if (AuthzInitializeContextFromSid(0,
                                      userSID,
                                      hManager,
                                      NULL,
                                      unusedId,
                                      NULL,
                                      &hAuthzClientContext)) {

        AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContextWithGroups = NULL;
        hr = AddUserGroupsToContext(&hAuthzClientContext, &hAuthzClientContextWithGroups, groupsSids);

        if (SUCCEEDED(hr)) {
            if (hAuthzClientContextWithGroups != NULL) {
                AuthzFreeContext(hAuthzClientContext);
                hr = UserHasEnoughRights(hAuthzClientContextWithGroups, pFileSD, accessMask);
                AuthzFreeContext(hAuthzClientContextWithGroups);
            } else {
                hr = UserHasEnoughRights(hAuthzClientContext, pFileSD, accessMask);
                AuthzFreeContext(hAuthzClientContext);
            }
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    AuthzFreeResourceManager(hManager);
    return hr;
}

HRESULT FolderPermissionsManager::GetObjectGuid(IDirectoryObject *pDO, BSTR *bsGuid) {
    GUID *pObjectGUID = NULL;
    PADS_ATTR_INFO pAttributeEntries;
    LPWSTR pAttributeName = L"objectGUID";
    DWORD dwAttributesReturned = 0;
    HRESULT hr;

    if (!pDO) {
        return E_FAIL;
    }

    hr = pDO->GetObjectAttributes(&pAttributeName, 1, &pAttributeEntries, &dwAttributesReturned);

    if (SUCCEEDED(hr) && dwAttributesReturned > 0) {
        if (pAttributeEntries->dwADsType == ADSTYPE_OCTET_STRING) {
            pObjectGUID = (GUID * )(pAttributeEntries->pADsValues[0].OctetString.lpValue);

            LPOLESTR szGUID = new WCHAR[64];
            szGUID[0] = NULL;
            // Convert GUID to string.
            ::StringFromGUID2(*pObjectGUID, szGUID, 39);
            *bsGuid = SysAllocString(szGUID);
            delete[] szGUID;
        }
    }

    return hr;

}

/***
 * We need to check if user is member of an authorized group
 * @param pADsGroup
 * @param userGuid
 * @return
 */
BOOL FolderPermissionsManager::RecursiveIsMember(IADsGroup *pADsGroup, BSTR userGuid) {

    BSTR adsPath;
    pADsGroup->get_ADsPath(&adsPath);
    wprintf_s(adsPath);
    printf_s("\n");

    HRESULT hr = S_OK;
    BOOL bRet = FALSE;

    IADsMembers *pADsMembers = NULL;
    hr = pADsGroup->Members(&pADsMembers);

    if (SUCCEEDED(hr)) {

        IUnknown *pUnKnown = NULL;
        hr = pADsMembers->get__NewEnum(&pUnKnown);

        if (SUCCEEDED(hr)) {

            IEnumVARIANT *pEnumVariant = NULL;
            hr = pUnKnown->QueryInterface(IID_IEnumVARIANT, (void **) &pEnumVariant);

            if (SUCCEEDED(hr)) {

                BOOL fContinue = TRUE;

                while (fContinue) {
                    ULONG ulElementsFetched = 0;
                    VARIANT VariantArray[10];

                    hr = ADsEnumerateNext(pEnumVariant, 10,
                                          VariantArray, &ulElementsFetched);

                    if (ulElementsFetched) {
                        for (ULONG i = 0; i < ulElementsFetched; i++) {
                            // Pointer for holding dispatch of element:
                            IDispatch *pDispatch = NULL;
                            BSTR bstrCurrentPath = NULL;
                            BSTR bstrGuidCurrent = NULL;
                            IDirectoryObject *pIDOCurrent = NULL;

                            // Get the dispatch pointer for the variant
                            pDispatch = VariantArray[i].pdispVal;
                            hr = pDispatch->QueryInterface(IID_IDirectoryObject,
                                                           (VOID * *) & pIDOCurrent);

                            if (SUCCEEDED(hr)) {

                                hr = GetObjectGuid(pIDOCurrent, &bstrGuidCurrent);

                                if (FAILED(hr)) {
                                    return hr;
                                }

                                IADs *pIADsCurrent = NULL;
                                hr = pIDOCurrent->QueryInterface(IID_IADs, (void **) &pIADsCurrent);

                                if (FAILED(hr)) {
                                    return hr;
                                }

                                hr = pIADsCurrent->get_ADsPath(&bstrCurrentPath);

                                if (SUCCEEDED(hr)) {

                                    if (_wcsicmp(bstrGuidCurrent, userGuid) == 0) {
                                        bRet = TRUE;
                                        break;
                                    }

                                    IADsGroup *pIADsGroupAsMember = NULL;
                                    hr = ADsGetObject(bstrCurrentPath,
                                                      IID_IADsGroup,
                                                      (void **) &pIADsGroupAsMember);

                                    if (SUCCEEDED(hr)) {

                                        BOOL bRetRecurse = RecursiveIsMember(pIADsGroupAsMember, userGuid);
                                        if (bRetRecurse) {
                                            bRet = TRUE;
                                            break;
                                        }
                                        pIADsGroupAsMember->Release();
                                        pIADsGroupAsMember = NULL;
                                    }
                                    SysFreeString(bstrCurrentPath);
                                    bstrCurrentPath = NULL;

                                    SysFreeString(bstrGuidCurrent);
                                    bstrGuidCurrent = NULL;
                                }

                                pIDOCurrent->Release();
                                pIDOCurrent = NULL;

                                if (pIADsCurrent) {
                                    pIADsCurrent->Release();
                                    pIADsCurrent = NULL;
                                }

                            }
                        }
                        memset(VariantArray, 0, sizeof(VARIANT) * 10);
                    } else {
                        fContinue = FALSE;
                    }
                }
                pEnumVariant->Release();
                pEnumVariant = NULL;
            }
            pUnKnown->Release();
            pUnKnown = NULL;
        }
        pADsMembers->Release();
        pADsMembers = NULL;
    }

    return bRet;
}

HRESULT
FolderPermissionsManager::GetUserGroups(PACL pDacl, std::wstring sidString, std::vector <std::wstring> *groupsSids) {

    ULONG entriesNbr = 0;
    PEXPLICIT_ACCESS_W entriesArray;

    BSTR guidValue = NULL;
    HRESULT hr = GetGuidFromSidString(sidString, &guidValue);

    if (FAILED(hr)) {
        return hr;
    }

    hr = GetExplicitEntriesFromAclW(pDacl, &entriesNbr, &entriesArray);

    if (FAILED(hr)) {
        return hr;
    }

    CoInitialize(NULL);

    for (ULONG i = 0; i < entriesNbr; i++) {
        EXPLICIT_ACCESS_W current = entriesArray[i];

        if (current.Trustee.TrusteeForm != TRUSTEE_IS_SID) {
            continue;
        }


        BOOL result = CheckIsMemberUsingSid((PSID) current.Trustee.ptstrName, guidValue);

        if (result) {
            LPWSTR sidString = NULL;
            HRESULT hr = ConvertSidToStringSidW((PSID) current.Trustee.ptstrName, &sidString);

            if (FAILED(hr)) {
                continue;
            }

            std::wstring sidString_w(sidString);
            std::wstring whatever = L"current sid:" + sidString_w + L"\n";
            wprintf_s(whatever.c_str());

            groupsSids->push_back(sidString_w);
        }
    }

    CoUninitialize();

    LocalFree(entriesArray);
    return hr;
}

HRESULT FolderPermissionsManager::AddUserGroupsToContext(PAUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClient,
                                                         PAUTHZ_CLIENT_CONTEXT_HANDLE newContext,
                                                         std::vector <std::wstring> groupsSids) {

    HRESULT hr = S_OK;

    if (groupsSids.size() == 0) {
        return hr;
    }

    PSID_AND_ATTRIBUTES usersSids = new SID_AND_ATTRIBUTES[groupsSids.size()];
    for (int i = 0; i < groupsSids.size(); i++) {
        GetSidFromString(groupsSids[i], &(usersSids[i].Sid));
        usersSids[i].Attributes = SE_GROUP_ENABLED;
    }

    SID_AND_ATTRIBUTES denySids = {0};
    if (!AuthzAddSidsToContext(*hAuthzClient, usersSids, groupsSids.size(), &denySids, 0, newContext)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //freeing resources
    for (int i = 0; i < groupsSids.size(); i++) {
        FreeSid(usersSids[i].Sid);
    }

    LocalFree(usersSids);

    return hr;
}

/***
 * Checking user's permissions on folder against a given access mask.
 * User must at least have the needed permissions on the folder, otherwise
 * function will throw an exception
 * @param info
 */
void FolderPermissionsManager::CheckUserPermissions(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length != 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Expecting 2 String parameters").ThrowAsJavaScriptException();
        return;
    }

    std::wstring userSidString = to_wstring(info[0]);
    std::wstring accessString = to_wstring(info[1]);

    //After Input Variables Check, making sure access mask string is valid
    ACCESS_MASK grfAccessPermissions = FolderPermissionsManager::ConvertStringToAccessMask(env, accessString);

    //Now getting user's SID
    PSID userSid;
    HRESULT hr = FolderPermissionsManager::GetSidFromString(userSidString, &userSid);

    if (SUCCEEDED(hr)) {

        PSECURITY_DESCRIPTOR pFileSD = NULL;
        PACL pDacl = NULL;

        hr = GetNamedSecurityInfoW((LPWSTR) folderPath_.c_str(), SE_FILE_OBJECT,
                                   DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                                   NULL, NULL, &pDacl, NULL, &pFileSD);

        if (SUCCEEDED(hr)) {

            std::vector <std::wstring> usersSids;
            hr = GetUserGroups(pDacl, userSidString, &usersSids);

            if (SUCCEEDED(hr)) {
                hr = CheckUserPermissionsUsingAuthz(pFileSD, userSidString, userSid, grfAccessPermissions, usersSids);
            }

            LocalFree(pFileSD);
        }

        FreeSid(userSid);
    }

    if (!SUCCEEDED(hr)) {
        createWindowsError(env, hr, "CheckUserPermissions").ThrowAsJavaScriptException();
        return;
    }
}

/***
 * Applying Rights to Folder
 * @param info
 */
void FolderPermissionsManager::ApplyRights(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    int length = info.Length();
    if (length != 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Expecting 1 Boolean Parameter").ThrowAsJavaScriptException();
        return;
    }

    if (explicitAccessList_.size() == 0) {
        Napi::Error::New(env, "Explicit Access List is empty").ThrowAsJavaScriptException();
        return;
    }

    auto disableInheritance = info[0].As<Napi::Boolean>();
    PACL pNewDAcl = NULL;

    HRESULT hr = SetEntriesInAclW(explicitAccessList_.size(), &(explicitAccessList_[0]), NULL, &pNewDAcl);
    if (SUCCEEDED(hr)) {
        hr = SetNamedSecurityInfoW((LPWSTR) folderPath_.c_str(), SE_FILE_OBJECT,
                                   disableInheritance ? DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION
                                                      : DACL_SECURITY_INFORMATION, NULL, NULL, pNewDAcl, NULL);
    }

    // Making sure Resources are freed
    if (pNewDAcl) {
        LocalFree(pNewDAcl);
    }

    FolderPermissionsManager::ClearExplicitAccessList(info);

    if (!SUCCEEDED(hr)) {
        createWindowsError(env, GetLastError(), "ApplyRights").ThrowAsJavaScriptException();
    }
}