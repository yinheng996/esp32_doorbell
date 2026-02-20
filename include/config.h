#pragma once

#define BTN_PIN     27

#define DEBOUNCE_MS 40UL
#define BTN_EVENT_COOLDOWN_MS 150UL       // minimum interval between accepted press events
#define PRESS_NOTIFY_COOLDOWN_MS 3000UL   // ignore repeated notify/log actions for 3 seconds

#define RELAY_PIN   33

#define RELAY_ACTIVE_LOW true
#define RELAY_PULSE_MS 3000UL
