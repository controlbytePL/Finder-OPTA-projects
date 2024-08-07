#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include "thingProperties.h"

constexpr auto baudrate { 19200 };
constexpr auto bitduration { 1.f / baudrate };
constexpr auto preDelayBR { bitduration * 9.6f * 3.5f * 1e6 };
constexpr auto postDelayBR { bitduration * 9.6f * 3.5f * 1e6 };

WiFiServer wifiServer(502);
WiFiClient client;


ModbusTCPServer modbusTCPServer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // Adjust the time offset as needed

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

bool loopData = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 500; // 500 ms interval for blinking
unsigned long lastButtonPress = 0;
const unsigned long readInterval = 1000; // Default read interval in milliseconds
unsigned long lastReadTime = 0;

void handleModbusTCP(const DataStructure& data) {
  modbusTCPServer.holdingRegisterWrite(0, data.jobID);
  writeStringToModbusTCP(1, data.partname);
  writeFloatToModbusTCP(11, data.deformdiameter);
  writeFloatToModbusTCP(13, data.correctionvalue);
  writeFloatToModbusTCP(15, data.deformpressure);
  writeFloatToModbusTCP(17, data.dieset);
  writeFloatToModbusTCP(19, data.maxdiameter);
  writeFloatToModbusTCP(21, data.mindiameter);
  writeFloatToModbusTCP(23, data.maxpressure);
  writeFloatToModbusTCP(25, data.minpressure);
  writeFloatToModbusTCP(27, data.opendiameter);
  writeFloatToModbusTCP(29, data.pressureswitch);
  writeFloatToModbusTCP(31, data.diameterswitch);
  writeFloatToModbusTCP(33, data.holdingtime);
  modbusTCPServer.holdingRegisterWrite(35, data.diameterunit ? 1 : 0);
  modbusTCPServer.holdingRegisterWrite(36, data.pressureunit ? 1 : 0);
  modbusTCPServer.holdingRegisterWrite(37, data.deformtopressure ? 1 : 0);
  writeStringToModbusTCP(38, data.nextpartname);
  modbusTCPServer.holdingRegisterWrite(48, data.batchsize);
  modbusTCPServer.holdingRegisterWrite(49, data.batchcount);
  writeFloatToModbusTCP(50, data.diameter);
  writeFloatToModbusTCP(52, data.pressure);
  writeStringToModbusTCP(54, data.now);
}

bool writeStringToModbusTCP(uint16_t startAddress, const char* str) {
  bool success = true;
  for (size_t i = 0; i < strlen(str); i += 2) {
    uint16_t combined = str[i] | (str[i + 1] << 8);
    success &= modbusTCPServer.holdingRegisterWrite(startAddress + i / 2, combined);
  }
  return success;
}

bool writeFloatToModbusTCP(uint16_t startAddress, float value) {
  uint32_t n;
  memcpy(&n, &value, sizeof(n));
  uint16_t high = (n >> 16) & 0xFFFF;
  uint16_t low = n & 0xFFFF;
  bool success = modbusTCPServer.holdingRegisterWrite(startAddress, high);
  success &= modbusTCPServer.holdingRegisterWrite(startAddress + 1, low);
  return success;
}


void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Modbus RTU Server");

  // Connect to WiFi
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");


  // Initialize NTPClient
  timeClient.begin();
  timeClient.update();

  RS485.setDelays(preDelayBR, postDelayBR);

  // Start the Modbus RTU server
  if (!ModbusRTUServer.begin(1, baudrate, SERIAL_8E1)) {
    Serial.println("Failed to start Modbus RTU Server!");
    while (1);
  }

  // Configure holding registers at address 0x00
  ModbusRTUServer.configureHoldingRegisters(0x00, 60); // Ensure enough registers are allocated

    // Initialize Modbus TCP Server
  if (!modbusTCPServer.begin()) {  // 1502 is the default port for Modbus TCP
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }
  Serial.println("Modbus TCP Server started");

  wifiServer.begin();

  // Configure holding registers for Modbus TCP
  modbusTCPServer.configureHoldingRegisters(0x00, 60);

  // Display IP address and port information
  Serial.print("Modbus TCP Server IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Modbus TCP Server Port: ");
  Serial.println(502);

  pinMode(A0, INPUT_PULLUP); // Button STOP (NC)
  pinMode(A1, INPUT); // Button START
  pinMode(A2, INPUT); // Potentiometer
  pinMode(D0, OUTPUT); // Output 1 (Active)
  pinMode(D1, OUTPUT); // Output 2 (Idle)
  pinMode(LED_RESET, OUTPUT); // LED success
  pinMode(LEDR, OUTPUT); // LED error
}

