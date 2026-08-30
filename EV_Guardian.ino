/*
  EV Guardian — prototype EV safety monitor

  For an ESP32 connected to a LOW-VOLTAGE demonstration battery only.
  Do not connect an ESP32, an ACS712, or a basic resistor divider directly
  to a traction battery / high-voltage EV pack. Use certified isolated
  measurement hardware and professional validation for any real vehicle.
*/

// ---------- Pin connections ----------
const int TEMP_PIN = 32;       // LM35 temperature sensor output
const int VOLTAGE_PIN = 34;    // Voltage-divider output (max 3.3 V)
const int CURRENT_PIN = 35;    // ACS712 analogue output (max 3.3 V)
const int GREEN_LED = 25;
const int YELLOW_LED = 26;
const int RED_LED = 27;
const int BUZZER = 14;         // Active buzzer module

// ---------- Prototype calibration ----------
// Change these values after measuring your own circuit with a multimeter.
const float ADC_REFERENCE_V = 3.30;
const float ADC_MAX = 4095.0;
const float VOLTAGE_DIVIDER_RATIO = 2.0;  // e.g. 10k / 10k divider
const float CURRENT_ZERO_V = 1.65;        // ACS712 midpoint at 0 A
const float CURRENT_SENSITIVITY = 0.185;  // V/A for ACS712 5 A version

// ---------- Demonstration thresholds ----------
const float TEMP_WARNING_C = 45.0;
const float TEMP_CRITICAL_C = 55.0;
const float MIN_BATTERY_V = 3.20;         // Example: one Li-ion cell
const float MAX_BATTERY_V = 4.25;
const float CURRENT_WARNING_A = 3.5;
const float CURRENT_CRITICAL_A = 4.5;
const unsigned long SAMPLE_INTERVAL_MS = 1000;

enum SafetyLevel { SAFE, WARNING, CRITICAL };
unsigned long lastSample = 0;

float readPinVoltage(int pin) {
  return (analogRead(pin) / ADC_MAX) * ADC_REFERENCE_V;
}

float readTemperatureC() {
  // LM35 output changes by 10 mV per degree Celsius.
  return readPinVoltage(TEMP_PIN) * 100.0;
}

float readBatteryVoltage() {
  return readPinVoltage(VOLTAGE_PIN) * VOLTAGE_DIVIDER_RATIO;
}

float readCurrentA() {
  float amps = (readPinVoltage(CURRENT_PIN) - CURRENT_ZERO_V) / CURRENT_SENSITIVITY;
  return abs(amps);
}

SafetyLevel assessSafety(float temperatureC, float batteryV, float currentA) {
  if (temperatureC >= TEMP_CRITICAL_C ||
      batteryV < MIN_BATTERY_V || batteryV > MAX_BATTERY_V ||
      currentA >= CURRENT_CRITICAL_A) {
    return CRITICAL;
  }

  if (temperatureC >= TEMP_WARNING_C || currentA >= CURRENT_WARNING_A) {
    return WARNING;
  }

  return SAFE;
}

int safetyScore(SafetyLevel level) {
  if (level == SAFE) return 100;
  if (level == WARNING) return 60;
  return 20;
}

const char* levelName(SafetyLevel level) {
  if (level == SAFE) return "SAFE";
  if (level == WARNING) return "WARNING";
  return "CRITICAL";
}

String faultSummary(float temperatureC, float batteryV, float currentA) {
  String faults = "";
  if (temperatureC >= TEMP_WARNING_C) faults += "High temperature; ";
  if (batteryV < MIN_BATTERY_V) faults += "Low voltage; ";
  if (batteryV > MAX_BATTERY_V) faults += "Over-voltage; ";
  if (currentA >= CURRENT_WARNING_A) faults += "High current; ";
  if (faults.length() == 0) return "No fault detected";
  return faults;
}

void showAlert(SafetyLevel level) {
  digitalWrite(GREEN_LED, level == SAFE);
  digitalWrite(YELLOW_LED, level == WARNING);
  digitalWrite(RED_LED, level == CRITICAL);

  // Active buzzer: audible only for a critical condition.
  digitalWrite(BUZZER, level == CRITICAL);
}

void printReading(float temperatureC, float batteryV, float currentA, SafetyLevel level) {
  Serial.println("----------------------------------------");
  Serial.printf("Temperature: %.1f C\n", temperatureC);
  Serial.printf("Battery:     %.2f V\n", batteryV);
  Serial.printf("Current:     %.2f A\n", currentA);
  Serial.printf("Status:      %s\n", levelName(level));
  Serial.printf("Safety score:%d / 100\n", safetyScore(level));
  Serial.print("Details:     ");
  Serial.println(faultSummary(temperatureC, batteryV, currentA));
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  showAlert(SAFE);
  Serial.println("EV Guardian prototype started");
}

void loop() {
  if (millis() - lastSample < SAMPLE_INTERVAL_MS) return;
  lastSample = millis();

  float temperatureC = readTemperatureC();
  float batteryV = readBatteryVoltage();
  float currentA = readCurrentA();
  SafetyLevel level = assessSafety(temperatureC, batteryV, currentA);

  showAlert(level);
  printReading(temperatureC, batteryV, currentA, level);
}
