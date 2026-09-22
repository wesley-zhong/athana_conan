#!/bin/bash
# 查看服务进程状态：PID / CPU / 内存 / 线程数 / 运行时长 / 监听端口
# 用法: ./stat.sh [服务名...]  不带参数默认查看 game gateway
# 不依赖 pidfile，按进程名匹配，手动 nohup 拉起的进程同样能查到

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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

# 按进程名精确匹配 PID；无 pgrep 的环境退回 ps -e -o pid=,comm=（要求 ps 支持 -o）
find_pids() {
    local svc=$1
    if command -v pgrep >/dev/null 2>&1; then
        pgrep -x "$svc"
    else
        ps -e -o pid=,comm= 2>/dev/null | awk -v s="$svc" '$2 == s {print $1}'
    fi
}

# 取某个 pid 的监听端口（去重排序，空格分隔）；优先 ss，老系统退回 netstat
listen_ports() {
    local pid=$1
    if command -v ss >/dev/null 2>&1; then
        ss -ltnpH 2>/dev/null | grep -F "pid=${pid}," \
            | awk '{print $4}' | awk -F: '{print $NF}' | sort -un | tr '\n' ' '
    elif command -v netstat >/dev/null 2>&1; then
        netstat -ltnp 2>/dev/null | grep -F "${pid}/" \
            | awk '{print $4}' | awk -F: '{print $NF}' | sort -un | tr '\n' ' '
    fi
}

printf "%-10s %-8s %-6s %-8s %-8s %-12s %s\n" "SERVICE" "PID" "CPU%" "MEM" "THREADS" "UPTIME" "PORTS"

running=0
for svc in "${SERVICES[@]}"; do
    pids=$(find_pids "$svc" | sort -n)
    if [ -z "$pids" ]; then
        printf "%-10s ${RED}%s${NC}\n" "$svc" "STOPPED"
        continue
    fi
    running=$((running + 1))
    for pid in $pids; do
        # %cpu %mem rss(kB) etime nlwp；进程恰好退出则跳过
        info=$(ps -o %cpu=,%mem=,rss=,etime=,nlwp= -p "$pid" 2>/dev/null) || continue
        [ -z "$info" ] && continue
        read -r cpu mem rss etetime nlwp <<< "$info"
        rss_mb=$((rss / 1024))
        ports=$(listen_ports "$pid")
        printf "%-10s %-8s %-6s %-8s %-8s %-12s %s\n" \
            "$svc" "$pid" "$cpu" "${rss_mb}M" "$nlwp" "$etetime" "$ports"
    done
done

echo "--------------------------------"
if [ "$running" -eq "${#SERVICES[@]}" ]; then
    echo "运行中: ${GREEN}${running}/${#SERVICES[@]}${NC}"
else
    echo "运行中: ${RED}${running}/${#SERVICES[@]}${NC}"
fi
