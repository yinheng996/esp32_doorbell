#pragma once
#include <Arduino.h>
#include "work_hours_types.h"

// --- Local policy (copy this to working_hours.h and edit) ---

// POSIX TZ for your site (Singapore UTC+8)
#ifndef WORK_TZ
#define WORK_TZ "SGT-8"
#endif

// Before first NTP sync, should we behave as if it's within hours?
#ifndef WORK_ALLOW_BEFORE_SYNC
#define WORK_ALLOW_BEFORE_SYNC true
#endif

// Consider time "valid" if epoch >= 2023-01-01
#ifndef WORK_VALID_EPOCH
#define WORK_VALID_EPOCH 1672531200UL
#endif

// Example: Mon–Fri 09:00–18:00, weekend closed
constexpr DayWindows WORK_HOURS[7] = {
  /* Sun */ {0, {{0,0},{0,0},{0,0},{0,0}}},
  /* Mon */ {1, {{ 9*60, 18*60 }, {0,0},{0,0},{0,0}}},
  /* Tue */ {1, {{ 9*60, 18*60 }, {0,0},{0,0},{0,0}}},
  /* Wed */ {1, {{ 9*60, 18*60 }, {0,0},{0,0},{0,0}}},
  /* Thu */ {1, {{ 9*60, 18*60 }, {0,0},{0,0},{0,0}}},
  /* Fri */ {1, {{ 9*60, 18*60 }, {0,0},{0,0},{0,0}}},
  /* Sat */ {0, {{0,0},{0,0},{0,0},{0,0}}},
};
