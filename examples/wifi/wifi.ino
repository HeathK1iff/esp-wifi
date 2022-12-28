
#include <WifiCtrl.h>

#define SSID "XXX"
#define SSID_PASS "YYYY"

WifiCtrl wifi(&WiFi, true, "default-wifi", "12345");

void setup(){
    Serial.begin(115200);
    wifi.begin("device", "device-api", "12345");
}

void loop(){
    wifi.loop();
}