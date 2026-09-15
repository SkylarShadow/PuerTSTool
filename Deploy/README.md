# PuerTS TypeScript Engineering Template

This folder contains the lightweight ESLint and Prettier setup used to standardize business-side PuerTS TypeScript code.

## Usage

Copy all files in this folder to the root of a UE project that contains a `TypeScript` folder.

If you want VSCode task buttons too, also copy the `.vscode` folder.

Or run the deployment script:

```bat
DeployPuerTSEnv.bat D:\Path\To\YourUEProject
```

If no project path is provided, the script walks upward from the current directory and the script directory until it finds a `.uproject`. If it cannot find one, it asks for the UE project root manually.

The script copies only the engineering config files. It does not copy `.vscode/tasks.json`, so existing VSCode project tasks will not be overwritten.

`npm install` runs by default after copying files:

```bat
DeployPuerTSEnv.bat D:\Path\To\YourUEProject
```

To skip dependency installation:

```bat
DeployPuerTSEnv.bat D:\Path\To\YourUEProject --no-install
```

The script follows `PuerTS框架使用指南.md` step by step. It does not silently skip required deployment stages:

1. Locates the UE project root by walking upward until it finds a `.uproject`, or asks for the path.
2. Checks Node.js and npm.
3. Looks for Puerts at `<ProjectRoot>/Plugins/puerts` or `<ProjectRoot>/Plugins/Puerts`; if neither exists, asks for the Puerts plugin directory.
4. Runs `node enable_puerts_module.js`.
5. Verifies that `TypeScript` and `tsconfig.json` exist.
6. Copies the TS engineering config files.
7. Runs `npm install` by default.
8. Stops for manual confirmation of backend/V8 and `GetJsEnv` patch steps.
9. Stops for manual confirmation that the UE project was compiled and GenDTS was run.
10. Runs `npm run check`.

Manual UE-only steps are shown in order and require confirmation before the script continues.

Then run the checks:

```sh
npm run check
```

For daily formatting and auto-fixes:

```sh
npm run fix
```

## VSCode Tasks

After copying `.vscode/tasks.json` to the UE project root, open the project root in VSCode and run:

```text
Terminal -> Run Task...
```

Then choose one of:

- `PuerTS: Fix TS Code`
- `PuerTS: Check TS Code`
- `PuerTS: Format TS Code`
- `PuerTS: Lint TS Code`
- `PuerTS: Type Check`
- `PuerTS: Build TS`
- `PuerTS: Watch TS`
- `PuerTSTool: Check VSCode Bridge`

## Scripts

- `npm run lint`: Fix ESLint issues in `TypeScript`.
- `npm run lint:check`: Check ESLint issues without writing files.
- `npm run format`: Format TypeScript and root config files.
- `npm run format:check`: Check formatting without writing files.
- `npm run fix`: Run lint fixes and Prettier formatting.
- `npm run check`: Run lint and format checks.
