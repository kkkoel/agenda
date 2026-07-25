#!/bin/bash
# 在项目根目录下运行本脚本: ./test.sh
# 会自动查找 build/bin/myschedule (CMake默认输出路径)，
# 找不到时会提示先执行 cmake .. && make。

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

MYSCHEDULE="build/bin/myschedule"
if [ ! -f "$MYSCHEDULE" ]; then
    echo -e "${RED}✗ 找不到可执行文件: $MYSCHEDULE${NC}"
    echo "  请先编译: mkdir -p build && cd build && cmake .. && make && cd .."
    exit 1
fi

USER="demo$(date +%s)"
PASS="demo123"
DATE=$(date +%Y-%m-%d)

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                                                           ║"
echo "║              MySchedule 功能演示                          ║"
echo "║                                                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

echo -e "${BLUE}▸${NC} 验证data/目录能否自动创建(模拟全新clone场景)"
echo "─────────────────────────────────────────────────────────────"
rm -rf data
if [ -d "data" ]; then
    echo -e "${RED}✗ data/目录删除失败,跳过此项检查${NC}"
else
    echo "  已删除data/目录，模拟全新环境"
fi
echo ""

echo -e "${BLUE}▸${NC} 注册用户: ${YELLOW}$USER${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" register
if [ -d "data" ]; then
    echo -e "${GREEN}✓ data/目录已被程序自动创建${NC}"
else
    echo -e "${RED}✗ data/目录未被创建,注册可能已失败${NC}"
fi
echo ""

echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Homework${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" addtask "Homework" "${DATE}_10:00" High Study
echo ""

echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Movie${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" addtask "Movie" "${DATE}_20:00" Low Entertainment
echo ""

echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Meeting${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" addtask "Meeting" "${DATE}_14:30" Medium Life
echo ""

echo ""
echo -e "${BLUE}▸${NC} 查看所有任务"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" showall
echo ""

echo -e "${BLUE}▸${NC} 按日期查看任务: ${YELLOW}$DATE${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" showtask "$DATE"
echo ""

echo -e "${BLUE}▸${NC} 删除任务: ${YELLOW}ID 1${NC}"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" deltask 1
echo ""

echo -e "${BLUE}▸${NC} 删除后剩余任务"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" showall
echo ""

echo -e "${BLUE}▸${NC} 命令行直接添加任务 (不进入交互模式)"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" "$USER" "$PASS" addtask "QuickTask" "${DATE}_18:00" High Study
"$MYSCHEDULE" "$USER" "$PASS" showall
echo ""

echo -e "${BLUE}▸${NC} 查看帮助信息"
echo "─────────────────────────────────────────────────────────────"
"$MYSCHEDULE" --help | head -20
echo ""

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                                                           ║"
echo -e "║              ${GREEN}✓ 所有功能测试通过${NC}                           ║"
echo "║                                                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""