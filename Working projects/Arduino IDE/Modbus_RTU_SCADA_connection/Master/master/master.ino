#include <ArduinoModbus.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library

struct DataStructure {
  int jobID;
  char partname[20];
  float deformdiameter;
  float correctionvalue;
  float deformpressure;
  float dieset;
  float maxdiameter;
  float mindiameter;
  float maxpressure;
  float minpressure;
  float opendiameter;
  float pressureswitch;
  float diameterswitch;
  float holdingtime;
  bool diameterunit;
  bool pressureunit;
  bool deformtopressure;
  char nextpartname[20];
  int batchsize;
  int batchcount;
  float diameter;
  float pressure;
  char now[20];
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
  data.jobID = 25;
  strcpy(data.partname, "28.91 VW 87665456");
  data.deformdiameter = 17.7;
  data.correctionvalue = 0.48;
  data.deformpressure = 10.99;
  data.dieset = 15.97;
  data.maxdiameter = 28.03;
  data.mindiameter = 18.29;
  data.maxpressure = 19.94;
  data.minpressure = 11.21;
  data.opendiameter = 31.02;
  data.pressureswitch = 29.58;
  data.diameterswitch = 22.68;
  data.holdingtime = 0.19;
  data.diameterunit = true;
  data.pressureunit = false;
  data.deformtopressure = false;
  strcpy(data.nextpartname, "clamp");
  data.batchsize = 868;
  data.batchcount = 57;
  data.diameter = 24.33651;
  data.pressure = 18.975712;
  strcpy(data.now, "06/21/2024 10:15:58 AM");

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
  success &= ModbusRTUClient.holdingRegisterWrite(1, 0, data.jobID);
  success &= writeStringToModbus(1, 1, data.partname, 10);
  success &= writeFloatToModbus(1, 11, data.deformdiameter);
  success &= writeFloatToModbus(1, 13, data.correctionvalue);
  success &= writeFloatToModbus(1, 15, data.deformpressure);
  success &= writeFloatToModbus(1, 17, data.dieset);
  success &= writeFloatToModbus(1, 19, data.maxdiameter);
  success &= writeFloatToModbus(1, 21, data.mindiameter);
  success &= writeFloatToModbus(1, 23, data.maxpressure);
  success &= writeFloatToModbus(1, 25, data.minpressure);
  success &= writeFloatToModbus(1, 27, data.opendiameter);
  success &= writeFloatToModbus(1, 29, data.pressureswitch);
  success &= writeFloatToModbus(1, 31, data.diameterswitch);
  success &= writeFloatToModbus(1, 33, data.holdingtime);
  success &= ModbusRTUClient.holdingRegisterWrite(1, 35, data.diameterunit ? 1 : 0);
  success &= ModbusRTUClient.holdingRegisterWrite(1, 36, data.pressureunit ? 1 : 0);
  success &= ModbusRTUClient.holdingRegisterWrite(1, 37, data.deformtopressure ? 1 : 0);
  success &= writeStringToModbus(1, 38, data.nextpartname, 10);
  success &= ModbusRTUClient.holdingRegisterWrite(1, 48, data.batchsize);
  success &= ModbusRTUClient.holdingRegisterWrite(1, 49, data.batchcount);
  success &= writeFloatToModbus(1, 50, data.diameter);
  success &= writeFloatToModbus(1, 52, data.pressure);
  success &= writeStringToModbus(1, 54, data.now, 10);

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

bool writeStringToModbus(uint8_t serverAddress, uint16_t startAddress, const char* str, size_t length) {
  bool success = true;
  for (size_t i = 0; i < length / 2; i++) {
    uint16_t combined = str[i * 2] | (str[i * 2 + 1] << 8);
    success &= ModbusRTUClient.holdingRegisterWrite(serverAddress, startAddress + i, combined);
  }
  return success;
}

bool writeFloatToModbus(uint8_t serverAddress, uint16_t startAddress, float value) {
  uint32_t n;
  memcpy(&n, &value, sizeof(n));
  uint16_t high = (n >> 16) & 0xFFFF;
  uint16_t low = n & 0xFFFF;
  bool success = ModbusRTUClient.holdingRegisterWrite(serverAddress, startAddress, high);
  success &= ModbusRTUClient.holdingRegisterWrite(serverAddress, startAddress + 1, low);
  return success;
}
