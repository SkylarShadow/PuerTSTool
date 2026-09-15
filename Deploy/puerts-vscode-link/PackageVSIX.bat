@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"

pushd "%SCRIPT_DIR%" || (
    echo Failed to enter extension directory.
    exit /b 1
)

where node >nul 2>nul || (
    echo Node.js was not found in PATH.
    popd
    exit /b 1
)

where npm >nul 2>nul || (
    echo npm was not found in PATH.
    popd
    exit /b 1
)

if not exist "node_modules" (
    call npm install || (
        popd
        exit /b 1
    )
)

for /f "delims=" %%V in ('node -p "const p=require('./package.json'); p.name + '-' + p.version + '.vsix'"') do set "VSIX_NAME=%%V"
set "OUT_FILE=%SCRIPT_DIR%%VSIX_NAME%"
set "VSCE_JS="

if exist "node_modules\.bin\vsce.cmd" (
    set "VSCE_CMD=node_modules\.bin\vsce.cmd"
) else if exist "%APPDATA%\npm\node_modules\vsce\vsce" (
    set "VSCE_JS=%APPDATA%\npm\node_modules\vsce\vsce"
) else if exist "%APPDATA%\npm\node_modules\@vscode\vsce\vsce" (
    set "VSCE_JS=%APPDATA%\npm\node_modules\@vscode\vsce\vsce"
) else (
    where vsce.cmd >nul 2>nul && set "VSCE_CMD=vsce.cmd"
)

if not defined VSCE_CMD if not defined VSCE_JS (
    echo vsce was not found. Install it with: npm install -g @vscode/vsce
    popd
    exit /b 1
)

call npm run compile || (
    popd
    exit /b 1
)

if defined VSCE_CMD (
    call "%VSCE_CMD%" package --out "%OUT_FILE%" || (
        popd
        exit /b 1
    )
) else (
    call node "%VSCE_JS%" package --out "%OUT_FILE%" || (
        popd
        exit /b 1
    )
)

echo.
echo Packaged VSCode extension:
echo %OUT_FILE%

popd
exit /b 0
