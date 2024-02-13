/*
  Ameba 1 window alerting system_magnet switch

  Item list:
  1. Magnetic reed switch (closed when near a magnet) x1
  2. buzzer x1
  3. Ameba 1 dev. board x2
  4. Jumper wires 
  5. LED (red) x1

  Application:
  when windows open, ameba 1_1 send a message to ameba 1_2 to trigger a buzzer and LED
*/

#include <WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

char ssid[] = "Your_WiFi_SSID";      // your network SSID (name)
char pass[] = "Your_WiFi_Password";  // your network password

int status  = WL_IDLE_STATUS;    // the Wifi radio's status

char mqttServer[]     = "cloud.amebaiot.com";
char clientId[]       = "amebaClientXXX";//replace XXX with your initials
char clientUser[]     = "YourUserName";//same username as www.amebaiot.com
char clientPass[]     = "YourPassWord";//same username as www.amebaiot.com
char subscribeTopic[] = "sub/security";
char publishTopic[]   = "pub/security";
char publishHello[]   = "---MQTT server Connected!---";
char publishOpen[]    = "open";
char publishClose[]   = "close";
#define inputPin      13

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

bool magnetSensor() {
    //if magnet present, pin 13 should reads HIGH and returning true,else false
    if (digitalRead(inputPin)) {
        return true;
    } else {
        return false;
    }
}

WiFiClient wifiClient;
PubSubClient client(mqttServer, 1883, callback, wifiClient);

void setup() {
    Serial.begin(9600);
    pinMode(inputPin, INPUT);

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    // Note - the default maximum packet size is 512 bytes. If the
    // combined length of clientId, username and password exceed this,
    // you will need to increase the value of MQTT_MAX_PACKET_SIZE in
    // PubSubClient.h
    if (client.connect(clientId, clientUser, clientPass)) {
        client.publish(publishTopic, publishHello);
        client.subscribe(subscribeTopic);
    }
}

int counter = 1;

void loop() {
    //magnetSensor() normally return FALSE when window is closed, TRUE when window opened
    if (magnetSensor()) {
        //counter ensure only publish once every time window state changes 
        if (counter > 0) {
            client.publish(publishTopic, publishClose);
            Serial.println("###");
            counter = 0;
        }
    } else {
        client.publish(publishTopic, publishOpen);
        Serial.println("---window is opened!---");
        counter = 1;
    }
    client.loop();
    delay(1000);
}//end of loop()
