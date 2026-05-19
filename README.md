# QuickerChatting

A lightweight P2P chat app.

![the banner](banner.png)

---

## How to join a party

1. Enter a display name
2. Enter the host's IP address
3. Enter the port
4. (Optional) Enter a password, both sides must use the same one
5. Press **Join**

## How to host a party

1. Enter a display name
2. Enter the port you want to listen on
3. (Optional) Enter a password, both sides must use the same one
4. Press **Host**
5. Share your public IP and port with whoever is joining

## Keybind

Press **Insert** to bring the chat window into focus. You can change this in the Settings

---

## Compile and build

1. Run `compile.bat` to build `backend.dll` (requires Mingw g++)
2. Run `build.bat` to package everything into a single executable
3. Find the executable in the `dist/` folder

> Tested with Gcc 14.2.0 (MinGW-W64 x86_64-ucrt-posix-seh) Linux/macOS users can use `compile.sh` for the dll step.
>
> Or download a prebuilt release if you trust me which makes it way simpler.

---

## Requirements

**Python 3.11+**

| Library | Source |
|---|---|
| `PySide6` | `pip install PySide6` |
| `keyboard` | `pip install keyboard` |
| `cryptography` | `pip install cryptography` |

Standard library modules (`sys`, `ctypes`, `os`, `uuid`, `base64`, `html`) require no installation.

---

## Networking note

The default ip shown is `127.0.0.1`. For friends to connect over the internet you need to:

1. Find your public IP at [whatismyipaddress.com](https://whatismyipaddress.com/)
2. Port forward the chosen port on your router ([tutorial](https://www.youtube.com/watch?v=NTLDsEuQlYc))
3. Give your friend your public ip and port

People on the same wifi can use your local ip directly.

---

## Security

Messages are encrypted with AES-128 when a shared password is set. The header shows **ENC** or **OPEN** so you always know the connection state. The host accepts a maximum of 20 clients.