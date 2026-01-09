@echo off
setlocal enabledelayedexpansion

if not exist "bin" mkdir bin

echo [i] compiling

g++ -O2 src/main.cpp src/globals.cpp src/utils.cpp src/ui.cpp src/network.cpp ^
    -Isrc ^
    -lws2_32 -lcomctl32 -lshell32 -lurlmon -lgdi32 -lcomdlg32 -lole32 ^
    -mwindows ^
    -o bin/chat.exe

if %errorlevel% equ 0 (
    echo [!] done
) else (
    echo [X] error code %errorlevel%
)

pause