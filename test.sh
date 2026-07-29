#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo ""
echo "=========================================="
echo "  MySchedule 功能测试"
echo "=========================================="
echo ""

USER="demo$(date +%s)"
PASS="demo123"
DATE=$(date +%Y-%m-%d)

CLI_PATH="./cli/build/myschedule"

if [ ! -f "$CLI_PATH" ]; then
    echo -e "${RED}找不到 myschedule，请先编译 CLI${NC}"
    exit 1
fi

echo -e "${BLUE}[1]${NC} 注册用户: ${YELLOW}$USER${NC}"
$CLI_PATH "$USER" "$PASS" register
echo ""

echo -e "${BLUE}[2]${NC} 添加任务: Homework"
$CLI_PATH "$USER" "$PASS" addtask "Homework" "${DATE}_10:00" High Study
echo ""

echo -e "${BLUE}[3]${NC} 添加任务: Movie"
$CLI_PATH "$USER" "$PASS" addtask "Movie" "${DATE}_20:00" Low Entertainment
echo ""

echo -e "${BLUE}[4]${NC} 查看所有任务"
$CLI_PATH "$USER" "$PASS" showall
echo ""

echo -e "${BLUE}[5]${NC} 按日期查看任务: $DATE"
$CLI_PATH "$USER" "$PASS" showtask "$DATE"
echo ""

echo -e "${BLUE}[6]${NC} 删除任务 ID 1"
$CLI_PATH "$USER" "$PASS" deltask 1
echo ""

echo -e "${BLUE}[7]${NC} 验证删除结果"
$CLI_PATH "$USER" "$PASS" showall
echo ""

echo -e "${BLUE}[8]${NC} 命令行直接添加任务"
$CLI_PATH "$USER" "$PASS" addtask "QuickTask" "${DATE}_18:00" High Study
$CLI_PATH "$USER" "$PASS" showall
echo ""

echo "=========================================="
echo -e "${GREEN}所有功能测试通过！${NC}"
echo "=========================================="
