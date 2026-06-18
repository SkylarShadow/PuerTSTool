import * as UE from "ue";
var puerts = require("puerts");

puerts.registerBuildinModule("path", {
    dirname(path) {
        return UE.TSToolBlueprintFunctionLibrary.GetDirectoryName(path);
    },
    resolve(dir, url) {
        url = url.replace(/\\/g, "/");
        while (url.startsWith("../")) {
            dir = UE.TSToolBlueprintFunctionLibrary.GetDirectoryName(dir);
            url = url.substr(3);
        }
        return UE.TSToolBlueprintFunctionLibrary.CombinePath(dir, url);
    },
});
puerts.registerBuildinModule("fs", {
    existsSync(path) {
        return UE.TSToolBlueprintFunctionLibrary.FileExists(path);
    },
    readFileSync(path) {
        return UE.TSToolBlueprintFunctionLibrary.ReadAllText(path);
    },
});
(function () {
    let global = this ?? globalThis;
    global["Buffer"] = global["Buffer"] ?? {};
})();

require("source-map-support").install();