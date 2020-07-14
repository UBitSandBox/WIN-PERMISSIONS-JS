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
                        "FolderPermissionsManager",{
                                InstanceMethod("addRight", &FolderPermissionsManager::AddRight),
                                InstanceMethod("applyRights", &FolderPermissionsManager::ApplyRights),
                                InstanceMethod("clearExplicitAccessList", &FolderPermissionsManager::ClearExplicitAccessList)
                        });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
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
        : Napi::ObjectWrap<FolderPermissionsManager>(info){
    Napi::Env env = info.Env();

    int length = info.Length();
    if(length != 1 || !info[0].IsString()){
        Napi::TypeError::New(env, "1 String parameter is expected!").ThrowAsJavaScriptException();
        return;
    }

    folderPath_ = to_wstring(info[0]);
}


/***
 * Initializing accessMask_
 */
FolderPermissionsManager::AccessMask FolderPermissionsManager::accessMask_ = {
        {L"D",0x00010000},
        {L"RC",0x00020000},
        {L"WDAC",0x00040000},
        {L"WO", 0x00080000},
        {L"S",0x00100000},
        {L"AS",0x01000000},
        {L"MA",0x02000000},
        {L"GR",0x80000000},
        {L"GW",0x40000000},
        {L"GE",0x20000000},
        {L"GA",0x10000000},
        {L"RD",0x00000001},
        {L"WD",0x00000002},
        {L"AD",0x00000004},
        {L"REA",0x00000008},
        {L"WEA",0x00000010},
        {L"X",0x00000020},
        {L"DC",0x00000040},
        {L"RA",0x00000080},
        {L"WA",0x00000100}
};


/***
 * Getting Sid for Given Account or Active Directory Group.
 * https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms707085(v=vs.85)
 * @param domain
 * @param name
 * @param ppSid
 * @return
 */
HRESULT FolderPermissionsManager::GetSid(std::wstring domain, std::wstring name, PSID* ppSid){

    std::wstring wszAccNameW = domain + L"\\" + name;

    // Create buffers that may be large enough.
    const DWORD INITIAL_SIZE = 32;
    DWORD dwSidBufferSize = INITIAL_SIZE;
    DWORD dwDomainBufferSize = INITIAL_SIZE;
    WCHAR * wszDomainName = NULL;
    SID_NAME_USE eSidType;
    HRESULT hr = S_OK;

    // Create buffers for the SID and the domain name.
    *ppSid = (PSID) new BYTE[dwSidBufferSize];
    memset(*ppSid, 0, dwSidBufferSize);

    wszDomainName = new WCHAR[dwDomainBufferSize];
    memset(wszDomainName, 0, dwDomainBufferSize*sizeof(WCHAR));

    // Obtain the SID for the account name passed.
    for ( ; ; )
    {

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
        ))
        {
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

    delete [] wszDomainName;
    return hr;
}

/***
 * Converting Access Mask as comma separated string to DWORD
 * @param accessString
 * @return Access Mask as DWORD
 */
DWORD FolderPermissionsManager::ConvertStringToAccessMask(std::wstring accessString){
    DWORD maskValue = 0;
    std::wstring temp;
    std::wstringstream wss(accessString);
    while(std::getline(wss, temp, L',')){
        maskValue |= accessMask_[temp];
    }

    return maskValue;
}

//parameters domain, name, accessString, isUser, propagate
void FolderPermissionsManager::AddRight(const Napi::CallbackInfo &info){
    Napi::Env env = info.Env();

    int length = info.Length();
    if(length != 5 || (!info[0].IsString() && !info[1].IsString() &&
        !info[2].IsString() && !info[3].IsBoolean() && !info[4].IsBoolean())){
        Napi::TypeError::New(env, "Expecting 5 Parameters (3 Strings + 2 Booleans)").ThrowAsJavaScriptException();
        return;
    }

    std::wstring domain = to_wstring(info[0]);
    std::wstring name = to_wstring(info[1]);
    std::wstring accessString = to_wstring(info[2]);
    auto isUser = info[3].As<Napi::Boolean>();
    auto propagate = info[4].As<Napi::Boolean>();

    //first check if Group or User exists and Get SID
    PSID currentSid;
    HRESULT hr = FolderPermissionsManager::GetSid(domain, name, &currentSid);
    if(!SUCCEEDED(hr)){
        createWindowsError(env, GetLastError(), "GetSid").ThrowAsJavaScriptException();
        return;
    }

    EXPLICIT_ACCESS_W ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS_W));

    ea.grfAccessPermissions = FolderPermissionsManager::ConvertStringToAccessMask(accessString) ;
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
void FolderPermissionsManager::ClearExplicitAccessList(const Napi::CallbackInfo &info){
    if(explicitAccessList_.size() == 0){
        return;
    }

    for(int i=0; i<explicitAccessList_.size();i++){
        FreeSid(explicitAccessList_[i].Trustee.ptstrName);
    }

    std::vector<EXPLICIT_ACCESS_W>().swap(explicitAccessList_);
}

/***
 * Applying Rights to Folder
 * @param info
 */
void FolderPermissionsManager::ApplyRights(const Napi::CallbackInfo &info){
    Napi::Env env = info.Env();

    int length = info.Length();
    if(length != 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Expecting 1 Boolean Parameter").ThrowAsJavaScriptException();
        return;
    }

    if(explicitAccessList_.size() == 0){
        return;
    }

    auto disableInheritance = info[0].As<Napi::Boolean>();
    PACL pNewDAcl = NULL;

    HRESULT hr = SetEntriesInAclW(explicitAccessList_.size(), &(explicitAccessList_[0]), NULL, &pNewDAcl);
    if(!SUCCEEDED(hr)){
        createWindowsError(env, GetLastError(), "ApplyRights").ThrowAsJavaScriptException();
        goto Cleanup;
    }

    hr = SetNamedSecurityInfoW((LPWSTR)folderPath_.c_str(), SE_FILE_OBJECT,
            disableInheritance?DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION:DACL_SECURITY_INFORMATION, NULL, NULL, pNewDAcl, NULL);

    if(!SUCCEEDED(hr)){
        createWindowsError(env, GetLastError(), "ApplyRights").ThrowAsJavaScriptException();
        goto Cleanup;
    }

    Cleanup:
        if(pNewDAcl){
            LocalFree(pNewDAcl);
        }

        FolderPermissionsManager::ClearExplicitAccessList(info);
        return;
}