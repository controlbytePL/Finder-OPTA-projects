#include "thingProperties.h"

bool lightSwitch;  // Zmienna sterująca włączeniem i wyłączeniem światła z dashboardu
float brightness;
int selectedPlan;  // 1: Default, 2: Mood, 3: Night, 4: Energy Saving, 5: Manual
bool scheduleActive;
unsigned long startDate;
unsigned long endDate;
unsigned long length;

unsigned long lastDacUpdate = 0;  // Zmienna przechowująca czas ostatniej aktualizacji DAC
const unsigned long updateInterval = 1000;  // Interwał aktualizacji DAC (w milisekundach, np. 1000 = 1 sekunda)

void setup() {
  Serial.begin(115200);
  delay(2000);

  OptaController.begin();

  // Konfiguracja kanałów analogowych jako DAC na każdym urządzeniu
  for (int device = 0; device < OptaController.getExpansionNum(); device++) {
    for (int ch = 0; ch < OA_AN_CHANNELS_NUM; ch++) {
      AnalogExpansion::beginChannelAsDac(OptaController,
                                         device,
                                         ch,
                                         OA_VOLTAGE_DAC,
                                         true,
                                         false,
                                         OA_SLEW_RATE_0);
    }
  }

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  while (ArduinoCloud.connected() == 0) {
    ArduinoCloud.update();
    Serial.println("- Waiting for connection to Arduino Cloud");
    delay(1000);
  }

  timeClient.begin();
  updateTime();
}

void loop() {
  ArduinoCloud.update();
  getVariables();
  handleSerialCommands();
  optaAnalogTask();  // Zadanie odpowiedzialne za sterowanie światłem
}

/* -------------------------------------------------------------------------- */
/*                   Funkcja pobierająca zmienne z chmury                     */
/* -------------------------------------------------------------------------- */
void getVariables() {
  lightSwitch = intensity.getSwitch();  // Sprawdza status przycisku ON/OFF na dashboard
  selectedPlan = iPlan;
  scheduleActive = scheduler.isActive();
  startDate = scheduler.getCloudValue().frm;
  endDate = scheduler.getCloudValue().to;
  length = scheduler.getCloudValue().len;
  brightness = intensity.getBrightness();
}

/* -------------------------------------------------------------------------- */
/*                      Funkcja do sterowania światłem                        */
/* -------------------------------------------------------------------------- */
void optaAnalogTask() {
  unsigned long currentTime = timeClient.getEpochTime();  // Aktualny czas Unix

  // Sprawdzanie, czy światło jest włączone za pomocą lightSwitch
  if (!lightSwitch) {
    setLightOff();  // Jeśli lightSwitch jest wyłączony, wyłącz światło
    return;  // Zatrzymaj dalsze przetwarzanie, aby zapobiec nadpisaniu
  }

  // Sprawdzanie harmonogramu
  if (scheduleActive) {
    if (currentTime >= startDate && currentTime <= endDate) {
      applySelectedPlan();  // Wykonaj plan, jeśli w ramach harmonogramu
    } else {
      Serial.println("Harmonogram nieaktywny, światło wyłączone");
      setLightOff();
    }
  } else {
    // W trybie manualnym lub bez harmonogramu, wybrany plan jest natychmiast aktywowany
    applySelectedPlan();
  }
}

/* -------------------------------------------------------------------------- */
/*                   Funkcja wykonująca wybrany plan pracy                    */
/* -------------------------------------------------------------------------- */
void applySelectedPlan() {
  // Ustaw jasność na podstawie wybranego planu
  switch (selectedPlan) {
    case 1:
      brightness = 100.0;  // Default plan - pełna jasność
      break;
    case 2:
      brightness = 75.0;   // Mood plan - 75% jasności
      break;
    case 3:
      brightness = 25.0;   // Night plan - 25% jasności
      break;
    case 4:
      brightness = 50.0;   // Energy Saving - 50% jasności
      break;
    case 5:
      brightness = intensity.getBrightness();  // Manual - ręczna kontrola
      break;
    default:
      brightness = 100.0;  // Domyślna wartość
      break;
  }

  // Aktualizacja suwaka po zmianie planu
  if (selectedPlan != 5) {
    intensity.setBrightness(brightness);  // Ustaw suwak zgodnie z planem
  }

  // Aktualizacja DAC co określony czas (updateInterval)
  if (millis() - lastDacUpdate >= updateInterval) {
    lastDacUpdate = millis();  // Zaktualizuj czas ostatniej aktualizacji
    setDacValue(brightness);  // Ustaw wartość DAC
  }
}

