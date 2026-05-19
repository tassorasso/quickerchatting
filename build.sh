echo building

if [-e "build" ]; then
    rm -r build
fi
if [-e "dist" ]; then
    rm -r dist
fi

python -m PyInstaller --noconsole --onefile --clean \
    --add-binary "backend.dll;." \
    --name "QuickerChatting" \
    chat.py

echo
echo build done
pause