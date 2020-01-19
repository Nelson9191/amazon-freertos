#ifndef __AM_ULTRASONIC_H_
#define __AM_ULTRASONIC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

static bool _am_ultrasonic_measure(int * distance_cm);

static inline bool _am_ultrasonic_time_expired(int64_t start_time, int64_t ping_timeout);


static void _am_ultrasonic_task(void * pvParameters);

static void _am_ultrasonic_report(int level);

static bool _am_ultrasonic_compare(int range, int past_val, int cur_val);

#endif