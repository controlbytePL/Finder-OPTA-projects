#include <ArduinoRS485.h>

#define FINDER_7M_REG_RUN_TIME 103             // Run time
#define FINDER_7M_REG_FREQUENCY 105            // Frequency
#define FINDER_7M_REG_U1 107                   // Voltage U1
#define FINDER_7M_REG_U2 109                   // Voltage U2
#define FINDER_7M_REG_U3 111                   // Voltage U3
#define FINDER_7M_REG_ACTIVE_POWER_TOTAL 140   // Active Power Total (Pt)
#define FINDER_7M_REG_ENERGY_COUNTER_E1 406    // Energy counter E1
#define FINDER_7M_REG_ENERGY_COUNTER_E2 408    // Energy counter E2
#define FINDER_7M_REG_ENERGY_COUNTER_E3 410    // Energy counter E3
#define FINDER_7M_REG_ENERGY_COUNTER_E4 412    // Energy counter E4
#define FINDER_7M_REG_ENERGY_COUNTER_XK_E1 462 // Energy counter E1 x 1000
#define FINDER_7M_REG_ENERGY_COUNTER_XK_E2 464 // Energy counter E2 x 1000
#define FINDER_7M_REG_ENERGY_COUNTER_XK_E3 466 // Energy counter E3 x 1000
#define FINDER_7M_REG_ENERGY_COUNTER_XK_E4 468 // Energy counter E4 x 1000

// Calculate preDelay and postDelay in microseconds as per Modbus RTU.
//
// See MODBUS over serial line specification and implementation guide V1.02
// Paragraph 2.5.1.1 MODBUS Message RTU Framing
// https://modbus.org/docs/Modbus_over_serial_line_V1_02.pdf

constexpr auto MODBUS_BAUDRATE = 19200;
constexpr auto MODBUS_SERIAL_PARAMETERS = SERIAL_8N2;
constexpr auto MODBUS_BIT_DURATION = 1.f / MODBUS_BAUDRATE;
constexpr auto MODBUS_PRE_DELAY = MODBUS_BIT_DURATION * 9.6f * 3.5f * 1e6;
constexpr auto MODBUS_POST_DELAY = MODBUS_BIT_DURATION * 9.6f * 3.5f * 1e6;
