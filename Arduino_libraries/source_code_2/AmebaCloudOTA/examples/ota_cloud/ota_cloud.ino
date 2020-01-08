#include "WiFi.h"
#include "OTA_r.h"


char ssid[] = "ssid";     // your network SSID (name)
char pass[] = "password";  // your network password (use for WPA, or use as key for WEP)
char REMOTE_ADDR[]="192.168.1.101";
int  REMOTE_PORT = 80;
int status = WL_IDLE_STATUS;

void setup() {
  // put your setup code here, to run once:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid,pass);
    // wait 10 seconds for connection:
    delay(1000);
  }
  int checksum = OTA.gatherOTAinfo(REMOTE_ADDR,"/check.txt", REMOTE_PORT);
  if( checksum != -1) Serial.println("info_ok");
  if(OTA.beginRemote(REMOTE_ADDR,"/ota.bin",REMOTE_PORT, checksum) != -1) Serial.println("OK");
  else Serial.println("FAIL");
}

void loop() {
  // put your main code here, to run repeatedly:

}
