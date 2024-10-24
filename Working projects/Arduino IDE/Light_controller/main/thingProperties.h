#include "OptaBlue.h"
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFiUdp.h>
#include <NTPClient.h>  

#define PERIODIC_UPDATE_TIME 2000

const char ssid[]     = "";    // Network SSID (name)
const char pass[]     = "";    // Network password (use for WPA, or use as key for WEP)
int status  = WL_IDLE_STATUS;

// Ustawienia NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -6*3600, 0);
unsigned long interval = 60*30*1000UL;
unsigned long lastTime = 0;

WiFiConnectionHandler ArduinoIoTPreferredConnection(ssid, pass);

void onSMessageChange();
void onIntensityChange();
void onIPlanChange();
void onSchedulerChange();
void onXLightBedroomStatusChange();

String sMessage;
CloudDimmedLight intensity;
int iPlan;
CloudSchedule scheduler;
bool xLightBedroomStatus;

void initProperties(){

  ArduinoCloud.addProperty(sMessage, READWRITE, ON_CHANGE, onSMessageChange);
  ArduinoCloud.addProperty(intensity, READWRITE, 5 * SECONDS, onIntensityChange);
  ArduinoCloud.addProperty(iPlan, READWRITE, ON_CHANGE, onIPlanChange);
  ArduinoCloud.addProperty(scheduler, READWRITE, ON_CHANGE, onSchedulerChange);
  ArduinoCloud.addProperty(xLightBedroomStatus, READWRITE, ON_CHANGE, onXLightBedroomStatusChange);

}


