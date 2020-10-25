const objectFactory = require("bindings")("addon");

function FolderPermissionsManager({folderPath}){
    this.nativeFpm = objectFactory(folderPath);
}

FolderPermissionsManager.prototype.checkUserPermissions = function({userSidString, accessString}){
    return this.nativeFpm.checkUserPermissions(userSidString, accessString);
};

//parameters domain, name, accessString, isUser, propagate
FolderPermissionsManager.prototype.addRight = function({domain, name, accessString, isUser = false, propagate = true}){
    return this.nativeFpm.addRight(domain, name, accessString, isUser, propagate);
};

FolderPermissionsManager.prototype.applyRights = function({disableInheritance=false}){
    return this.nativeFpm.applyRights(disableInheritance);
};

FolderPermissionsManager.prototype.clearExplicitAccessList = function(){
    return this.nativeFpm.clearExplicitAccessList();
};

module.exports = FolderPermissionsManager;