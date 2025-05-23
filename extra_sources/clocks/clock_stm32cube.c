#include <stdint.h>
#include <time.h>
#include <unistd.h>

#if defined(STM32F0xx)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#elif defined(STM32F2xx)
#include "stm32f2xx_hal.h"
#elif defined(STM32F3xx)
#include "stm32f3xx_hal.h"
#elif defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#endif

#define GETTICK_ROLLOVER_USECONDS 4294967296000UL
#define NANOSECONDS_PER_MICROSECOND (1000LL)

int clock_gettime(int unused, struct timespec *tp) {
  (void)unused;
  static uint32_t rollover = 0;
  static uint32_t last_measure = 0;

  __disable_irq();
  uint32_t m = HAL_GetTick();
  rollover += (m < last_measure) ? 1 : 0;
  last_measure = m;
  __enable_irq();

  uint64_t real_us =
      ((uint64_t)m) * 1000UL + rollover * GETTICK_ROLLOVER_USECONDS;
  tp->tv_sec = real_us / 1000000;
  tp->tv_nsec = (real_us % 1000000) * 1000;

  return 0;
}

int gettimeofday(struct timeval *tv, void * __tz) {
  struct timespec tp;
  clock_gettime(0, &tp);

  tv->tv_sec = tp.tv_sec;
  tv->tv_usec = (suseconds_t)(tp.tv_nsec / NANOSECONDS_PER_MICROSECOND);

  return 0;
}
