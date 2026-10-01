@echo off
pushd "%~dp0"
powershell -ExecutionPolicy Bypass -File ".\deploy_2026.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Deploy failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo [SUCCESS] Deploy completed.
pause
popd
