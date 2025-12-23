@echo off
echo ================================================
echo 矿洞资源检查脚本
echo ================================================
echo.

echo [1] 检查源文件中的 TMX 地图...
if exist "Resources\map\Mines\1.tmx" (
    echo     ✓ 1.tmx 存在
    findstr /C:"source=\"mine.png\"" "Resources\map\Mines\1.tmx" >nul
    if %ERRORLEVEL% EQU 0 (
        echo     ✓ 1.tmx tileset 引用正确 ^(mine.png^)
    ) else (
        echo     ✗ 1.tmx tileset 引用错误 ^(缺少 .png 扩展名^)
    )
) else (
    echo     ✗ 1.tmx 不存在
)
echo.

echo [2] 检查源文件中的图片资源...
if exist "Resources\map\Mines\mine.png" (
    echo     ✓ mine.png 存在
) else (
    echo     ✗ mine.png 不存在
)
echo.

echo [3] 检查构建输出目录...
set BUILD_DIR=build\bin\Sigeluguanshou\Debug

if exist "%BUILD_DIR%\map\Mines\1.tmx" (
    echo     ✓ 构建目录中存在 1.tmx
    findstr /C:"source=\"mine.png\"" "%BUILD_DIR%\map\Mines\1.tmx" >nul
    if %ERRORLEVEL% EQU 0 (
        echo     ✓ 构建目录中的 1.tmx tileset 引用正确
    ) else (
        echo     ✗ 构建目录中的 1.tmx tileset 引用错误
        echo     ! 需要重新构建项目
    )
) else (
    echo     ✗ 构建目录中不存在 1.tmx
    echo     ! 需要构建项目
)
echo.

if exist "%BUILD_DIR%\map\Mines\mine.png" (
    echo     ✓ 构建目录中存在 mine.png
) else (
    echo     ✗ 构建目录中不存在 mine.png
    echo     ! 需要构建项目
)
echo.

echo [4] 检查 1.tmx 中的 Front 层数据...
findstr /C:"name=\"Front\"" "Resources\map\Mines\1.tmx" >nul
if %ERRORLEVEL% EQU 0 (
    echo     ✓ Front 层定义存在

    REM 提取 Front 层数据并检查是否有非零值
    echo     检查 Front 层是否包含装饰物数据...

    REM 这里简单检查是否有常见的装饰 GID (78, 112, 128, 224)
    findstr /C:",78," "Resources\map\Mines\1.tmx" >nul
    if %ERRORLEVEL% EQU 0 (
        echo     ✓ 发现 GID 78 ^(装饰物^)
    )

    findstr /C:",112," "Resources\map\Mines\1.tmx" >nul
    if %ERRORLEVEL% EQU 0 (
        echo     ✓ 发现 GID 112 ^(装饰物^)
    )
) else (
    echo     ✗ Front 层定义不存在
)
echo.

echo [5] 统计地图文件数量...
set COUNT=0
for %%f in (Resources\map\Mines\*.tmx) do set /a COUNT+=1
echo     找到 %COUNT% 个 TMX 地图文件
echo.

echo ================================================
echo 检查完成！
echo ================================================
echo.
echo 如果所有检查都通过 ^(✓^)，说明资源配置正确。
echo 如果有错误 ^(✗^) 或警告 ^(^^!^)，请按照提示操作。
echo.
echo 如果构建目录中的文件不正确，请运行：
echo     cmake --build build --config Debug
echo.
pause