void loop() {
  // Read potentiometer value and map to blink interval
  int potValue = analogRead(A2);
  blinkInterval = map(potValue, 0, 100, 200, 3000);

  // Handle START button
  if (digitalRead(A1) == HIGH && millis() - lastButtonPress > 300) { // Debounce delay
    loopData = true;
    digitalWrite(D0, HIGH); // Set D0 high (active)
    digitalWrite(D1, LOW);  // Set D1 low (idle)
    lastButtonPress = millis();
  }

  // Handle STOP button (NC)
  if (digitalRead(A0) == LOW && millis() - lastButtonPress > 300) { // Debounce delay
    loopData = false;
    digitalWrite(D0, LOW); // Set D0 low (inactive)
    lastButtonPress = millis();
  }

  if (loopData && millis() - lastReadTime >= readInterval) {
    receiveData();
    lastReadTime = millis();
    handleModbusTCP(data);
  }

  if (!loopData) {
    // Blink D1 when idle
    if (millis() - lastBlinkTime >= blinkInterval) {
      digitalWrite(D1, !digitalRead(D1)); // Toggle D1
      lastBlinkTime = millis();
    }
  }

  ModbusRTUServer.poll();

  // Check for new clients
  if (!client || !client.connected()) {
    client = wifiServer.available();
    if (client) {
      // a new client connected
      Serial.println("New client connected");
      modbusTCPServer.accept(client);
    }
  }

  // Handle Modbus TCP requests if client is connected
  if (client && client.connected()) {
    modbusTCPServer.poll();
  }
}


