#define BLYNK_TEMPLATE_ID "TMPL43SorwpJl"
#define BLYNK_TEMPLATE_NAME "hamza"
#define BLYNK_AUTH_TOKEN "U2PfIVo9thUTYqglsY_PtBE5L_0Zk_mP"  //Include the library files
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Enter your Auth token
char auth[] = "U2PfIVo9thUTYqglsY_PtBE5L_0Zk_mP";

//Enter your WIF SSID and password
char ssid[] = "NCD";
char pass[] = "N123456n";

void setup() {
  // Debug console
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
}

void loop() {
  Blynk.run();
}