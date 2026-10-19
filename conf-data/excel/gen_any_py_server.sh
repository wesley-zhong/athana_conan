#!/bin/bash

# 启用严格错误检查（任何命令失败或未定义变量立即退出）
set -euo pipefail

# 获取脚本绝对路径作为工作目录
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${BASE_DIR}" || exit 1

# 定义路径变量（Windows 反斜杠 → Linux 正斜杠）
FORMULA_EXCEL="${BASE_DIR}/Datas/combat/formulations.xlsx"
FORMUTPATH="${BASE_DIR}/../../FYServer/game-common/src/main/java/com/ares/game/comm/utils"
AutoMathPath="${FORMUTPATH}/FormulationsFunction.java"

if [ ! -f "$AutoMathPath" ]; then
	touch "$AutoMathPath"
fi

# 调用 Python 解析脚本（明确使用 python3）
python3 "${BASE_DIR}/Tools/PythonScript/formulation_auto_math_java.py" \
    "${FORMULA_EXCEL}" "${AutoMathPath}"

# 检查 Python 执行状态
if [ $? -ne 0 ]; then
    echo -e "\033[31m[ERROR] Python 解析失败！请检查：\033[0m"
    echo "1. Excel 文件是否存在: ${FORMULA_EXCEL}"
    echo "2. Python 依赖是否安装: xlrd==2.0.1 pandas==1.1.5 openpyxl==3.1.3"
    exit 1
fi

# 成功提示（绿色高亮）
echo -e "\033[32m[SUCCESS] 公式函数已添加到 ${AutoMathPath}\033[0m"
exit 0