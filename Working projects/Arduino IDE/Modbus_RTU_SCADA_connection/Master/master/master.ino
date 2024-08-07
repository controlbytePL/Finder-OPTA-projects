#include <ArduinoModbus.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library

struct DataStructure {
  int parameter1;
  int parameter2;
  int parameter3;
  int parameter4;
  int parameter5;
};

DataStructure data;

constexpr auto baudrate { 19200 };
constexpr auto bitduration { 1.f / baudrate };
constexpr auto preDelayBR { bitduration * 9.6f * 3.5f * 1e6 };
constexpr auto postDelayBR { bitduration * 9.6f * 3.5f * 1e6 };

void setup() {
    Serial.begin(9600);
    while (!Serial);

    Serial.println("Modbus RTU Client");

    RS485.setDelays(preDelayBR, postDelayBR);

    // Start the Modbus RTU client
    if (!ModbusRTUClient.begin(baudrate, SERIAL_8E1)) {
        Serial.println("Failed to start Modbus RTU Client!");
        while (1);
    }

    pinMode(LED_BUILTIN, OUTPUT); // LED USER
    pinMode(LED_D0, OUTPUT); // LED 1
    pinMode(LED_D1, OUTPUT); // LED 2
    pinMode(LED_D2, OUTPUT); // LED 3
    pinMode(LED_D3, OUTPUT); // LED 4
    pinMode(LED_RESET, OUTPUT); // LED success
    pinMode(LEDR, OUTPUT); // LED error

    // Włączenie LED USER
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    // Losowe generowanie danych co 1 sekundę
    data.parameter1 = random(0, 100);
    data.parameter2 = random(0, 100);
    data.parameter3 = random(0, 100);
    data.parameter4 = random(0, 100);
    data.parameter5 = random(0, 100);

    // Sygnalizacja wygenerowania danych
    digitalWrite(LED_D0, HIGH);
    delay(100);
    digitalWrite(LED_D0, LOW);

    // Przepływ LED (fala)
    digitalWrite(LED_D1, HIGH);
    delay(100);
    digitalWrite(LED_D1, LOW);
    digitalWrite(LED_D2, HIGH);
    delay(100);
    digitalWrite(LED_D2, LOW);
    digitalWrite(LED_D3, HIGH);
    delay(100);
    digitalWrite(LED_D3, LOW);

    // Wysyłanie danych przez Modbus RTU
    bool success = true;
    success &= ModbusRTUClient.holdingRegisterWrite(1, 0, data.parameter1);
    success &= ModbusRTUClient.holdingRegisterWrite(1, 1, data.parameter2);
    success &= ModbusRTUClient.holdingRegisterWrite(1, 2, data.parameter3);
    success &= ModbusRTUClient.holdingRegisterWrite(1, 3, data.parameter4);
    success &= ModbusRTUClient.holdingRegisterWrite(1, 4, data.parameter5);

    if (success) {
        digitalWrite(LED_RESET, HIGH); // Sygnalizacja sukcesu
        digitalWrite(LEDR, LOW); // Wyłączenie sygnalizacji błędu
        Serial.println("Successfully wrote registers");
    } else {
        digitalWrite(LED_RESET, LOW); // Wyłączenie sygnalizacji sukcesu
        digitalWrite(LEDR, HIGH); // Sygnalizacja błędu
        Serial.print("Failed to write registers! ");
        Serial.println(ModbusRTUClient.lastError());
    }

    delay(1000); // Czekanie 1 sekundy
}
