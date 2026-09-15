# PuerTS Tool VSCode Extension

This extension adds CodeLens actions for PuerTS files that contain an `AssetPath` constant:

用于从vscode快速打开蓝图，要符合以下的mixin方式
```ts
const AssetPath = "/Game/Blueprint/Test/BP_TestActor2.BP_TestActor2_C";
```

It also adds a PuerTS type declaration hover. When TypeScript can resolve a variable's type, hover on the symbol to preview the corresponding declaration from generated declaration files such as `ue.d.ts`.

If the declaration is too long, use the hover links:

- `Peek Declaration`: opens VSCode's inline peek panel at the declaration location.
- `Open Declaration`: opens the declaration file directly.

## Package

Run `PackageVSIX.bat` in this directory. The script compiles the extension and writes a `.vsix` using the extension `name` and `version` from `package.json`.
