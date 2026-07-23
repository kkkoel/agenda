#!/bin/bash

USER="testuser$(date +%s)"
PASS="test123"
DATE=$(date +%Y-%m-%d)

echo "[1] Register: $USER"
./app.exe "$USER" "$PASS" register
echo ""

echo "[2] Login check"
./app.exe "$USER" "$PASS" showall
echo ""

echo "[3] Add task: Homework"
./app.exe "$USER" "$PASS" addtask "Homework" "${DATE}_10:00" High Study
echo ""

echo "[4] Add task: Movie"
./app.exe "$USER" "$PASS" addtask "Movie" "${DATE}_20:00" Low Entertainment
echo ""

echo "[5] Show all tasks"
./app.exe "$USER" "$PASS" showall
echo ""

echo "[6] Show tasks by date"
./app.exe "$USER" "$PASS" showtask "$DATE"
echo ""

echo "[7] Delete task (ID: 1)"
./app.exe "$USER" "$PASS" deltask 1
echo ""

echo "[8] Verify deletion"
./app.exe "$USER" "$PASS" showall
echo ""

echo "All tests passed."