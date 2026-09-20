import * as UE from "ue";
declare const require: (moduleName: string) => any;

const puerts = require("puerts") as {
  registerBuildinModule(
    moduleName: string,
    module: Record<string, unknown>,
  ): void;
};

puerts.registerBuildinModule("path", {
  dirname(path: string): string {
    return UE.TSToolBlueprintFunctionLibrary.GetDirectoryName(path);
  },
  resolve(dir: string, url: string): string {
    url = url.replace(/\\/g, "/");
    while (url.startsWith("../")) {
      dir = UE.TSToolBlueprintFunctionLibrary.GetDirectoryName(dir);
      url = url.substr(3);
    }
    return UE.TSToolBlueprintFunctionLibrary.CombinePath(dir, url);
  },
});
puerts.registerBuildinModule("fs", {
  existsSync(path: string): boolean {
    return UE.TSToolBlueprintFunctionLibrary.FileExists(path);
  },
  readFileSync(path: string): string {
    return UE.TSToolBlueprintFunctionLibrary.ReadAllText(path);
  },
});

const globalObject = globalThis as unknown as Record<string, unknown>;
globalObject.Buffer = globalObject.Buffer ?? {};

try {
  const sourceMapSupport = require("source-map-support") as {
    install?: () => void;
  };
  sourceMapSupport.install?.();
} catch (error) {
  const message = error instanceof Error ? error.message : String(error);
  console.warn(
    `[LogExt] source-map-support is unavailable; stack traces will use generated JavaScript locations. ${message}`,
  );
}
