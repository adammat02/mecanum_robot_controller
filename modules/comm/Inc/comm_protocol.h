/**
 * @file comm_protocol.h
 * @brief Communication protocol – command parser and response serialiser.
 *
 * Parses single-line ASCII commands into a Command struct. Supported forms:
 *   - "S <s0> <s1> <s2> <s3>"  set speed of 4 motors (float, RPM)
 *   - "P <kp> <ki> <kd>"       set PID gains (float)
 *   - "F <s0> <s1> <s2> <s3>"  full frame: set speeds, respond with full telemetry
 *   - "E"                      request encoder positions
 *   - "R"                      reset encoder counters
 *
 * Response wire format (all lines terminated with '\r'):
 *   - "OK"                                 acknowledge
 *   - "ERR"                                parse or command error
 *   - "E <p0> <p1> <p2> <p3>"             encoder positions [rotations]
 *   - "A <p0> <p1> <p2> <p3> <vbat> <d1> <d2>"  full telemetry
 */

#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/** ASCII tag bytes identifying each host→robot command or robot→host response. */
typedef enum {
  CMD_SET_SPEED     = 'S',  /**< Set motor speeds: "S s0 s1 s2 s3" [RPM] */
  CMD_SET_PID       = 'P',  /**< Set PID gains: "P kp ki kd" */
  CMD_GET_POS       = 'E',  /**< Request encoder positions (no payload) */
  CMD_RESET         = 'R',  /**< Reset all encoder counters (no payload) */
  CMD_FULL_FRAME_RX = 'F',  /**< Set speeds + receive full telemetry response */
  CMD_FULL_FRAME_TX = 'A',  /**< Response tag for full telemetry frame */
  CMD_ERR           = 'X'   /**< Error sentinel; not a valid inbound command */
} CmdType;

/** Parsed command with tag-dependent payload. */
typedef struct
{
  CmdType cmd;
  union
  {
    /** Motor target speeds [RPM]; used by CMD_SET_SPEED and CMD_FULL_FRAME_RX. */
    struct { float speeds[4]; } frame_rx;
    /** PID gains; used by CMD_SET_PID. */
    struct { float kp, ki, kd; } set_pid;
  };
} Command;

/** Robot→host response categories. */
typedef enum {
  RESP_OK,         /**< Generic acknowledgement */
  RESP_ERR,        /**< Error (unknown command or malformed input) */
  RESP_POSITIONS,  /**< Encoder positions: "E p0 p1 p2 p3" */
  RESP_FULL_FRAME, /**< Full telemetry: "A p0 p1 p2 p3 vbat d1 d2" */
} RespType;

/** Response to be serialised and sent to the host. */
typedef struct {
  RespType type;
  union {
    /** Encoder positions [rotations]; used by RESP_POSITIONS. */
    struct { float poses[4]; } positions;
    /** Full telemetry: encoder positions [rot], battery voltage [V],
     *  ToF distances d1 and d2 [m]; used by RESP_FULL_FRAME. */
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

/**
 * @brief Enable or disable debug logging of raw RX/TX strings via printf.
 * @param enabled  true to print "[RX]"/"[TX]" lines; false to silence them.
 */
void comm_set_debug(bool enabled);

/**
 * @brief Pull and parse the next command from the UART line buffer.
 *
 * Returns false immediately if no complete line is waiting. On a parse
 * failure an "ERR\r" response is sent automatically.
 *
 * @param cmd  Destination command, written only on success.
 * @return true if a valid command was received and @p cmd is populated.
 */
bool comm_pull_command(Command *cmd);

/**
 * @brief Serialise @p resp and transmit it over UART.
 * @param resp  Response to send; must have a valid @c type field.
 */
void comm_send_response(const Response *resp);

#endif
