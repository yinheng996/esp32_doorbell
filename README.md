# ESP32 Smart Doorbell

ESP32 firmware for a Telegram-connected doorbell with door release control.

## Overview
- Instant Telegram alerts when the button is pressed.
- Optional inline Telegram button to trigger door release.
- Working-hours policy for both notifications and release authorization.
- Offline press logging with replay/summary when back online.
- Input debounce and anti-spam cooldown controls.

## Branches
- `main`: Full feature set (doorbell notifications + door unlock flow).
- `notification-only`: Doorbell notification mode only, no door unlock feature.

If you only want doorbell notifications without door unlock, use the `notification-only` branch.

## Hardware and Software Requirements
- ESP32 board supported by PlatformIO (project tested on `esp32doit-devkit-v1`).
- Momentary doorbell button wired to the configured GPIO.
- Relay module (only if using door unlock).
- VS Code + PlatformIO extension.
- Telegram bot token and chat ID.

## Setup
1. Clone the repository and open it in VS Code.
2. Configure Telegram by following `docs/telegram-bot-setup.md`.
3. Create `include/credentials.h` from `include/credentials.example.h`.
4. Create `include/working_hours.h` from `include/working_hours.example.h`.
5. Review `include/config.h` for pin mapping and cooldown settings.
6. Build and upload with PlatformIO.

`include/credentials.h` is gitignored and should never be committed.

## Build and Flash
```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Configuration Reference
- `include/config.h`
  - `BTN_PIN`: doorbell input pin.
  - `DEBOUNCE_MS`: button debounce time.
  - `BTN_EVENT_COOLDOWN_MS`: minimum accepted press interval.
  - `PRESS_NOTIFY_COOLDOWN_MS`: anti-spam window for notifications.
  - `RELAY_PIN`, `RELAY_ACTIVE_LOW`, `RELAY_PULSE_MS`: unlock relay behavior.
- `include/working_hours.h`
  - `WORK_TZ`: timezone.
  - `WORK_HOURS`: allowed time windows.
  - `WORK_ALLOW_BEFORE_SYNC`, `WORK_VALID_EPOCH`: policy before valid NTP time.

## Runtime Behavior
- Boots, connects Wi-Fi, syncs time, mounts local storage, and starts schedule services.
- Sends online/offline status around working-hours transitions.
- During working hours:
  - Button press sends Telegram alert.
  - Door release callback is accepted.
- Outside working hours:
  - Presses are queued.
  - Door release is rejected.

## Project Structure
```text
esp32_doorbell/
  docs/
    telegram-bot-setup.md
  include/
    config.h
    credentials.example.h
    credentials.h
    working_hours.example.h
    working_hours.h
    work_hours_types.h
  lib/
  src/
    main.cpp
    app/
      App.cpp, App.h
      BusinessHours.cpp, BusinessHours.h
      Button.cpp, Button.h
      Net.cpp, Net.h
      Notifier.cpp, Notifier.h
      OfflineLog.cpp, OfflineLog.h
      Relay.cpp, Relay.h
      Scheduler.cpp, Scheduler.h
      TimeSvc.cpp, TimeSvc.h
  platformio.ini
  README.md
```

## Security Notes
- TLS currently uses `setInsecure()` for Telegram requests.
- For production deployments, use proper certificate validation.
- Rotate bot token immediately if exposure is suspected.
