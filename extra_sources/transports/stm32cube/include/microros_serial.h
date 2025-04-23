#ifndef MICROROS_SERIAL_H
#define MICROROS_SERIAL_H

#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DMAStream {
  /// UART handle to use for transport.
  UART_HandleTypeDef* uart;

  /// Size of the read buffer, must be a power of 2.
  uint16_t rbuffer_size;

  /// Pointer to read buffer.
  uint8_t* rbuffer;

  /// Size of the transmit buffer, must be a power of 2.
  uint16_t tbuffer_size;

  /// Pointer to transmit buffer.
  uint8_t* tbuffer;
} DMAStream;

void microros_set_serial_transport(DMAStream* stream);

/// This function should be called from HAL_UART_TxCpltCallback. Otherwise the
/// transform won't work properly.
void microros_uart_transfer_complete_callback(DMAStream* stream);

#ifdef __cplusplus
}
#endif

#endif  // MICROROS_SERIAL_H
