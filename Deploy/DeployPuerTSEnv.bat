@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SOURCE_DIR=%~dp0"
set "TARGET_DIR="
set "RUN_NPM_INSTALL=1"
set "PUERTS_DIR="
set "PAUSE_ON_EXIT=1"
set "EXIT_CODE=0"

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
if /I "%~1"=="--no-pause" (
    set "PAUSE_ON_EXIT=0"
    shift
    goto :ParseArgs
)
if /I "%~1"=="/no-pause" (
    set "PAUSE_ON_EXIT=0"
    shift
    goto :ParseArgs
)
if "%TARGET_DIR%"=="" (
    set "TARGET_DIR=%~1"
)
shift
goto :ParseArgs

:ParseArgsDone

call :Step "0" "Locate UE project root"
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
    goto :failed
)
for %%I in ("%TARGET_DIR%") do set "TARGET_DIR=%%~fI"
if not exist "%TARGET_DIR%" (
    echo [PuerTSTool] Target directory does not exist:
    echo %TARGET_DIR%
    goto :failed
)
if not exist "%TARGET_DIR%\*.uproject" (
    echo [PuerTSTool] No .uproject found in target directory:
    echo %TARGET_DIR%
    goto :failed
)
echo [PuerTSTool] Project root: %TARGET_DIR%
echo.

call :Step "1" "Check Node.js and npm"
call :CheckCommand node "Node.js is required. Install Node.js first, then run this script again." || goto :failed
call :CheckCommand npm "npm is required. Reinstall Node.js or check PATH." || goto :failed
echo [PuerTSTool] Node version:
node -v
echo [PuerTSTool] npm version:
call npm -v
echo.

call :Step "2" "Locate Puerts plugin under project Plugins"
call :ResolvePuertsDir "%TARGET_DIR%" PUERTS_DIR
if "%PUERTS_DIR%"=="" (
    echo [PuerTSTool] Could not find Puerts at:
    echo   %TARGET_DIR%\Plugins\puerts
    echo   %TARGET_DIR%\Plugins\Puerts
    echo.
    set /p "PUERTS_DIR=Input Puerts plugin directory: "
)
if "%PUERTS_DIR%"=="" (
    echo [PuerTSTool] Puerts plugin directory is required for this step.
    goto :failed
)
for %%I in ("%PUERTS_DIR%") do set "PUERTS_DIR=%%~fI"
call :ValidatePuertsDir "%TARGET_DIR%" "%PUERTS_DIR%" || goto :failed
echo [PuerTSTool] Puerts directory: %PUERTS_DIR%
echo [PuerTSTool] Puerts enable script: %PUERTS_DIR%\enable_puerts_module.js
echo [PuerTSTool] Node option: --preserve-symlinks-main
echo.

call :Step "3" "Run node enable_puerts_module.js"
pushd "%PUERTS_DIR%" || goto :failed
node --preserve-symlinks-main "%PUERTS_DIR%\enable_puerts_module.js"
if errorlevel 1 (
    popd
    echo [PuerTSTool] enable_puerts_module.js failed.
    goto :failed
)
popd
echo [PuerTSTool] Puerts module deploy finished.
echo.

call :Step "4" "Verify generated TypeScript project files"
if not exist "%TARGET_DIR%\TypeScript" (
    echo [PuerTSTool] TypeScript directory was not found:
    echo %TARGET_DIR%\TypeScript
    echo.
    echo Run node enable_puerts_module.js successfully before continuing.
    goto :failed
)
if not exist "%TARGET_DIR%\tsconfig.json" (
    echo [PuerTSTool] tsconfig.json was not found in project root:
    echo %TARGET_DIR%\tsconfig.json
    echo.
    echo Run node enable_puerts_module.js successfully before continuing.
    goto :failed
)
echo [PuerTSTool] TypeScript and tsconfig.json found.
echo.

call :Step "5" "Copy TypeScript engineering config files"
copy /Y "%SOURCE_DIR%package.json" "%TARGET_DIR%\package.json" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%eslint.config.mjs" "%TARGET_DIR%\eslint.config.mjs" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.prettierrc" "%TARGET_DIR%\.prettierrc" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.prettierignore" "%TARGET_DIR%\.prettierignore" >nul || goto :copy_failed
copy /Y "%SOURCE_DIR%.editorconfig" "%TARGET_DIR%\.editorconfig" >nul || goto :copy_failed
if not exist "%TARGET_DIR%\package.json" (
    echo [PuerTSTool] package.json was not copied to project root:
    echo %TARGET_DIR%\package.json
    goto :copy_failed
)
echo [PuerTSTool] Config files copied.
echo [PuerTSTool] package.json: %TARGET_DIR%\package.json
echo.

