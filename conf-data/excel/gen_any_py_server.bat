@echo off
setlocal enabledelayedexpansion

set "FORMULA_EXCEL=.\Datas\combat\formulations.xlsx"
set "FORMUTPATH=..\..\..\FYServer\game-common\src\main\java\com\ares\game\comm\utils"
set "AutoMathPath=..\..\..\FYServer\game-common\src\main\java\com\ares\game\comm\utils\FormulationsFunction.java"

:: 递归移除文件下所有只读权限
attrib -R "%FORMUTPATH%\*" /S /D >nul 2>&1

:: 调用Python解析脚本
python .\Tools\PythonScript\formulation_auto_math_java.py "%FORMULA_EXCEL%" "%AutoMathPath%"

if %errorlevel% neq 0 (
    echo Python解析失败!
    exit /b 1
)

echo 公式函数已成功添加到 %AutoMathPath%
endlocal
exit /b 0