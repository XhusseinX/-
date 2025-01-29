#define BLYNK_TEMPLATE_ID "TMPL4rmwiLFqh"
#define BLYNK_TEMPLATE_NAME "IoT Energy Guard"
#define BLYNK_AUTH_TOKEN "gBtkOsEL-vg-gi3pJNl168LDgj0GM068"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "NCD";
char pass[] = "N123456n";

// Pins for switches and ACS712 sensors
const int switchPins[] = { 13, 2, 5, 12 };  // GPIO pins connected to switches
const int sensorPins[] = {34, 35, 32, 39}; // Analog input pins for ACS712
const int switchPin4 = 4;                   // Pin for toggle switch
const int irPin = 25;                       // Pin for IR sensor

// Virtual pins for Blynk
const int gaugePins[] = {V0, V2, V5, V12};
const int statusPin = V1;             // Virtual pin for IR sensor status
const int totalConsumptionPin = V20;  // Virtual pin for total consumption
const int nextDayPin = V30;
const int nextWeekPin = V31;
const int nextMonthPin = V32;

// ACS712 parameters
const float sensitivity = 0.185; // Sensitivity for 5A module (adjust based on your ACS712 model)
const int adcMax = 4095;         // ESP32 ADC resolution
const float vRef = 3.3;          // ESP32 ADC reference voltage
const float offset = vRef / 2;   // Midpoint voltage

// Flags
bool startPrintingData = false;
bool sentA = false;
bool lastSwitchState = HIGH;  // To track previous switch state

// Function to read current from ACS712 sensor
float readCurrent(int pin) {
  float voltage = (analogRead(pin) * vRef) / adcMax; // Convert ADC reading to voltage
  float current = (voltage - offset) / sensitivity;  // Convert voltage to current using sensitivity
  return abs(current); // Return absolute value to avoid negative readings
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  delay(1000);

  // Set up switch pins as inputs
  for (int i = 0; i < 4; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
  }
  pinMode(switchPin4, INPUT_PULLUP);
  pinMode(irPin, INPUT);
}

void loop() {
  Blynk.run();

  int switchState = digitalRead(switchPin4);
  if (switchState == LOW && lastSwitchState == HIGH) {
    sentA = !sentA;
    startPrintingData = !startPrintingData;
    Serial.println("A");
    delay(300);
  }
  lastSwitchState = switchState;

  if (startPrintingData) {
    int irState = digitalRead(irPin);
    Blynk.virtualWrite(statusPin, irState == LOW ? "There is someone " : " empty room ");

    float totalConsumption = 0.0;
    String valuesToPrint = "";

    for (int i = 0; i < 4; i++) {
      float current = readCurrent(sensorPins[i]);
      float power = current * 220.0; // Assume 220V for power calculation

      // Send the calculated power consumption to Blynk for each device
      // This represents the real-time power consumption of the device connected to each ACS712 sensor
      Blynk.virtualWrite(gaugePins[i], power);
      valuesToPrint += String(power, 3) + ",";
      totalConsumption += power;
    }

    Blynk.virtualWrite(totalConsumptionPin, totalConsumption);
    Serial.print("Total Consumption: ");
    Serial.println(totalConsumption, 3);

    valuesToPrint.remove(valuesToPrint.length() - 1);
    Serial.println(valuesToPrint);

    if (Serial.available()) {
      String receivedData = Serial.readStringUntil('\n');
      receivedData.trim();
      int firstComma = receivedData.indexOf(',');
      int secondComma = receivedData.indexOf(',', firstComma + 1);

      if (firstComma != -1 && secondComma != -1) {
        double nextDay = receivedData.substring(0, firstComma).toFloat();
        double nextWeek = receivedData.substring(firstComma + 1, secondComma).toFloat();
        double nextMonth = receivedData.substring(secondComma + 1).toFloat();

        Blynk.virtualWrite(nextDayPin, nextDay);
        Blynk.virtualWrite(nextWeekPin, nextWeek);
        Blynk.virtualWrite(nextMonthPin, nextMonth);
      }
    }
    delay(1000);
  }
}
