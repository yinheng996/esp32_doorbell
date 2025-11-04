#pragma once
#include <stdint.h>

struct WorkWindow { uint16_t start_min; uint16_t end_min; }; // [start, end)
struct DayWindows { uint8_t count; WorkWindow windows[4]; };
