#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Battsafe"
#define BLYNK_AUTH_TOKEN    "YOUR_DEVICE_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// ESP32-S3 example pins; verify board pinout.
#define TEMP_PIN       4
#define VOLTAGE_PIN    1
#define GAS_PIN        2
#define BUZZER_PIN     5
#define WARNING_LED    6
#define CUTOFF_LED     7

OneWire oneWire(TEMP_PIN);
DallasTemperature tempSensor(&oneWire);
BlynkTimer timer;

// Replace these example values with means/stds from Battsafe_model_config.json
// These defaults are based on the bundled synthetic demo data, NOT real EV data.
const float TEMP_MEAN = 29.0;
const float TEMP_STD  = 3.2;
const float VOLT_MEAN = 13.2;
const float VOLT_STD  = 0.7;
const float GAS_MEAN  = 450.0;
const float GAS_STD   = 110.0;
const float Z_LIMIT   = 3.0;

const float SIMULATED_MAX_VOLTAGE = 24.0;
const float TEMP_HARD_LIMIT_C = 45.0;
const float VOLTAGE_LOW_LIMIT = 10.0;
const float VOLTAGE_HIGH_LIMIT = 16.0;
const int GAS_HARD_LIMIT = 1800;

void publishReadings() {
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);
  bool tempValid = tempC != DEVICE_DISCONNECTED_C &&
                   tempC >= -40.0 && tempC <= 125.0;

  int voltageRaw = analogRead(VOLTAGE_PIN);
  float voltageV = (voltageRaw / 4095.0f) * SIMULATED_MAX_VOLTAGE;
  int gasRaw = analogRead(GAS_PIN);

  // Lightweight embedded anomaly score: max absolute z-score.
  float zTemp = tempValid ? fabs((tempC - TEMP_MEAN) / max(TEMP_STD, 0.01f)) : 99.0f;
  float zVolt = fabs((voltageV - VOLT_MEAN) / max(VOLT_STD, 0.01f));
  float zGas  = fabs((gasRaw - GAS_MEAN) / max(GAS_STD, 1.0f));
  float anomalyScore = max(zTemp, max(zVolt, zGas));

  bool hardFault = !tempValid ||
                   (tempValid && tempC >= TEMP_HARD_LIMIT_C) ||
                   voltageV < VOLTAGE_LOW_LIMIT ||
                   voltageV > VOLTAGE_HIGH_LIMIT ||
                   gasRaw >= GAS_HARD_LIMIT;
  bool alert = hardFault || anomalyScore >= Z_LIMIT;

  digitalWrite(WARNING_LED, alert ? HIGH : LOW);
  digitalWrite(CUTOFF_LED, alert ? HIGH : LOW);

  if (alert) tone(BUZZER_PIN, 2000);
  else noTone(BUZZER_PIN);

  // Blynk datastream mapping:
  // V0 temperature, V1 voltage, V2 gas ADC,
  // V3 anomaly score, V4 alert (0/1), V5 demo cutoff flag (0/1).
  if (Blynk.connected()) {
    if (tempValid) Blynk.virtualWrite(V0, tempC);
    Blynk.virtualWrite(V1, voltageV);
    Blynk.virtualWrite(V2, gasRaw);
    Blynk.virtualWrite(V3, anomalyScore);
    Blynk.virtualWrite(V4, alert ? 1 : 0);
    Blynk.virtualWrite(V5, alert ? 1 : 0);
  }

  Serial.printf("Temp=%s%.2f C | Voltage=%.2f V (sim) | Gas=%d | Score=%.2f | %s\n",
                tempValid ? "" : "INVALID/",
                tempValid ? tempC : 0.0f,
                voltageV, gasRaw, anomalyScore,
                alert ? "ALERT" : "NORMAL");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WARNING_LED, OUTPUT);
  pinMode(CUTOFF_LED, OUTPUT);
  digitalWrite(WARNING_LED, LOW);
  digitalWrite(CUTOFF_LED, LOW);
  noTone(BUZZER_PIN);

  analogReadResolution(12);
  tempSensor.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, publishReadings);
}

void loop() {
  Blynk.run();
  timer.run();
}
