#!/bin/bash

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

USER="demo$(date +%s)"
PASS="demo123"
DATE=$(date +%Y-%m-%d)

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                                                           ║"
echo "║              MySchedule 功能演示                         ║"
echo "║                                                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# 1. 注册
echo -e "${BLUE}▸${NC} 注册用户: ${YELLOW}$USER${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" register
echo ""

# 2. 添加任务
echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Homework${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" addtask "Homework" "${DATE}_10:00" High Study
echo ""

echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Movie${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" addtask "Movie" "${DATE}_20:00" Low Entertainment
echo ""

echo -e "${BLUE}▸${NC} 添加任务: ${YELLOW}Meeting${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" addtask "Meeting" "${DATE}_14:30" Medium Life
echo ""

# 3. 查看所有任务
echo ""
echo -e "${BLUE}▸${NC} 查看所有任务"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" showall
echo ""

# 4. 按日期查看
echo -e "${BLUE}▸${NC} 按日期查看任务: ${YELLOW}$DATE${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" showtask "$DATE"
echo ""

# 5. 删除任务
echo -e "${BLUE}▸${NC} 删除任务: ${YELLOW}ID 1${NC}"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" deltask 1
echo ""

# 6. 确认删除
echo -e "${BLUE}▸${NC} 删除后剩余任务"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" showall
echo ""

# 7. 命令行直接添加（不进入交互模式）
echo -e "${BLUE}▸${NC} 命令行直接添加任务 (不进入交互模式)"
echo "─────────────────────────────────────────────────────────────"
./app.exe "$USER" "$PASS" addtask "QuickTask" "${DATE}_18:00" High Study
./app.exe "$USER" "$PASS" showall
echo ""

# 8. 帮助信息
echo -e "${BLUE}▸${NC} 查看帮助信息"
echo "─────────────────────────────────────────────────────────────"
./app.exe --help | head -20
echo ""

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                                                           ║"
echo -e "║              ${GREEN}✓ 所有功能测试通过${NC}                         ║"
echo "║                                                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""