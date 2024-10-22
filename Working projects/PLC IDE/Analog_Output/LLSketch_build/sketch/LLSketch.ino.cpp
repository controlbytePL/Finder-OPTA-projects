#include <Arduino.h>
#line 1 "C:\\Users\\k0go\\Desktop\\Praca\\ControlByte\\Projekty\\Finder OPTA\\PLC  IDE\\Analog Output\\LLSketch\\LLSketch.ino"
#include <AlPlc_Opta.h>

/* opta_1.2.0
      - Arduino_ConnectionHandler (1.0.0)
      - ArduinoECCX08 (1.3.8)
      - ArduinoMqttClient (0.1.8)
      - Arduino_DebugUtils (1.4.0)
      - Arduino_Portenta_OTA (1.2.1)
      - Arduino_SecureElement (0.1.2)
      - ArduinoIoTCloud (2.1.0)
      - ArduinoHttpClient (0.6.1)
      - NTPClient (3.2.1)
*/

struct PLCSharedVarsInput_t
{
	int16_t in_iPlan;
	float in_brightness;
	bool in_switchStatus;
	bool in_scheduleActive;
	int16_t in_startDate;
	int16_t in_endDate;
	int16_t in_length;
	uint64_t in_currentTime;
	bool in_xGreenLed;
	bool in_xRedLed;
};
PLCSharedVarsInput_t& PLCIn = (PLCSharedVarsInput_t&)m_PLCSharedVarsInputBuf;

struct PLCSharedVarsOutput_t
{
	bool out_xLightStatus;
};
PLCSharedVarsOutput_t& PLCOut = (PLCSharedVarsOutput_t&)m_PLCSharedVarsOutputBuf;


AlPlc AxelPLC(1120069359);

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFiUdp.h>
#include <NTPClient.h>   // Biblioteka NTPClient do pobierania czasu z serwera NTP

// Konfiguracja WiFi
const char SSID[]     = "Internet ADAM";    
const char PASS[]     = "MartynaSzymon2003!";

// Ustawienia NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 60000); // Serwer NTP (UTC+2)

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void onIntensityChange();
void onIPlanChange();
void onSchedulerChange();
void onXLightBedroomStatusChange();

CloudDimmedLight intensity;
int iPlan;
CloudSchedule scheduler;
bool xLightBedroomStatus;

#line 64 "C:\\Users\\k0go\\Desktop\\Praca\\ControlByte\\Projekty\\Finder OPTA\\PLC  IDE\\Analog Output\\LLSketch\\LLSketch.ino"
void initProperties();
#line 71 "C:\\Users\\k0go\\Desktop\\Praca\\ControlByte\\Projekty\\Finder OPTA\\PLC  IDE\\Analog Output\\LLSketch\\LLSketch.ino"
void setup();
#line 90 "C:\\Users\\k0go\\Desktop\\Praca\\ControlByte\\Projekty\\Finder OPTA\\PLC  IDE\\Analog Output\\LLSketch\\LLSketch.ino"
void loop();
#line 64 "C:\\Users\\k0go\\Desktop\\Praca\\ControlByte\\Projekty\\Finder OPTA\\PLC  IDE\\Analog Output\\LLSketch\\LLSketch.ino"
void initProperties(){
  ArduinoCloud.addProperty(intensity, READWRITE, 5 * SECONDS, onIntensityChange);
  ArduinoCloud.addProperty(iPlan, READWRITE, ON_CHANGE, onIPlanChange);
  ArduinoCloud.addProperty(scheduler, READWRITE, ON_CHANGE, onSchedulerChange);
  ArduinoCloud.addProperty(xLightBedroomStatus, READWRITE, ON_CHANGE, onXLightBedroomStatusChange);
}

void setup() {
  // Inicjalizacja komunikacji szeregowej
  Serial.begin(9600);
  delay(1500);
  
  PLCIn.in_xRedLed = true;

  // Po��czenie z Arduino Cloud i NTP
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  timeClient.begin();

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

	AxelPLC.Run();
}

void loop() {
  ArduinoCloud.update();
  PLCIn.in_xRedLed = false;
  PLCIn.in_xGreenLed = true;
  
  timeClient.update();  // Aktualizacja czasu z NTP
  
  // Pobieranie aktualnego czasu z NTP
  unsigned long currentEpochTime = timeClient.getEpochTime();
  Serial.println(String(currentEpochTime));
  
  //Przekazywanie czasu do programu PLC
  PLCIn.in_currentTime = currentEpochTime;
  
  //Pobieranie wybranego planu na dashboardzie
  PLCIn.in_iPlan = iPlan;
  
  // Pobieranie jasno�ci i statusu w��cznika z CloudDimmedLight
  PLCIn.in_brightness = intensity.getBrightness();
  PLCIn.in_switchStatus = intensity.getSwitch();
  
  // Sprawdzanie, czy harmonogram jest aktywny
  PLCIn.in_scheduleActive = scheduler.isActive();

  // Pobieranie startu i ko�ca harmonogramu
  PLCIn.in_startDate = scheduler.getCloudValue().frm;   // Start harmonogramu
  PLCIn.in_endDate = scheduler.getCloudValue().to;      // Koniec harmonogramu
  PLCIn.in_length = scheduler.getCloudValue().len;      // D�ugo�� harmonogramu
  
  //Aktualizowanie statusu o�wietlenia
  xLightBedroomStatus = PLCOut.out_xLightStatus;
  
  // Aktualny czas w formacie EPOCH (sekundy od 1970-01-01)
  //Serial.print("Aktualny czas (Unix Timestamp): ");
  //Serial.println(currentEpochTime);

}
/*
  Obs�uga zmian jasno�ci (intensity) z Arduino Cloud.
*/
void onIntensityChange() {
  // Tutaj mo�na doda� logik� reaguj�c� na zmiany jasno�ci
}

/*
  Obs�uga zmian harmonogramu (scheduler) z Arduino Cloud.
*/
void onSchedulerChange() {
  // Tutaj mo�na doda� logik� reaguj�c� na zmiany harmonogramu
}

/*
  Obs�uga zmian statusu �wiat�a w sypialni.
*/
void onXLightBedroomStatusChange() {
  // Tutaj mo�na doda� logik� reaguj�c� na zmiany statusu �wiat�a w sypialni
}

/*
  Obs�uga zmian planu (iPlan) z Arduino Cloud.
*/
void onIPlanChange() {
  // Tutaj mo�na doda� logik� reaguj�c� na zmiany planu o�wietlenia
}

