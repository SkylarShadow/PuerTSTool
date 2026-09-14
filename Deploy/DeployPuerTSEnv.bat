@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SOURCE_DIR=%~dp0"
set "TARGET_DIR="
set "RUN_NPM_INSTALL=1"

:ParseArgs
if "%~1"=="" goto :ParseArgsDone
if /I "%~1"=="--install" (
    set "RUN_NPM_INSTALL=1"
    shift
    goto :ParseArgs
)
if /I "%~1"=="/install" (
    set "RUN_NPM_INSTALL=1"
    shift
    goto :ParseArgs
)
if /I "%~1"=="--no-install" (
    set "RUN_NPM_INSTALL=0"
    shift
    goto :ParseArgs
)
if /I "%~1"=="/no-install" (
    set "RUN_NPM_INSTALL=0"
    shift
    goto :ParseArgs
)
if "%TARGET_DIR%"=="" (
    set "TARGET_DIR=%~1"
)
shift
goto :ParseArgs

:ParseArgsDone

if "%TARGET_DIR%"=="" (
    call :FindProjectRoot "%CD%" TARGET_DIR
)

if "%TARGET_DIR%"=="" (
    call :FindProjectRoot "%SOURCE_DIR%" TARGET_DIR
)

if "%TARGET_DIR%"=="" (
    echo [PuerTSTool] Could not find a .uproject by walking upward from:
    echo   %CD%
    echo   %SOURCE_DIR%
    echo.
    set /p "TARGET_DIR=Input UE project root directory: "
)

if "%TARGET_DIR%"=="" (
    echo [PuerTSTool] Target directory is empty.
    exit /b 1
)

for %%I in ("%TARGET_DIR%") do set "TARGET_DIR=%%~fI"

echo [PuerTSTool] === PuerTS TypeScript environment deploy ===
echo Source: %SOURCE_DIR%
echo Target: %TARGET_DIR%
echo.

if not exist "%TARGET_DIR%" (
    echo [PuerTSTool] Target directory does not exist:
    echo %TARGET_DIR%
    exit /b 1
)

call :CheckCommand node "Node.js is required. Install Node.js first, then run this script again." || exit /b 1
call :CheckCommand npm "npm is required. Reinstall Node.js or check PATH." || exit /b 1

echo [PuerTSTool] Node version:
node -v
echo [PuerTSTool] npm version:
npm -v
echo.

if not exist "%TARGET_DIR%\TypeScript" (
    echo [PuerTSTool] Warning: TypeScript directory was not found:
    echo %TARGET_DIR%\TypeScript
    echo.
    echo The guide expects Puerts enable_puerts_module.js or PuerTSTool editor deploy to create TypeScript.
    echo Continue deploying engineering config only.
    echo.
)

if not exist "%TARGET_DIR%\tsconfig.json" (
    echo [PuerTSTool] Warning: tsconfig.json was not found in project root.
    echo Run Puerts enable_puerts_module.js or generate the TS project config before using npm run build/watch.
    echo.
)

echo [PuerTSTool] Copying engineering config files...
copy /Y "%SOURCE_DIR%package.json" "%TARGET_DIR%\package.json" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%eslint.config.mjs" "%TARGET_DIR%\eslint.config.mjs" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.prettierrc" "%TARGET_DIR%\.prettierrc" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.prettierignore" "%TARGET_DIR%\.prettierignore" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.editorconfig" "%TARGET_DIR%\.editorconfig" >nul || goto :copy_failed
echo [PuerTSTool] Config files copied.
echo.

if "%RUN_NPM_INSTALL%"=="1" (
    echo [PuerTSTool] Running npm install in target project...
    pushd "%TARGET_DIR%" || exit /b 1
    npm install
    if errorlevel 1 (
        popd
        echo [PuerTSTool] npm install failed.
        echo Check whether Content\JavaScript\PuertsEditor exists when package.json uses local puerts-editor dependency.
        exit /b 1
    )
    popd
) else (
    echo [PuerTSTool] npm install was skipped.
    echo To install dependencies later:
    echo   cd /d "%TARGET_DIR%"
    echo   npm install
    echo.
    echo npm install runs by default. Use --no-install only when dependencies are already installed.
)

echo.
echo [PuerTSTool] Manual checklist from PuerTS guide:
echo   1. Puerts plugin has been installed under Plugins.
echo   2. In the Puerts plugin directory, run node enable_puerts_module.js once if the project has not been initialized.
echo   3. Ensure V8 or the selected JS backend exists under Puerts\ThirdParty and JsEnv.Build.cs points to it.
echo   4. Compile the UE project and open the editor.
echo   5. Click GenDTS in the editor to generate UE TypeScript declarations.
echo   6. Use npm run build, npm run watch, npm run check, or VSCode tasks after npm install.
echo   7. For packaging, include Content\JavaScript in UE packaging settings.
echo.
echo [PuerTSTool] VSCode tasks:
echo   This script does not copy .vscode\tasks.json.
echo   Copy %SOURCE_DIR%.vscode manually if you want VSCode task buttons.
echo.
echo [PuerTSTool] Done.
exit /b 0

:CheckCommand
where %~1 >nul 2>nul
if errorlevel 1 (
    echo [PuerTSTool] %~2
    exit /b 1
)
exit /b 0

:FindProjectRoot
set "SEARCH_DIR=%~f1"
set "%~2="

:FindProjectRootLoop
if "%SEARCH_DIR%"=="" exit /b 0
if exist "%SEARCH_DIR%\*.uproject" (
    set "%~2=%SEARCH_DIR%"
    exit /b 0
)

for %%I in ("%SEARCH_DIR%\..") do set "PARENT_DIR=%%~fI"
if /I "%PARENT_DIR%"=="%SEARCH_DIR%" exit /b 0
set "SEARCH_DIR=%PARENT_DIR%"
goto :FindProjectRootLoop

:copy_failed
echo [PuerTSTool] Deploy failed while copying files.
exit /b 1
