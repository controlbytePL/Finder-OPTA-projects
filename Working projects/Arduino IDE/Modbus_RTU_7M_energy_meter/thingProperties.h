#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "config.h"

void onHeaterChange();
void onVentilatorChange();
void onLampChange();

const char SSID[] = WIFI_SECRET_SSID;
const char PASS[] = WIFI_SECRET_PASSWORD;

float energy;
float iFreq;
float iPower;
float iVoltage; 
float iCurrent;
bool xHeater_button;
bool xVentilator_button;
bool xLamp_button;

void initProperties()
{
  ArduinoCloud.addProperty(energy, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iFreq, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iPower, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iVoltage, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(iCurrent, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(xHeater_button, READWRITE, ON_CHANGE, onHeaterChange);
  ArduinoCloud.addProperty(xVentilator_button, READWRITE, ON_CHANGE, onVentilatorChange);
  ArduinoCloud.addProperty(xLamp_button, READWRITE, ON_CHANGE, onLampChange);
}

#if ARDUINO_CLOUD_USE_WIFI == 1
    WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
#else
    EthernetConnectionHandler ArduinoIoTPreferredConnection;
#endif





