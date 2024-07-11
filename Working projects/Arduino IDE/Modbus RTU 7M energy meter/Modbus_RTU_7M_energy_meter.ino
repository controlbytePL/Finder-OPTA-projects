#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <WiFi.h>
#include <math.h>
#include "finder-7m.h"
#include "config.h"
#include "thingProperties.h"

const uint8_t MODBUS_7M_ADDRESS = 33;

float power;
float voltage;
float freq;

void setup()
{
    Serial.begin(9600);

    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);
    digitalWrite(LED_D0, HIGH);
    digitalWrite(LED_D1, HIGH);
    digitalWrite(LED_D2, HIGH);
    digitalWrite(LED_D3, HIGH);

    delay(2000);

    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, LOW);
    digitalWrite(LED_D0, LOW);
    digitalWrite(LED_D1, LOW);
    digitalWrite(LED_D2, LOW);
    digitalWrite(LED_D3, LOW);

    Serial.println("Finder Opta + 7M example: setup");

    initProperties();

    RS485.setDelays(MODBUS_PRE_DELAY, MODBUS_POST_DELAY);

    ModbusRTUClient.setTimeout(200);

    if (ModbusRTUClient.begin(MODBUS_BAUDRATE, MODBUS_SERIAL_PARAMETERS))
    {
        Serial.println("Modbus RTU client started");
    }
    else
    {
        Serial.println("Failed to start Modbus RTU client: reset board to restart.");
        while (1)
        {
        }
    }

    setDebugMessageLevel(2);
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, iotConnect);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, iotDisconnect);
    ArduinoCloud.printDebugInfo();
}

const uint32_t INVALID_DATA = 0xFFFFFFFF;

void loop()
{
    uint32_t data;

    Serial.println("** Reading 7M at address " + String(MODBUS_7M_ADDRESS));

    data = modbus_7m_read32(MODBUS_7M_ADDRESS, FINDER_7M_REG_ENERGY_COUNTER_XK_E1);
    Serial.println("   energy = " + (data != INVALID_DATA ? String(data) : String("read error")));

    if (data != INVALID_DATA)
    {
        energy = (float)data;
    }

    data = modbus_7m_read32(MODBUS_7M_ADDRESS, FINDER_7M_REG_RUN_TIME);
    Serial.println("   run time = " + (data != INVALID_DATA ? String(data) : String("read error")));

    data = modbus_7m_read32(MODBUS_7M_ADDRESS, FINDER_7M_REG_FREQUENCY);
    Serial.println("   frequency = " + (data != INVALID_DATA ? String(convert_t5(data)) : String("read error")));

    if (data != INVALID_DATA)
    {
        freq = convert_t5(data);
        iFreq = (float)freq;
    }

    data = modbus_7m_read32(MODBUS_7M_ADDRESS, FINDER_7M_REG_U1);
    Serial.println("   voltage = " + (data != INVALID_DATA ? String(convert_t5(data)) : String("read error")));

    if (data != INVALID_DATA)
    {
        voltage = convert_t5(data);
        iVoltage = (float)voltage;
    }

    data = modbus_7m_read32(MODBUS_7M_ADDRESS, FINDER_7M_REG_ACTIVE_POWER_TOTAL);
    Serial.println("   active power = " + (data != INVALID_DATA ? String(convert_t6(data)) : String("read error")));

    if (data != INVALID_DATA)
    {
        power = convert_t6(data);
        iPower = (float)power;
    }
    for (uint32_t i = 0; i < READ_INTERVAL_SECONDS * 10; i++)
    {
        ArduinoCloud.update();
        delay(100);
    }
}

float convert_t5(uint32_t n)
{
    uint32_t s = (n & 0x80000000) >> 31;
    int32_t e = (n & 0x7F000000) >> 24;
    if (s == 1)
    {
        e = e - 0x80;
    }
    uint32_t m = n & 0x00FFFFFF;
    return (float)m * pow(10, e);
}

float convert_t6(uint32_t n)
{
    uint32_t s = (n & 0x80000000) >> 31;
    int32_t e = (n & 0x7F000000) >> 24;
    if (s == 1)
    {
        e = e - 0x80;
    }
    uint32_t ms = (n & 0x00800000) >> 23;
    int32_t mv = (n & 0x007FFFFF);
    if (ms == 1)
    {
        mv = mv - 0x800000;
    }
    return (float)mv * pow(10, e);
}

uint32_t modbus_7m_read16(uint8_t addr, uint16_t reg)
{
    uint32_t attempts = 3;

    while (attempts > 0)
    {
        digitalWrite(LED_D0, HIGH);

        ModbusRTUClient.requestFrom(addr, INPUT_REGISTERS, reg, 1);
        uint32_t data = ModbusRTUClient.read();

        digitalWrite(LED_D0, LOW);

        if (data != INVALID_DATA)
        {
            return data;
        }
        else
        {
            attempts -= 1;
            delay(10);
        }
    }

    return INVALID_DATA;
}

uint32_t modbus_7m_read32(uint8_t addr, uint16_t reg)
{
    uint8_t attempts = 3;

    while (attempts > 0)
    {
        digitalWrite(LED_D0, HIGH);

        ModbusRTUClient.requestFrom(addr, INPUT_REGISTERS, reg, 2);
        uint32_t data1 = ModbusRTUClient.read();
        uint32_t data2 = ModbusRTUClient.read();

        digitalWrite(LED_D0, LOW);

        if (data1 != INVALID_DATA && data2 != INVALID_DATA)
        {
            return data1 << 16 | data2;
        }
        else
        {
            attempts -= 1;
            delay(10);
        }
    }

    return INVALID_DATA;
}

void iotConnect()
{
    digitalWrite(LEDB, HIGH);
}

void iotDisconnect()
{
    digitalWrite(LEDB, LOW);
}