void receiveData() {
  bool success = true;
  data.jobID = ModbusRTUServer.holdingRegisterRead(0);
  success &= readStringFromModbus(1, 1, data.partname, 10);
  data.deformdiameter = modbusToFloat(ModbusRTUServer.holdingRegisterRead(11), ModbusRTUServer.holdingRegisterRead(12));
  data.correctionvalue = modbusToFloat(ModbusRTUServer.holdingRegisterRead(13), ModbusRTUServer.holdingRegisterRead(14));
  data.deformpressure = modbusToFloat(ModbusRTUServer.holdingRegisterRead(15), ModbusRTUServer.holdingRegisterRead(16));
  data.dieset = modbusToFloat(ModbusRTUServer.holdingRegisterRead(17), ModbusRTUServer.holdingRegisterRead(18));
  data.maxdiameter = modbusToFloat(ModbusRTUServer.holdingRegisterRead(19), ModbusRTUServer.holdingRegisterRead(20));
  data.mindiameter = modbusToFloat(ModbusRTUServer.holdingRegisterRead(21), ModbusRTUServer.holdingRegisterRead(22));
  data.maxpressure = modbusToFloat(ModbusRTUServer.holdingRegisterRead(23), ModbusRTUServer.holdingRegisterRead(24));
  data.minpressure = modbusToFloat(ModbusRTUServer.holdingRegisterRead(25), ModbusRTUServer.holdingRegisterRead(26));
  data.opendiameter = modbusToFloat(ModbusRTUServer.holdingRegisterRead(27), ModbusRTUServer.holdingRegisterRead(28));
  data.pressureswitch = modbusToFloat(ModbusRTUServer.holdingRegisterRead(29), ModbusRTUServer.holdingRegisterRead(30));
  data.diameterswitch = modbusToFloat(ModbusRTUServer.holdingRegisterRead(31), ModbusRTUServer.holdingRegisterRead(32));
  data.holdingtime = modbusToFloat(ModbusRTUServer.holdingRegisterRead(33), ModbusRTUServer.holdingRegisterRead(34));
  data.diameterunit = ModbusRTUServer.holdingRegisterRead(35);
  data.pressureunit = ModbusRTUServer.holdingRegisterRead(36);
  data.deformtopressure = ModbusRTUServer.holdingRegisterRead(37);
  success &= readStringFromModbus(1, 38, data.nextpartname, 10);
  data.batchsize = ModbusRTUServer.holdingRegisterRead(48);
  data.batchcount = ModbusRTUServer.holdingRegisterRead(49);
  data.diameter = modbusToFloat(ModbusRTUServer.holdingRegisterRead(50), ModbusRTUServer.holdingRegisterRead(51));
  data.pressure = modbusToFloat(ModbusRTUServer.holdingRegisterRead(52), ModbusRTUServer.holdingRegisterRead(53));
  success &= readStringFromModbus(1, 54, data.now, 10);

  // Get actual date and time from NTP
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm * ptm = gmtime(&rawTime);
  snprintf(data.now, sizeof(data.now), "%02d-%02d %02d:%02d:%02d", ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  if (success) {
    digitalWrite(LED_RESET, HIGH); // Indicate successful Modbus connection
    digitalWrite(LEDR, LOW); // Turn off error indicator
  } else {
    digitalWrite(LED_RESET, LOW); // Turn off success indicator
    digitalWrite(LEDR, HIGH); // Indicate Modbus connection error
  }

  if (loopData) {
    printData();
  }
}

void printData() {
  Serial.print("JobID: "); Serial.println(data.jobID);
  Serial.print("Part Name: "); Serial.println(data.partname);
  Serial.print("Deform Diameter: "); Serial.println(data.deformdiameter);
  Serial.print("Correction Value: "); Serial.println(data.correctionvalue);
  Serial.print("Deform Pressure: "); Serial.println(data.deformpressure);
  Serial.print("Dieset: "); Serial.println(data.dieset);
  Serial.print("Max Diameter: "); Serial.println(data.maxdiameter);
  Serial.print("Min Diameter: "); Serial.println(data.mindiameter);
  Serial.print("Max Pressure: "); Serial.println(data.maxpressure);
  Serial.print("Min Pressure: "); Serial.println(data.minpressure);
  Serial.print("Open Diameter: "); Serial.println(data.opendiameter);
  Serial.print("Pressure Switch: "); Serial.println(data.pressureswitch);
  Serial.print("Diameter Switch: "); Serial.println(data.diameterswitch);
  Serial.print("Holding Time: "); Serial.println(data.holdingtime);
  Serial.print("Diameter Unit: "); Serial.println(data.diameterunit);
  Serial.print("Pressure Unit: "); Serial.println(data.pressureunit);
  Serial.print("Deform to Pressure: "); Serial.println(data.deformtopressure);
  Serial.print("Next Part Name: "); Serial.println(data.nextpartname);
  Serial.print("Batch Size: "); Serial.println(data.batchsize);
  Serial.print("Batch Count: "); Serial.println(data.batchcount);
  Serial.print("Diameter: "); Serial.println(data.diameter);
  Serial.print("Pressure: "); Serial.println(data.pressure);
  Serial.print("Now: "); Serial.println(data.now);
}

float modbusToFloat(uint16_t high, uint16_t low) {
  uint32_t combined = ((uint32_t)high << 16) | low;
  float result;
  memcpy(&result, &combined, sizeof(result));
  return result;
}

bool readStringFromModbus(uint8_t serverAddress, uint16_t startAddress, char* str, size_t length) {
  bool success = true;
  for (size_t i = 0; i < (length + 1) / 2; i++) {
    uint16_t combined = ModbusRTUServer.holdingRegisterRead(startAddress + i);
    str[i * 2] = combined & 0xFF;
    str[i * 2 + 1] = (combined >> 8) & 0xFF;
  }
  str[length] = '\0';
  return success;
}