call :Step "6" "Install npm dependencies"
if "%RUN_NPM_INSTALL%"=="1" (
    if not exist "%TARGET_DIR%\package.json" (
        echo [PuerTSTool] package.json is missing before npm install:
        echo %TARGET_DIR%\package.json
        goto :failed
    )
    pushd "%TARGET_DIR%" || goto :failed
    call npm install
    set "NPM_INSTALL_EXIT=!ERRORLEVEL!"
    popd
    if not "!NPM_INSTALL_EXIT!"=="0" (
        echo [PuerTSTool] npm install failed with exit code !NPM_INSTALL_EXIT!.
        echo Check whether Content\JavaScript\PuertsEditor exists when package.json uses local puerts-editor dependency.
        goto :failed
    )
) else (
    echo [PuerTSTool] npm install was skipped by --no-install.
    echo Run this later from project root:
    echo   npm install
)
echo.

call :Step "7" "Finish"
echo [PuerTSTool] Deploy finished.
echo.
echo [PuerTSTool] Next manual steps when needed:
echo   - Compile the UE project, open the editor, and run GenDTS.
echo   - Run npm run type-check to check TypeScript types.
echo   - Run npm run check to run type-check, ESLint, and Prettier checks.
echo   - Run npm run build to compile TypeScript into Content\JavaScript.
echo   - For packaging, include Content\JavaScript in UE packaging settings.
echo [PuerTSTool] This script does not copy .vscode\tasks.json. Copy %SOURCE_DIR%.vscode manually if you want VSCode task buttons.
echo.
set "EXIT_CODE=0"
goto :ExitScript

:failed
set "EXIT_CODE=1"
goto :ExitScript

:Step
echo.
echo [PuerTSTool] Step %~1 - %~2
echo ------------------------------------------------------------
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

:ValidatePuertsDir
set "VALIDATE_PROJECT_ROOT=%~f1"
set "VALIDATE_PUERTS_DIR=%~f2"
for %%I in ("%VALIDATE_PROJECT_ROOT%\Plugins") do set "VALIDATE_PROJECT_PLUGINS=%%~fI"
for %%I in ("%VALIDATE_PUERTS_DIR%\..") do set "VALIDATE_PUERTS_PARENT=%%~fI"

if /I not "%VALIDATE_PUERTS_PARENT%"=="%VALIDATE_PROJECT_PLUGINS%" (
    echo [PuerTSTool] Invalid Puerts directory. It must be directly under the UE project's Plugins directory.
    echo [PuerTSTool] Project Plugins directory:
    echo   %VALIDATE_PROJECT_PLUGINS%
    echo [PuerTSTool] Current Puerts directory:
    echo   %VALIDATE_PUERTS_DIR%
    echo.
    echo Do not use a shared Puerts source directory such as D:\TempPrjs\Plugins\puerts\unreal\Puerts here.
    exit /b 1
)

if not exist "%VALIDATE_PUERTS_DIR%\Puerts.uplugin" (
    echo [PuerTSTool] Puerts.uplugin was not found in:
    echo %VALIDATE_PUERTS_DIR%
    exit /b 1
)

if not exist "%VALIDATE_PUERTS_DIR%\enable_puerts_module.js" (
    echo [PuerTSTool] enable_puerts_module.js was not found in:
    echo %VALIDATE_PUERTS_DIR%
    exit /b 1
)

exit /b 0

:ResolvePuertsDir
set "PROJECT_ROOT=%~f1"
set "%~2="

if not exist "%PROJECT_ROOT%\Plugins" exit /b 0

if exist "%PROJECT_ROOT%\Plugins\puerts\enable_puerts_module.js" (
    set "%~2=%PROJECT_ROOT%\Plugins\puerts"
    exit /b 0
)

if exist "%PROJECT_ROOT%\Plugins\Puerts\enable_puerts_module.js" (
    set "%~2=%PROJECT_ROOT%\Plugins\Puerts"
    exit /b 0
)

exit /b 0

:copy_failed
echo [PuerTSTool] Deploy failed while copying files.
goto :failed

:ExitScript
if "%EXIT_CODE%"=="" set "EXIT_CODE=0"
if "%PAUSE_ON_EXIT%"=="1" (
    echo.
    if "%EXIT_CODE%"=="0" (
        echo [PuerTSTool] Press any key to close this window.
    ) else (
        echo [PuerTSTool] Failed with exit code %EXIT_CODE%. Press any key to close this window.
    )
    pause >nul
)
exit /b %EXIT_CODE%
