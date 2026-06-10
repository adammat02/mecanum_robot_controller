/**
 * @file tof_vl53l1x.h
 * @brief VL53L1X time-of-flight distance sensor driver.
 *
 * Wraps the ST VL53L1X ULD API. Each physical sensor is represented by a
 * @c tof_t handle that must be populated (I2C address, timing budget, XSHUT
 * pin) before calling @c tof_init.
 *
 * Typical single-sensor startup sequence:
 * @code
 *   tof_reset(&tof);
 *   tof_init(&tof);
 * @endcode
 *
 * When multiple sensors share the same I2C bus, bring them up one at a time
 * by keeping the others in shutdown (XSHUT low) and reassigning the I2C
 * address of each before releasing the next:
 * @code
 *   tof_shut_down(&tof2);           // keep tof2 off
 *   tof_reset(&tof1);               // tof1 boots at default address 0x52
 *   tof_init(&tof1);
 *   tof_change_address(&tof1, 0x54);
 *   tof_start_up(&tof2);            // tof2 now boots at 0x52
 *   tof_init(&tof2);
 * @endcode
 */

#ifndef TOF_VL53L1X_H
#define TOF_VL53L1X_H

#include "VL53L1X_api.h"
#include "i2c.h"
#include "gpio.h"

/** VL53L1X sensor handle. Caller must initialise all fields before use. */
typedef struct
{
  VL53L1_Dev_t dev;               /**< Underlying ST device handle (holds I2C bus and address). */
  uint8_t sensor_ready;           /**< Non-zero when a fresh measurement is available. */
  uint16_t timing_budget_ms;      /**< Measurement timing budget [ms]; higher = more accurate. */
  uint32_t inter_measurement_ms;  /**< Period between successive measurements [ms]; must be ≥ timing_budget_ms. */
  GPIO_TypeDef *xshut_port;       /**< GPIO port of the XSHUT pin. */
  uint16_t xshut_pin;             /**< GPIO pin mask of the XSHUT pin. */
  uint16_t distance;              /**< Most recently measured distance [mm]. */
} tof_t;

/**
 * @brief Initialise the sensor and start continuous ranging.
 *
 * Blocks until the sensor reports it has booted, then applies timing_budget_ms,
 * inter_measurement_ms, and starts ranging. The XSHUT pin must already be high.
 *
 * @param tof  Pointer to a fully populated sensor handle.
 */
void tof_init(tof_t *tof);

/**
 * @brief Hardware-reset the sensor by pulsing XSHUT low then high.
 *
 * Use this before @c tof_init to guarantee a clean boot state.
 * The sensor returns to its default I2C address (0x52) after reset.
 *
 * @param tof  Pointer to sensor handle.
 */
void tof_reset(tof_t *tof);

/**
 * @brief Disable the sensor by holding XSHUT low.
 *
 * While shut down the sensor draws minimal current and releases its I2C
 * address, allowing another sensor to be brought up on the same bus.
 *
 * @param tof  Pointer to sensor handle.
 */
void tof_shut_down(tof_t *tof);

/**
 * @brief Release XSHUT to allow the sensor to boot.
 *
 * The sensor will boot at its default I2C address (0x52). Call
 * @c tof_change_address before starting a second sensor on the same bus.
 *
 * @param tof  Pointer to sensor handle.
 */
void tof_start_up(tof_t *tof);

/**
 * @brief Reassign the sensor's I2C address.
 *
 * Updates both the hardware register and the handle's @c dev.dev_addr field.
 * Required when multiple sensors share the same I2C bus.
 *
 * @param tof          Pointer to sensor handle.
 * @param new_address  New 8-bit I2C address (e.g. 0x54).
 */
void tof_change_address(tof_t *tof, uint8_t new_address);

/**
 * @brief Return the most recently measured distance.
 *
 * Polls the data-ready flag. If a new measurement is available it is read,
 * the interrupt is cleared, and the cached @c distance value is updated.
 * Returns the cached value immediately if no new data is ready.
 *
 * @param tof  Pointer to sensor handle.
 * @return Distance in millimetres.
 */
uint16_t tof_get_distance(tof_t *tof);

#endif // TOF_VL53L1X_H
