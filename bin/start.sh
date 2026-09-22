#!/bin/bash
# nohup 后台启动服务，默认 game gateway，可传参指定：./start.sh game
# 工作目录固定为工程根目录：程序按相对路径读 config/*.toml、写 ../logs/*.log
# stat.sh 按进程名匹配，不依赖 pidfile，本脚本不额外记录 pid

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ $# -gt 0 ]; then
    SERVICES=("$@")
else
    SERVICES=(game gateway)
fi

# 仅在终端输出时着色
if [ -t 1 ]; then
    RED=$'\033[31m'; GREEN=$'\033[32m'; NC=$'\033[0m'
else
    RED=''; GREEN=''; NC=''
fi

# 与 stat.sh 相同的按进程名找 PID；无 pgrep 的环境退回 ps
find_pids() {
    local svc=$1
    if command -v pgrep >/dev/null 2>&1; then
        pgrep -x "$svc"
    else
        ps -e -o pid=,comm= 2>/dev/null | awk -v s="$svc" '$2 == s {print $1}'
    fi
}

# 定位二进制：Linux 单配置生成器直接输出到 bin/linux，多配置生成器在 Debug 子目录
find_bin() {
    local svc=$1
    if [ -x "${PROJECT_ROOT}/bin/linux/${svc}" ]; then
        echo "${PROJECT_ROOT}/bin/linux/${svc}"
    elif [ -x "${PROJECT_ROOT}/bin/linux/Debug/${svc}" ]; then
        echo "${PROJECT_ROOT}/bin/linux/Debug/${svc}"
    fi
}

cd "${PROJECT_ROOT}" || exit 1
# spdlog 文件 sink 目录不存在会抛异常直接终止，先兜底创建
mkdir -p ../logs

fail=0
for svc in "${SERVICES[@]}"; do
    pids=$(find_pids "$svc")
    if [ -n "$pids" ]; then
        echo "${GREEN}${svc} 已在运行 pid=${pids//$'\n'/, }${NC}，跳过"
        continue
    fi
    bin=$(find_bin "$svc")
    if [ -z "$bin" ]; then
        echo "${RED}找不到 ${svc} 二进制（bin/linux/${svc}），请先编译${NC}"
        fail=$((fail + 1))
        continue
    fi
    # stdout/stderr 收进 nohup 日志，保留启动早期（配置失败等）的报错现场
    nohup "$bin" >> "../logs/${svc}.nohup.log" 2>&1 &
    echo "${GREEN}已启动 ${svc} pid=$!${NC} -> ${bin}"
done

# 给进程一点时间暴露启动失败（配置缺失、端口占用等），再统一核对状态
sleep 1
echo
"$SCRIPT_DIR/stat.sh" "${SERVICES[@]}"
exit $fail
