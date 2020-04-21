/*
  Ameba 1 window alerting system_Buzzer

  Item list:
  1. Magnetic reed switch (circuit closed when near a magnet) x1
  2. buzzer x1
  3. Ameba 1 dev. board x2
  4. Jumper wires 
  5. LED (red) x1

  Application:
  when windows open, ameba 1_1 send a message to ameba 1_2 to trigger a buzzer and LED
*/

#include <WiFi.h>
#include <PubSubClient.h>

#define NOTE_G5  784
#define NOTE_G4  392

// Update these with values suitable for your network.

char ssid[] = "Your_WiFi_SSID";      // your network SSID (name)
char pass[] = "Your_WiFi_Password";  // your network password

int status  = WL_IDLE_STATUS;    // the Wifi radio's status

char mqttServer[]     = "cloud.amebaiot.com";
char clientId[]       = "amebaClient2XXX"; //replace XXX with your initials
char clientUser[]     = "YourUserName";//same username as www.amebaiot.com
char clientPass[]     = "YourPassWord";//same username as www.amebaiot.com
char subscribeTopic[] = "pub/security";
char publishTopic[]   = "sub/security";
char publishHello[]   = "---MQTT server Connected!---";

#define buzzerPin     8  //PWM pin
#define ledPin        9  

int alarmFlag = 0;

void callback(char* topic, byte* payload, unsigned int length) {
    // handle message arrived
    char arr[30] = {NULL};
    int i = 0;
    //printf("the payload length is %d\n",length);
    for (i = 0; length > 0; i++) {
        arr[i] = *payload;
        payload++;
        length--;
        //printf("payload received: %c\n",arr[i]);
    }

    if ((arr[0] == 'o') && (arr[1] == 'p') && (arr[2] == 'e') && (arr[3] == 'n')) {
        printf("---Received a OPEN msg---\n");
        alarmFlag = 1;
    } else if ((arr[0] == 'c') && (arr[1] == 'l') && (arr[2] == 'o') && (arr[3] == 's') && (arr[4] == 'e')) {
        printf("---Received a CLOSE msg---\n");
        alarmFlag = 0;
    } else {
        printf("---Received an INVALID msg---\n");
    }
}

WiFiClient wifiClient;
PubSubClient client(mqttServer, 1883, callback, wifiClient);

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);

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

void loop() {
    if (alarmFlag == 1) {
        //set alarm and blink red LED
        digitalWrite(ledPin, HIGH);
        tone(buzzerPin, NOTE_G5); //pin 8, G5=784Hz
        delay(500);//half note
        digitalWrite(ledPin, LOW);
        tone(buzzerPin, NOTE_G4); //pin 8, G4=392Hz
        delay(500);//half note
    } else {
        noTone(buzzerPin);
        digitalWrite(ledPin, LOW);
    }
    client.loop(); //loop every 1 second
}// end of loop()
