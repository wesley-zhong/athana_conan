#!/bin/bash
# 停止服务：默认 game gateway，可传参指定：./stop.sh game
# 先发 SIGTERM 让进程走优雅停机（main.cpp 的 handleSignal），
# 超时未退出再 SIGKILL 强杀；与 start.sh/stat.sh 一样按进程名匹配，不依赖 pidfile

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -gt 0 ]; then
    SERVICES=("$@")
else
    SERVICES=(game gateway)
fi

# 等待优雅退出的最长时间，超时升级为 SIGKILL
WAIT_SEC=5

# 仅在终端输出时着色
if [ -t 1 ]; then
    RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; NC=$'\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; NC=''
fi

# 与 stat.sh/start.sh 相同的按进程名找 PID；无 pgrep 的环境退回 ps
find_pids() {
    local svc=$1
    if command -v pgrep >/dev/null 2>&1; then
        pgrep -x "$svc"
    else
        ps -e -o pid=,comm= 2>/dev/null | awk -v s="$svc" '$2 == s {print $1}'
    fi
}

kill_pid() {
    local pid=$1
    kill -TERM "$pid" 2>/dev/null || return 0 # 已退出视为成功
    # 轮询等待优雅退出；kill -0 探活，僵尸进程也算已死
    for _ in $(seq 1 $((WAIT_SEC * 2))); do
        if ! kill -0 "$pid" 2>/dev/null; then
            return 0
        fi
        # 仍在停机流程（排干任务、关句柄）则继续等
        sleep 0.5
    done
    if kill -0 "$pid" 2>/dev/null; then
        echo "${YELLOW}pid=${pid} ${WAIT_SEC}s 未退出，SIGKILL 强杀${NC}"
        kill -KILL "$pid" 2>/dev/null
    fi
}

fail=0
for svc in "${SERVICES[@]}"; do
    pids=$(find_pids "$svc" | sort -n)
    if [ -z "$pids" ]; then
        echo "${GREEN}${svc} 未在运行${NC}"
        continue
    fi
    for pid in $pids; do
        echo "停止 ${svc} pid=${pid} (SIGTERM)..."
        kill_pid "$pid"
        if kill -0 "$pid" 2>/dev/null; then
            echo "${RED}${svc} pid=${pid} 停止失败${NC}"
            fail=$((fail + 1))
        else
            echo "${GREEN}${svc} pid=${pid} 已停止${NC}"
        fi
    done
done

exit $fail
