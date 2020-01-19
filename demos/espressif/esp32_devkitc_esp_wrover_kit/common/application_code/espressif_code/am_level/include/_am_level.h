#ifndef __AM_LEVEL_H_
#define __AM_LEVEL_H_

#include <stdbool.h>

static inline bool am_level_is_response_available();

static bool am_level_recv();

static bool am_level_compare(int range, int past_val, int cur_val);

static int am_level_get_lvl_from_buffer(const char * buffer);

static void am_level_report(int level);

#endif