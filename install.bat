@echo off
setlocal EnableDelayedExpansion

echo =========================================
echo  Szip Installation (Windows)
echo =========================================

REM 1. Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Error: 'cmake' could not be found. Please install CMake first.
    exit /b 1
)

REM 2. Set install directory
set "INSTALL_DIR=%USERPROFILE%\szip"
set "BIN_DIR=%INSTALL_DIR%\bin"

echo [1/3] Building Szip...
if not exist build mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ..
cmake --build . --config Release -j 4
if %errorlevel% neq 0 (
    echo [!] Build failed.
    exit /b 1
)

echo.
echo [2/3] Installing Szip to %INSTALL_DIR%...
cmake --install . --config Release
if %errorlevel% neq 0 (
    echo [!] Installation failed.
    exit /b 1
)

echo.
echo [3/3] Adding %BIN_DIR% to User PATH Environment Variable...

REM Use PowerShell to safely append to User PATH without truncating 1024 char limits of setx
powershell -NoProfile -ExecutionPolicy Bypass -Command "$newPath = '%BIN_DIR%'; $oldPath = [Environment]::GetEnvironmentVariable('PATH', 'User'); if ($oldPath -inotmatch [regex]::Escape($newPath)) { $finalPath = $oldPath; if (-not $finalPath.EndsWith(';')) { $finalPath += ';' }; $finalPath += $newPath; [Environment]::SetEnvironmentVariable('PATH', $finalPath, 'User'); Write-Host 'Successfully added to PATH.' } else { Write-Host 'Directory is already in PATH.' }"

echo.
echo =========================================
echo [x] Installation Complete!
echo Please restart your terminal (or open a new CMD/PowerShell window)
echo so the new PATH variable takes effect.
echo Then try running: szip -h
echo =========================================
pause
