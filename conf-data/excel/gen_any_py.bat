@echo off
setlocal enabledelayedexpansion

:: 执行日志
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"

set "FORMULA_EXCEL=.\Datas\combat\formulations.xlsx"
set "AutoMath_CPP_h=..\..\Source\FY\luban\Gen\AutoMath.h"
set "FORMUTPATH=..\..\Source\FY\luban\"
set "GenLocalizationData=.\Datas\Localization\Test\"
set "AudioPath=.\Datas\audio\"

:: 递归移除文件下所有只读权限
attrib -R "%FORMUTPATH%\*" /S /D >nul 2>&1

:: python .\Tools\PythonScript\formulation_auto_math_ue.py "%FORMULA_EXCEL%" "%AutoMath_CPP_h%"
if exist ".\Tools\PythonScript\formulation_auto_math_ue.exe" (
	echo %ESC%[33m##开始执行 =========== 异常属性计算 公式库cpp生成 =============%ESC%[0m
    .\Tools\PythonScript\formulation_auto_math_ue.exe "%FORMULA_EXCEL%" "%AutoMath_CPP_h%"
	echo %ESC%[33m##执行结束 =========== 异常属性计算 公式库cpp生成 =============%ESC%[0m
) else (
    echo 未找到formulation_auto_math_ue.exe
    exit /b 0
)

if %errorlevel% neq 0 (
    echo Python解析失败!
    exit /b 0
)
echo 公式函数已成功添加到 %AutoMath_CPP_h%

:: python .\Tools\PythonScript\TransExcelBeCpp.exe"
if exist ".\Tools\PythonScript\TransExcelBeCpp.exe" (
	echo %ESC%[33m##开始执行 =========== scene.xlsx 转换 Cpp 代码 脚本 ============%ESC%[0m
    .\Tools\PythonScript\TransExcelBeCpp.exe
	echo %ESC%[33m##执行结束 =========== scene.xlsx 转换 Cpp 代码 脚本=====================%ESC%[0m
) else (
    echo 未找到TransExcelBeCpp.exe
    exit /b 0
)

if exist ".\Tools\PythonScript\GenLocalizationCheckAKEvent.exe" (
	echo %ESC%[33m## 开始执行 ========音频组 %GenLocalizationData% 台词文件汇成总表脚本 ==========%ESC%[0m
	.\Tools\PythonScript\GenLocalizationCheckAKEvent.exe %GenLocalizationData% %AudioPath%
	echo %ESC%[33m## 执行结束 ========音频组FYGame\Data\excel\Datas\Localization\Test 台词文件汇成总表脚本 ==========%ESC%[0m
) else (
    echo GenLocalizationCheckAKEvent.exe
    exit /b 0
)

endlocal
exit /b 0