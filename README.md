# WIN-PERMISSIONS-JS

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d940d6d2301b41ca8894582696618d91)](https://app.codacy.com/gh/UBitSandBox/WIN-PERMISSIONS-JS?utm_source=github.com&utm_medium=referral&utm_content=UBitSandBox/WIN-PERMISSIONS-JS&utm_campaign=Badge_Grade_Dashboard)

Folder Permissions Manager, Wrapper for Win32 Permissions

###### Visual Studio Build Tools 2019

Manually: https://visualstudio.microsoft.com/fr/visual-cpp-build-tools/

Visual Studio Build Tools > 2017 are needed for win-permissions-js (C++ 17)!!!

```powershell
$buildToolsUrl="https://aka.ms/vs/16/release/vs_buildtools.exe"
$buildToolsPath=Join-Path $base_install_path 'BuildTools'

# Big mistake here, we should use Microsoft.VisualStudio.Component.VC.Tools.x86.x64 and Microsoft.VisualStudio.Component.VC.v141.x86.x64
# instead of Microsoft.VisualStudio.Workload.AzureBuildTools

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $buildToolsUrl -OutFile "vs_buildtools.exe"
Start-Process -Wait -NoNewWindow -FilePath .\vs_buildtools.exe -ArgumentList '--quiet', '--wait', '--norestart', '--nocache', $('--installPath {0}' -f $buildToolsPath),
    '--add Microsoft.VisualStudio.Component.VC.CoreBuildTools',
    '--add Microsoft.VisualStudio.Component.VC.Redist.14.Latest',
    '--add Microsoft.VisualStudio.Component.Windows10SDK',
    '--add Microsoft.VisualStudio.Component.TestTools.BuildTools',
    '--add Microsoft.VisualStudio.Component.VC.ASAN',
    '--add Microsoft.VisualStudio.Component.VC.CMake.Project',
    '--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    '--add Microsoft.VisualStudio.Component.Windows10SDK.18362'

$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

```
###### Build Tools (Visual Studio Build Tools 2017 + Python 2.7) for gyp

https://www.npmjs.com/package/windows-build-tools

```powershell

npm config set proxy http://proxy.test.org:8080
npm config set https-proxy http://proxy.test.org:8080

npm install --global --production --add-python-to-path windows-build-tools

#Refreshing PATH Variable after build tools installation
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
```
Usage
```javascript

import winPermissionsManager       from "win-permissions-js";

/**
 * Setting Access Rights on a Folder. This function is intended for
 * Windows Use only.
 * @param folderPath
 * @param rights
 * @returns {Promise<void>}
 */
export const setAccessRightsOnFolder =  ({folderPath, rights}) =>
    new Promise(resolve => {
        let currentPermissionsManager = new winPermissionsManager({folderPath});
        rights.forEach(right => {
            currentPermissionsManager.addRight(right);
        });
        currentPermissionsManager.applyRights({disableInheritance: true});
        resolve();
    });

```

```javascript
//folderPath: current folder Path
let currentPermissionsManager = new winPermissionsManager({folderPath});

//domain: user or group domain
//name: user- or group- name
//accessString: Access Mask as String
//isUser: user or group account?
//propagate: Should propagate Rights or not (CI)(OI) vs. (NP)
currentPermissionsManager.addRight({domain, name, accessString, isUser, propagate});

//applying new Permissions (SET) on folder
//disabling Inheritance if desired with parameter disableInheritance:true
currentPermissionsManager.applyRights({disableInheritance:true});

//you should be careful with applyRights and disableInheritance:true! You could end up resetting the Folder Rights

```

## Access Mask as comma separated String
Please Read: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/icacls

```c++
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

/*
 * const modify = "GE,GR,DC,AD,WD,WA,WEA,RA"; (modify without right to delete current folder, only sub folders)
 * const read = "GE,GR,RA"; 
 * const fullControl = "GA";
 */
```
