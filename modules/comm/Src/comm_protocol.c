#include "comm_protocol.h"
#include "uart_comm.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool s_debug = false;
void comm_set_debug(bool enabled) { s_debug = enabled; }

bool parse_command(const char *line, Command *cmd)
{
  char type;
  if (sscanf(line, " %c", &type) < 1)
    return false;

  switch (type)
  {
  case CMD_SET_SPEED:
  {
    cmd->cmd = CMD_SET_SPEED;
    return sscanf(line, " %*c %f %f %f %f",
                  &cmd->frame_rx.speeds[0], &cmd->frame_rx.speeds[1],
                  &cmd->frame_rx.speeds[2], &cmd->frame_rx.speeds[3]) == 4;
  }
  case CMD_SET_PID:
  {
    cmd->cmd = CMD_SET_PID;
    return sscanf(line, " %*c %f %f %f",
                  &cmd->set_pid.kp, &cmd->set_pid.ki, &cmd->set_pid.kd) == 3;
  }
  case CMD_FULL_FRAME_RX:
  {
    cmd->cmd = CMD_FULL_FRAME_RX;
    return sscanf(line, " %*c %f %f %f %f",
                  &cmd->frame_rx.speeds[0], &cmd->frame_rx.speeds[1],
                  &cmd->frame_rx.speeds[2], &cmd->frame_rx.speeds[3]) == 4;
  }
  case CMD_GET_POS:
  {
    cmd->cmd = CMD_GET_POS;
    return true;
  }
  case CMD_RESET:
  {
    cmd->cmd = CMD_RESET;
    return true;
  }
  default:
    return false;
  }
}

void comm_send_response(const Response *resp)
{
  char out[128];
  switch (resp->type)
  {
  case RESP_POSITIONS:
    sprintf(out, "%c %.3f %.3f %.3f %.3f\r",
            (char)CMD_GET_POS,
            resp->positions.poses[0],
            resp->positions.poses[1],
            resp->positions.poses[2],
            resp->positions.poses[3]);
    break;
  case RESP_FULL_FRAME:
    sprintf(out, "%c %.3f %.3f %.3f %.3f %.3f %.3f %.3f\r",
            (char)CMD_FULL_FRAME_TX,
            resp->full_frame.poses[0],
            resp->full_frame.poses[1],
            resp->full_frame.poses[2],
            resp->full_frame.poses[3],
            resp->full_frame.vbat,
            resp->full_frame.dist1,
            resp->full_frame.dist2);
    break;
  case RESP_OK:
    sprintf(out, "OK\r");
    break;
  default:
    sprintf(out, "ERR\r");
    break;
  }
  if (s_debug)
    printf("[TX] %s\n", out);
  uart_send_str(out);
}

bool comm_pull_command(Command *cmd)
{
  if (!uart_is_line())
  {
    return false;
  }
  char rx_buff[100];
  uart_get_line(rx_buff, sizeof(rx_buff));
  if (s_debug)
    printf("[RX] %s\n", rx_buff);
  bool result = parse_command(rx_buff, cmd);
  if (!result)
    uart_send_str("ERR\r");
  return result;
}
