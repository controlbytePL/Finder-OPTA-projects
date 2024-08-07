#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

constexpr auto baudrate { 19200 };
constexpr auto bitduration { 1.f / baudrate };
constexpr auto preDelayBR { bitduration * 9.6f * 3.5f * 1e6 };
constexpr auto postDelayBR { bitduration * 9.6f * 3.5f * 1e6 };

struct DataStructure {
  int parameter1;
  int parameter2;
  int parameter3;
  int parameter4;
  int parameter5;
};

DataStructure data;

bool loopData = false;
unsigned long lastBlinkTime = 0;
constexpr unsigned long blinkInterval = 500; // 500 ms interval for blinking
unsigned long lastButtonPress = 0;
unsigned long readInterval = 1000; // Default read interval in milliseconds
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Modbus RTU Server");

  RS485.setDelays(preDelayBR, postDelayBR);

  // Start the Modbus RTU server
  if (!ModbusRTUServer.begin(1, baudrate, SERIAL_8E1)) {
    Serial.println("Failed to start Modbus RTU Server!");
    while (1);
  }

  // Configure holding registers at address 0x00
  ModbusRTUServer.configureHoldingRegisters(0x00, 10);

  pinMode(A0, INPUT_PULLUP); // Button STOP (NC)
  pinMode(A1, INPUT); // Button START
  pinMode(A2, INPUT); // Potentiometer
  pinMode(D0, OUTPUT); // Output 1 (Active)
  pinMode(D1, OUTPUT); // Output 2 (Idle)
  pinMode(LED_RESET, OUTPUT); // LED success
  pinMode(LEDR, OUTPUT); // LED error
}

void loop() {
  // Read potentiometer value and map to interval
  int potValue = analogRead(A2);
  readInterval = map(potValue, 0, 100, 100, 2000);

  // Obsługa przycisku START
  if (digitalRead(A1) == HIGH && millis() - lastButtonPress > 300) { // Debounce delay
    loopData = true;
    digitalWrite(D0, HIGH); // Set D0 high (active)
    digitalWrite(D1, LOW);  // Set D1 low (idle)
    lastButtonPress = millis();
  }

  // Obsługa przycisku STOP (NC)
  if (digitalRead(A0) == LOW && millis() - lastButtonPress > 300) { // Debounce delay
    loopData = false;
    digitalWrite(D0, LOW); // Set D0 low (inactive)
    lastButtonPress = millis();
  }

  if (loopData) {
    if (millis() - lastReadTime >= readInterval) {
      receiveData();
      lastReadTime = millis();
    }
  } else {
    // Blink D1 when idle
    if (millis() - lastBlinkTime >= blinkInterval) {
      digitalWrite(D1, !digitalRead(D1)); // Toggle D1
      lastBlinkTime = millis();
    }
  }

  ModbusRTUServer.poll();
}

void receiveData() {
  bool success = true;
  data.parameter1 = ModbusRTUServer.holdingRegisterRead(0);
  data.parameter2 = ModbusRTUServer.holdingRegisterRead(1);
  data.parameter3 = ModbusRTUServer.holdingRegisterRead(2);
  data.parameter4 = ModbusRTUServer.holdingRegisterRead(3);
  data.parameter5 = ModbusRTUServer.holdingRegisterRead(4);

  if (success) {
    digitalWrite(LED_RESET, HIGH); // Sygnalizacja sukcesu połączenia Modbus
    digitalWrite(LEDR, LOW); // Wyłączenie sygnalizacji błędu
  } else {
    digitalWrite(LED_RESET, LOW); // Wyłączenie sygnalizacji sukcesu
    digitalWrite(LEDR, HIGH); // Sygnalizacja błędu połączenia Modbus
  }

  printData();
}

void printData() {
  Serial.print("Param1: "); Serial.println(data.parameter1);
  Serial.print("Param2: "); Serial.println(data.parameter2);
  Serial.print("Param3: "); Serial.println(data.parameter3);
  Serial.print("Param4: "); Serial.println(data.parameter4);
  Serial.print("Param5: "); Serial.println(data.parameter5);
}
