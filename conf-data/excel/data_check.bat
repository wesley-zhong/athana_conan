@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

rem 获取已安装的 .NET 运行时版本
for /f "tokens=1,2 delims= " %%A in ('dotnet --list-runtimes') do (
    rem 查找 Microsoft.NETCore.App 并提取版本号
    if "%%A"=="Microsoft.NETCore.App" (
        set "version=%%B"
        rem 提取主版本号和次版本号
        for /f "tokens=1,2 delims=." %%C in ("!version!") do (
            set "major=%%C"
            set "minor=%%D"
            
            rem 检查是否是 8.0 或以上的版本
            if !major! geq 8 (
                echo 找到.NET 版本 !major!.!minor!, 是 8.0 或以上版本,允许执行
                goto :continue
            )
        )
    )
)

echo 本地没有找到 .NET 8.0 以上版本，请安装后重试！！！
exit

:continue
rem 在这里继续运行你的脚本

cd /d %~dp0%
set CLIENT_PROJECT_DATA=..\..\Data
set PROJECT_SRC_CPP=..\..\Source\FY
set LUBAN_DLL=.\Tools\Luban\Luban.dll
set CONF_ROOT=.
set PROJECT_CONTENT_BIN=..\..\Content\FY\Data\bin
call common_rw.bat
cd /d %~dp0%
dotnet %LUBAN_DLL% ^
    -t client ^
    -c ue-bin ^
    -d bin  ^
    --conf %CONF_ROOT%\luban.conf ^
    -x outputCodeDir=%PROJECT_SRC_CPP%\luban\Gen ^
    -x outputDataDir=%CLIENT_PROJECT_DATA%\bin
ROBOCOPY %CLIENT_PROJECT_DATA%\bin %PROJECT_CONTENT_BIN%  /E /MT:12 /compress

pause