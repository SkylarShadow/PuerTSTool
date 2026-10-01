# PuerTS Tool VSCode Extension

This extension adds CodeLens actions for PuerTS files that contain an `assetPath` or `AssetPath` constant:

用于从vscode快速打开蓝图，要符合以下的mixin方式
```ts
const assetPath = "/Game/Blueprint/Test/BP_TestActor2.BP_TestActor2_C";
```

It also adds a PuerTS type declaration hover. When TypeScript can resolve a variable's type, hover on the symbol to preview the corresponding declaration from generated declaration files such as `ue.d.ts`.

If the declaration is too long, use the hover links:

- `Peek Declaration`: opens VSCode's inline peek panel at the declaration location.
- `Open Declaration`: opens the declaration file directly.

## DTS API Completion

When writing TypeScript, this extension indexes `Typing/**/*.d.ts` and adds PuerTS API suggestions directly to VSCode IntelliSense. It supports regular names, camel-case fragments, abbreviations, and multiple words.

```ts
get controller
```

can suggest `UE.GameplayStatics.GetPlayerController`. When writing a member expression such as `UE.GameplayStatics.Get...`, it only suggests members of that type and inserts the member name instead of duplicating the receiver. The index is rebuilt when a declaration file under `Typing` changes.

The extension also provides built-in PuerTSTool helper completions. For example, typing:

```ts
GetWorld
```

can suggest `PuerTS API -> Misc.GetWorld()` and insert:

```ts
Misc.GetWorld()
```

This helper completion does not add imports automatically, so mixin files should already import `Misc`.

## Package

Run `PackageVSIX.bat` in this directory. The script compiles the extension and writes a `.vsix` using the extension `name` and `version` from `package.json`.
