# WIN-PERMISSIONS-JS
Folder Permissions Manager, Wrapper for Win32 Permissions

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
