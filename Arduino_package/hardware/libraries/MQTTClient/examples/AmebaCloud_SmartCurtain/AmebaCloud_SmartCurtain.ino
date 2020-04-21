/*
  Ameba 1 SmartCurtainSystem

  Item list:
  1. Ameba 1 dev. board
  2. Jumper wires 
  3. Servo motor

  Application:
  remotely control curtain to open or close
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <AmebaServo.h>

#define closeCurtain  179
#define openCurtain   0

// Update these with values suitable for your network.
char ssid[] = "your_WiFi_SSID";      // your network SSID (name)
char pass[] = "your_WiFi_password";  // your network password

int status  = WL_IDLE_STATUS;    // the Wifi radio's status

char mqttServer[]     = "cloud.amebaiot.com";
char clientId[]       = "amebaClient_XXX";    //replace XXX with your initials 
char clientUser[]     = "your_Amebaiot_Username";
char clientPass[]     = "your_Amebaiot_Password";
char publishTopic[]   = "amebapubto";
char publishHello[]   = "---MQTT server Connected!---";
char subscribeTopic[] = "amebasubto"; 
char pubClosing[]     = "Curtain closing";
char pubOpenning[]    = "Curtain openning";

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(mqttServer, 1883, callback, wifiClient);
AmebaServo myservo;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
    char arr[30] = {NULL};
    int i = 0;
    printf("the payload length is %d\n", length);
    for (i = 0; length > 0; i++) {
        arr[i] = *payload;
        payload++;
        length--;
        //printf("payload received: %c\n",arr[i]);
    }

    if ((arr[0] == 'o') && (arr[1] == 'p') && (arr[2] == 'e') && (arr[3] == 'n')) {
        printf("---Received a OPEN msg, openning curtain now---\n"); 
        myservo.write(openCurtain);
        client.publish(publishTopic,pubOpenning);   
    } else if ((arr[0] == 'c') && (arr[1] == 'l') && (arr[2] == 'o') && (arr[3] == 's') && (arr[4] == 'e')) {
        printf("---Received a CLOSE msg, closing curtain now---"); 
        myservo.write(closeCurtain);
        client.publish(publishTopic,pubClosing);
    } else {
        printf("---Received an INVALID msg---\n");
    }
} // end of MQTT callback


void setup() {
    Serial.begin(9600);
    myservo.attach(9);          // attaches the servo on pin 9 to the servo object
    myservo.write(closeCurtain); //defualt curtain is close

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
} //end of setup()

void loop() {
    client.loop();
} // end of loop()
