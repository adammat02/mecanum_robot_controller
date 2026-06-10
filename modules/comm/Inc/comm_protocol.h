/**
 * @file comm_protocol.h
 * @brief Communication protocol – command parser and response serialiser.
 *
 * Parses single-line ASCII commands into a Command struct. Supported forms:
 *   - "S <s0> <s1> <s2> <s3>"  set speed of 4 motors (float, RPM)
 *   - "P <kp> <ki> <kd>"       set PID gains (float)
 *   - "F <s0> <s1> <s2> <s3>"  full frame: set speeds, respond with encoder positions
 *   - "E"                      request encoder positions
 *   - "R"                      reset encoder counters
 */

#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/** Recognised command types. */
typedef enum {
  CMD_SET_SPEED = 'S',
  CMD_SET_PID = 'P',
  CMD_GET_POS = 'E',
  CMD_RESET = 'R',
  CMD_FULL_FRAME_RX = 'F',
  CMD_FULL_FRAME_TX = 'A',
  CMD_ERR = 'X'
} CmdType;

/** Parsed command with tag-dependent payload. */
typedef struct
{
  CmdType cmd;
  union
  {
    struct { float speeds[4]; } frame_rx;
    struct { float kp, ki, kd; } set_pid;   /**< Used when cmd == CMD_SET_PID. */
  };

} Command;

typedef enum {
  RESP_OK,
  RESP_ERR,
  RESP_POSITIONS,   // "E p0 p1 p2 p3"
  RESP_FULL_FRAME,  // "A p0 p1 p2 p3 vbat d1 d2"
} RespType;

typedef struct {
  RespType type;
  union {
    struct { float poses[4]; } positions;
    struct { float poses[4]; float vbat; float dist1; float dist2; } full_frame;
  };
} Response;


/**
 * @brief Parse a single line into a Command.
 *
 * Leading whitespace is skipped. The line must contain all fields required
 * by the command type; otherwise parsing fails and @p cmd is left untouched.
 *
 * @param line  Null-terminated input line.
 * @param cmd   Destination command, written only on success.
 * @return true on successful parse, false on malformed or unknown input.
 */
bool parse_command(const char *line, Command *cmd);

void comm_set_debug(bool enabled);

bool comm_pull_command(Command *cmd);

void comm_send_response(const Response *resp);

#endif
