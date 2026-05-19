@echo off
echo building

if exist build rmdir /s /q build
if exist dist rmdir /s /q dist

python -m PyInstaller --noconsole --onefile --clean ^
 --add-binary "backend.dll;." ^
 --add-data "drawing.ico;." ^
 --icon "drawing.ico" ^
 --name "QuickerChatting" ^
 chat.py

echo.
echo build done
pause