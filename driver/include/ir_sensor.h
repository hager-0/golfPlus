#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IrSensor IrSensor;

/**
 * Open IR sensor on a given gpiochip + line.
 * @param chip_name     e.g. "gpiochip0"
 * @param gpio_line     e.g. 17
 * @param active_low    1 => raw 0 means detected, 0 => raw 1 means detected
 * @param debounce_ms   ignore edges within this time window (e.g. 50)
 * @return pointer on success, NULL on failure
 */
IrSensor* ir_sensor_open(const char* chip_name,
                         unsigned int gpio_line,
                         int active_low,
                         int debounce_ms);

/** Close and free resources. */
void ir_sensor_close(IrSensor* s);

/**
 * Wait for a stable state change (debounced).
 * @param s             sensor handle
 * @param detected_out  logical state: true = detected, false = no object
 * @param raw_out       optional: raw GPIO value (0/1). pass NULL if not needed
 * @return true on success, false on error
 */
bool ir_sensor_wait_change(IrSensor* s, bool* detected_out, int* raw_out);

#ifdef __cplusplus
}
#endif
