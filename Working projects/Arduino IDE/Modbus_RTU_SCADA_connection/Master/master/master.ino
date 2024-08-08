#include <ArduinoModbus.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library

struct DataStructure {
  int jobID;
  char partname[24];
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
  generateRandomData();

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

  delay(100); // Czekanie 1 sekundy
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

void generateRandomData() {
  data.jobID = random(1, 100);
  snprintf(data.partname, sizeof(data.partname), "Part%d", data.jobID);
  data.deformdiameter = random(10, 200) / 10.0;
  data.correctionvalue = random(0, 100) / 100.0;
  data.deformpressure = random(0, 200) / 10.0;
  data.dieset = random(0, 200) / 10.0;
  data.maxdiameter = random(10, 300) / 10.0;
  data.mindiameter = random(10, 300) / 10.0;
  data.maxpressure = random(10, 200) / 10.0;
  data.minpressure = random(10, 200) / 10.0;
  data.opendiameter = random(10, 400) / 10.0;
  data.pressureswitch = random(10, 400) / 10.0;
  data.diameterswitch = random(10, 300) / 10.0;
  data.holdingtime = random(0, 100) / 100.0;
  data.diameterunit = random(0, 2);
  data.pressureunit = random(0, 2);
  data.deformtopressure = random(0, 2);
  snprintf(data.nextpartname, sizeof(data.nextpartname), "Part%d", random(1, 100));
  data.batchsize = random(1, 1000);
  data.batchcount = random(1, 100);
  data.diameter = random(10, 300) / 10.0;
  data.pressure = random(10, 50) / 10.0;
  snprintf(data.now, sizeof(data.now), "%02d/%02d/%04d %02d:%02d:%02d", random(1, 32), random(1, 13), random(2000, 2030), random(0, 24), random(0, 60), random(0, 60));
}