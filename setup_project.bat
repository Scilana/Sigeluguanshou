@echo off
echo ==========================================
echo       正在为 Sigeluguanshou 生成工程...
echo ==========================================

:: 1. 检查是否安装了 CMake (简单检查)
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 你的电脑好像没装 CMake，或者没配环境变量！
    pause
    exit /b
)

:: 2. 运行 CMake 生成命令
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32

:: 3. 检查是否成功
if %errorlevel% neq 0 (
    echo.
    echo [失败] 生成出错了！请截图发给组长。
    pause
    exit /b
)

echo.
echo ==========================================
echo [成功] 工程已生成！
echo 请进入 build 文件夹打开 Sigeluguanshou.sln
echo ==========================================
pause