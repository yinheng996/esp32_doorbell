# ESP32 Doorbell

Small ESP32 firmware that connects to a WPA2-Enterprise network (PEAP/MSCHAPv2) and sends a Telegram message when a doorbell button is pressed.

## Requirements
- VS Code + PlatformIO
- ESP32 DevKit (Arduino framework)
- Telegram bot token + chat ID (see [`docs/telegram-bot-setup.md`](docs/telegram-bot-setup.md))

## Project structure
.
├─ include/
│  ├─ config.h
│  ├─ credentials.example.h
│  └─ credentials.h       
├─ src/
│  └─ main.cpp
├─ docs/
│  └─ telegram-setup.md
├─ README.md
└─ platformio.ini


## Quick start
1. Clone & open this repo in VS Code (PlatformIO installed).
2. Create credentials:
   - Copy `include/credentials.example.h` → `include/credentials.h`.
   - Fill these fields with real values:
     - `WIFI_SSID`, `EAP_IDENTITY`, `EAP_USERNAME`, `EAP_PASSWORD`
     - `TG_BOT_TOKEN`, `TG_CHAT_ID`
     - `DOOR_NAME` (device label shown in alerts)
3. Build & Upload using the PlatformIO toolbar.
4. Serial Monitor @ 115200 baud to view logs.

> Note: `include/credentials.h` is **gitignored** by default.

## Runtime behavior
- On boot: connects to Wi-Fi (PEAP/MSCHAPv2), sets hostname from `DOOR_NAME`, sends a “🔌 \<name\> online” Telegram message on success.
- On button press (falling edge): sends “🔔 \<name\> doorbell pressed”, respecting debounce and cooldown.


## Security notes
- For first tests, TLS uses `setInsecure()`. For production, pin Telegram’s CA or use certificate validation.
- Never commit real tokens/passwords. Rotate the Telegram token immediately if leaked.