/* -------------------------------------------------------------------------- */
/*             Funkcja ustawiająca wartość DAC na podstawie jasności           */
/* -------------------------------------------------------------------------- */
void setDacValue(float brightness) {
  uint16_t dac_value = map(brightness, 0, 100, 0, 8192);  // Przekształcanie jasności na wartość DAC
  AnalogExpansion exp = OptaController.getExpansion(0);
  exp.setDac(4, dac_value);  // Ustaw wartość DAC

  // Aktualizowanie statusu światła
  if (dac_value > 0) {
    xLightBedroomStatus = true;  // Światło włączone
  } else {
    xLightBedroomStatus = false;  // Światło wyłączone
  }

  // Aktualizacja zmiennej w chmurze
  ArduinoCloud.update();
}

/* -------------------------------------------------------------------------- */
/*                       Funkcja wyłączająca światło                          */
/* -------------------------------------------------------------------------- */
void setLightOff() {
  uint16_t dac_value = 0;
  AnalogExpansion exp = OptaController.getExpansion(0);
  exp.setDac(4, dac_value);  // Ustawienie DAC na 0 - światło wyłączone

  Serial.println("Światło wyłączone, DAC ustawiony na 0");

  xLightBedroomStatus = false;  // Aktualizowanie statusu na wyłączony
  ArduinoCloud.update();  // Aktualizacja statusu w chmurze
}

/* -------------------------------------------------------------------------- */
/*                      Komenda do odczytywania statusu                       */
/* -------------------------------------------------------------------------- */
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();  // Usuwa spacje i znaki nowej linii

    if (command == "status") {
      printInfo();
    } else {
      Serial.println("Nieznana komenda. Dostępne komendy: status");
    }
  }
}

/* -------------------------------------------------------------------------- */
/*                    Wyświetlanie informacji o stanie systemu                */
/* -------------------------------------------------------------------------- */
void printInfo() {
  Serial.println("Switch: " + String(lightSwitch));
  Serial.println("Plan: " + String(selectedPlan));
  Serial.println("Brightness: " + String(brightness));
  Serial.println("Scheduler aktywny? " + String(scheduleActive));
  Serial.println("Start date: " + String(startDate));
  Serial.println("End date: " + String(endDate));
  Serial.println("Length: " + String(length));
  Serial.println("xLightBedroomStatus (czy światło się świeci): " + String(xLightBedroomStatus ? "On" : "Off"));
}

void updateTime() {
  Serial.println();
  Serial.println("- TIME INFORMATION:");
  timeClient.update();
  const unsigned long epoch = timeClient.getEpochTime();
  Serial.print("- Unix time: ");
  Serial.println(epoch);
}

/* -------------------------------------------------------------------------- */
/*     Funkcje wykonywane przy zmianie zmiennych intensywności i planu         */
/* -------------------------------------------------------------------------- */
void onIntensityChange() {
  Serial.println("Zmieniono intensywność: " + String(intensity.getBrightness()));
  // Automatyczna zmiana na tryb manualny, jeśli zmieniono intensywność ręcznie
  iPlan = 5;  // Ustaw na tryb manualny
  Serial.println("Automatyczna zmiana na tryb manualny");
}

void onIPlanChange() {
  Serial.println("Zmieniono plan: " + String(iPlan));
  applySelectedPlan();  // Upewnij się, że po zmianie planu zostanie ustawiona jasność i zaktualizowany suwak
}

void onSchedulerChange() {
  // Add your code here to act upon Scheduler change
}

void onXLightBedroomStatusChange() {
  // Dodaj swoją logikę reakcji na zmianę statusu w chmurze
}

void onSMessageChange() {
  // Add your code here to act upon SMessage change
}
