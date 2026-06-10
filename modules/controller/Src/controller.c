#include "controller.h"
#include "ctrl_config.h"
#include <stdio.h>
#include <stdbool.h>
#include "micros.h"
#include "comm_protocol.h"

static RotationDirection_t dirs[N_MOTORS] =
    {ROTATION_CCW, ROTATION_CCW, ROTATION_CCW, ROTATION_CCW};

static float32_t set_speed[N_MOTORS];
static uint32_t last_cmd_us = 0;
static bool debug_enabled = false;

void controller_init(bool debug)
{
  debug_enabled = debug;
  comm_set_debug(debug);
  for (uint8_t i = 0; i < N_MOTORS; i++)
  {
    motor_init(&motors[i]);
    encoder_init(&encoders[i]);
    motor_pid_init(&motor_pids[i]);
  }
  battery_measure_init(&bat);

  tof_shut_down(&tof2);
  tof_reset(&tof1);
  tof_init(&tof1);
  tof_change_address(&tof1, TOF1_ADR);
  tof_start_up(&tof2);
  tof_init(&tof2);
}

void controller_adc_callback(ADC_HandleTypeDef *hadc)
{
  if (hadc == bat.hadc)
  {
    battery_measure_callback(&bat);
  }
}

static void controller_execute(const Command *cmd)
{
  Response resp;
  CmdType type = cmd->cmd;
  switch (type)
  {
  case CMD_GET_POS:
  {
    resp.type = RESP_POSITIONS;
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      resp.positions.poses[i] = encoder_get_rotations(&encoders[i]) * (float)motor_sign[i];
    }
    break;
  }
  case CMD_SET_SPEED:
  {
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      float dir = cmd->frame_rx.speeds[i] * motor_sign[i];
      dirs[i] = (dir >= 0) ? ROTATION_CCW : ROTATION_CW;
      set_speed[i] = fabsf(cmd->frame_rx.speeds[i]);
    }
    resp.type = RESP_OK;
    break;
  }
  case CMD_SET_PID:
  {
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      motor_pid_set_pid(
          &motor_pids[i], cmd->set_pid.kp, cmd->set_pid.ki, cmd->set_pid.kd);
    }
    resp.type = RESP_OK;
    break;
  }
  case CMD_FULL_FRAME_RX:
  {
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      // set motors speed
      float dir = cmd->frame_rx.speeds[i] * motor_sign[i];
      dirs[i] = (dir >= 0) ? ROTATION_CCW : ROTATION_CW;
      set_speed[i] = fabsf(cmd->frame_rx.speeds[i]);

      // read rotations
      resp.full_frame.poses[i] = encoder_get_rotations(&encoders[i]) * (float)motor_sign[i];
    }
    // read battery voltage 
    resp.full_frame.vbat = battery_get_voltage(&bat);

    // read distance from tofs
    resp.full_frame.dist1 = (float)tof_get_distance(&tof1) / 1000.0f;
    resp.full_frame.dist2 = (float)tof_get_distance(&tof2) / 1000.0f;

    resp.type = RESP_FULL_FRAME;
    break;
  }
  case CMD_RESET:
  {
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      encoder_reset(&encoders[i]);
    }
    resp.type = RESP_OK;
    break;
  }
  default:
    resp.type = RESP_ERR;
    break;
  }
  comm_send_response(&resp);
}

void controller_poll(void)
{
  Command cmd;
  if (comm_pull_command(&cmd))
  {
    last_cmd_us = micros();
    controller_execute(&cmd);
  }
}

void controller_update()
{
  if (micros() - last_cmd_us > CMD_TIMEOUT_US)
  {
    for (uint8_t i = 0; i < N_MOTORS; i++)
    {
      set_speed[i] = 0.0;
    }
  }
  for (uint8_t i = 0; i < N_MOTORS; i++)
  {
    motor_pid_update(&motor_pids[i], set_speed[i], dirs[i]);
  }
}