#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "config.h"

const char SSID[] = WIFI_SECRET_SSID;
const char PASS[] = WIFI_SECRET_PASSWORD;

float energy;
float iFreq;
float iPower;
float iVoltage;

void initProperties()
{
  ArduinoCloud.addProperty(energy, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iFreq, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iPower, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iVoltage, READ, ON_CHANGE, NULL);
}

#if ARDUINO_CLOUD_USE_WIFI == 1
    WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
#else
    EthernetConnectionHandler ArduinoIoTPreferredConnection;
#endif
