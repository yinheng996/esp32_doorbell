#pragma once
#include <time.h>

// POSIX TZ (Singapore = UTC+8 → "SGT-8")
#ifndef SCHED_TZ
#define SCHED_TZ "SGT-8"
#endif

// Business hours (minutes from midnight)
#ifndef MON_THU_START
#define MON_THU_START  (9 * 60)        // 09:00
#endif
#ifndef MON_THU_END
#define MON_THU_END    (18 * 60)       // 18:00
#endif
#ifndef FRI_START
#define FRI_START      (9 * 60)        // 09:00
#endif
#ifndef FRI_END
#define FRI_END        (17 * 60 + 30)  // 17:30
#endif

// Policy before NTP sync (true = allow sends before time is known)
#ifndef SCHED_ALLOW_BEFORE_SYNC
#define SCHED_ALLOW_BEFORE_SYNC true
#endif

// Consider time valid if after 2023-01-01
#ifndef SCHED_VALID_EPOCH
#define SCHED_VALID_EPOCH 1672531200
#endif

// API
void scheduleSetupTime();                            // start NTP (non-blocking)
bool scheduleTimeSynced();                           // epoch sanity
bool scheduleEnsureTime(uint32_t wait_ms = 10000);   // wait up to N ms for NTP
bool schedulePrintCurrentLocalTime();                // print "%a %Y-%m-%d %H:%M:%S %Z"
bool isWithinSchedule();                             // M–Th 09–18, Fri 09–17:30
