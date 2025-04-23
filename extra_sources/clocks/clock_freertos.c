#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#define MICROSECONDS_PER_SECOND (1000000LL)
#define NANOSECONDS_PER_SECOND (1000000000LL)
#define NANOSECONDS_PER_TICK (NANOSECONDS_PER_SECOND / configTICK_RATE_HZ)
#define NANOSECONDS_PER_MICROSECOND (1000LL)

int clock_gettime(int clock_id, struct timespec *tp) {
  TimeOut_t current_time = {0};

  /* Get the current tick count and overflow count. vTaskSetTimeOutState()
   * is used to get these values because they are both static in tasks.c. */
  vTaskSetTimeOutState(&current_time);

  /* Adjust the tick count for the number of times a TickType_t has overflowed.
   * portMAX_DELAY should be the maximum value of a TickType_t. */
  uint64_t tick_count = (uint64_t)(current_time.xOverflowCount)
                        << (sizeof(TickType_t) * 8);

  /* Add the current tick count. */
  tick_count += current_time.xTimeOnEntering;
  uint64_t nanosecond_count = tick_count * NANOSECONDS_PER_TICK;

  /* Convert to timespec. */
  tp->tv_sec = (time_t)(nanosecond_count / NANOSECONDS_PER_SECOND);
  tp->tv_nsec = (long)(nanosecond_count % NANOSECONDS_PER_SECOND);

  return 0;
}