#include "tof_vl53l1x.h"

void tof_init(tof_t *tof)
{
  uint8_t boot_state = 0;
  while (!boot_state)
    VL53L1X_BootState(&tof->dev, &boot_state);

  VL53L1X_SensorInit(&tof->dev);
  VL53L1X_SetTimingBudgetInMs(&tof->dev, tof->timing_budget_ms);
  VL53L1X_SetInterMeasurementInMs(&tof->dev, tof->inter_measurement_ms);
  VL53L1X_StartRanging(&tof->dev);
}

void tof_reset(tof_t *tof)
{
  HAL_GPIO_WritePin(tof->xshut_port, tof->xshut_pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(tof->xshut_port, tof->xshut_pin, GPIO_PIN_SET);
  HAL_Delay(10);
}

void tof_shut_down(tof_t *tof)
{
  HAL_GPIO_WritePin(tof->xshut_port, tof->xshut_pin, GPIO_PIN_RESET);
  HAL_Delay(10);
}

void tof_start_up(tof_t *tof)
{
  HAL_GPIO_WritePin(tof->xshut_port, tof->xshut_pin, GPIO_PIN_SET);
  HAL_Delay(10);
}

void tof_change_address(tof_t *tof, uint8_t new_address)
{
  VL53L1X_SetI2CAddress(&tof->dev, new_address);
  tof->dev.dev_addr = new_address;
}

uint16_t tof_get_distance(tof_t *tof)
{
  VL53L1X_CheckForDataReady(&tof->dev, &tof->sensor_ready);
  if (tof->sensor_ready)
  {
    VL53L1X_GetDistance(&tof->dev, &tof->distance);
    VL53L1X_ClearInterrupt(&tof->dev);
  }
  return tof->distance;
}
