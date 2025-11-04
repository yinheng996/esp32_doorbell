# ESP32 Smart Doorbell

ESP32 Smart Doorbell is a firmware project for Wi-Fi enabled doorbells that delivers instant Telegram updates when the chime is pressed.

## Key features
- ESP32 Wi-Fi onboarding with configurable hostname.
- Instant Telegram alerts on button presses with HTML formatting.
- Working-hours scheduler that queues off-hours presses and summarizes them on reconnect.
- Offline logging with size-based rotation to preserve event history.
- Debounced button handling with cooldown enforcement to prevent duplicate alerts.

## Requirements
- VS Code with the PlatformIO extension installed.
- A Wi-Fi capable microcontroller supported by PlatformIO (firmware developed and tested on an ESP32 DevKit using the Arduino framework).
- A Telegram bot token and chat ID (follow [`docs/telegram-bot-setup.md`](docs/telegram-bot-setup.md)).

## Quick start
1. Follow [`docs/telegram-bot-setup.md`](docs/telegram-bot-setup.md) to create a Telegram bot and find your chat ID; save both values for later.
2. Fork or clone the project, then open it in VS Code with the PlatformIO extension (free on all platforms).
3. Review `include/config.h` and tune `BTN_PIN`, debounce, or cooldown values so they match your hardware.
4. Wire the button between the configured `BTN_PIN` (set as an input with pull-up) and ground, checking the board pinout as you go.
5. Copy `include/credentials.example.h` to `include/credentials.h` and fill in Wi-Fi details, the Telegram bot token/chat ID, and your preferred `DOOR_NAME`.
6. Copy `include/working_hours.example.h` to `include/working_hours.h`, then adjust the timezone and schedule to suit your space.
7. Use the PlatformIO toolbar to build and upload; no custom scripts required.
8. Open the Serial Monitor at 115200 baud to watch Wi-Fi and Telegram status messages stream by.

> Note: `include/credentials.h` is **gitignored** by default.

## Project architecture
```
esp32_doorbell/
|-- docs/
|   `-- telegram-bot-setup.md (Telegram bot setup notes)
|-- include/
|   |-- config.h               (button pin and debounce configuration)
|   |-- working_hours.example.h (sample timezone/schedule profile)
|   `-- working_hours.h        (active working-hours definition)
|-- src/
|   |-- main.cpp               (Arduino entry point)
|   `-- app/
|       |-- App.{h,cpp}        (glue code for services)
|       |-- BusinessHours.{h,cpp} (working-hours logic)
|       |-- Button.{h,cpp}      (debounced GPIO input)
|       |-- Net.{h,cpp}         (ESP32 Wi-Fi connectivity helper)
|       |-- Notifier.{h,cpp}    (Telegram messaging)
|       |-- OfflineLog.{h,cpp}  (store presses when offline)
|       |-- Scheduler.{h,cpp}   (detect work-hour edges)
|       `-- TimeSvc.{h,cpp}     (NTP + timezone sync)
|-- platformio.ini            (PlatformIO configuration)
`-- README.md                 (project overview)
```

## Runtime behavior
- On power-up the firmware brings up Wi-Fi, syncs time, and posts a `<name> online` heartbeat so you know the doorbell is active.
- Button presses are debounced and rate-limited; during working hours they send an immediate `<name> doorbell pressed` alert, otherwise they are stored for a later summary.
- When the working-hours window opens or closes the app announces `<name> online` or `<name> offline` and delivers any queued events if the network is available.
- Background service loops keep network, time, and button handling responsive without blocking the main loop.

## Security notes
- TLS currently calls `setInsecure()` for quick testing; before production pin Telegram's CA certificate or enable full certificate validation.
- Keep `include/credentials.h` out of version control and rotate Wi-Fi or bot credentials that may have been shared.
- Regenerate the Telegram bot token immediately if you suspect it has leaked.
- Use unique Wi-Fi credentials per device and disable unused SSIDs to limit cross-device exposure.
