const objectFactory = require("bindings")("addon");

const folderPath = "D:\\os\\0";
let nativeFpm = objectFactory(folderPath);

const userSidString = "S-1-5-21-1148199417-2901198143-3741518822-1001";
const accessString = "GE,GR,DC,AD,WD,WA,WEA,RA";

nativeFpm.checkUserPermissions(userSidString, accessString);