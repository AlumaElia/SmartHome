
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Bounce2.h>

//LED on ESP8266 GPIO2
const int lightPin = 2;

//button on ESP8266 GPIO0
const int buttonPin = 0;

//topic to subscribe to the light
const char* lightTopic = "/house/light1";

//topic to publish to confirm that the light has been turned on for the python script to log
const char* lightConfirmTopic = "/house/light1confirm";


//create an instance of the bounce class
Bounce myButton = Bounce();

//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "91.121.93.94"
const char* ssid = "ssid";
const char* password = "password";

static boolean isOn = false;  //static var to keep track of the intended current boiler state

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);


WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void setup() {
  
  //initialize the button pin as an input
  pinMode(buttonPin, INPUT);

  myButton.attach(buttonPin);
  myButton.interval(5);

  //initialize the light as an output and set to LOW (off)
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, LOW);

  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);


  //start wifi subsystem
  WiFi.begin(ssid, password);

  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
      delay(200);
}

void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

 //monitor the button
  checkButton();
  
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
}


//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  payload[length] = 0;
  String recv_payload = String(( char *) payload);
  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.println(recv_payload);
  if(topicStr.equals(lightTopic)){
     if (recv_payload == "update"){
        if (isOn == true) 
        {
            client.publish(lightConfirmTopic, "On");
            Serial.print("p");
        }
        else
        {
            client.publish(lightConfirmTopic, "Off");
            Serial.print("b");
        }
     }
     else 
     { 
     lightSwitchUpdate();
     }
  }
}

void checkButton(){
  //static boolean isOn = false;  //static var to keep track of the intended current light state

  if(myButton.update() && myButton.read() == HIGH){ //update the button and check for HIGH or LOW state
    lightSwitchUpdate();
  }
}

void lightSwitchUpdate(){

  //on false, the light is off so tell it to turn on and set the internal var to true, 
    if(isOn == false){
      digitalWrite(lightPin, HIGH);
      client.publish(lightConfirmTopic, "On"); //publish to the confirmation topic so the python script can log it
      Serial.println("light on");
      isOn = true;
    }

    //else (on true), the light is on so tell it to turn off and set the internal var to false
    else{
      digitalWrite(lightPin, LOW);
      client.publish(lightConfirmTopic, "Off"); //publish to the confirmation topic so the python script can log it
      Serial.println("light off");
      isOn = false;
    }
}

//networking functions

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected");
        client.subscribe(lightTopic);
      }

      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
    }
  }
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    } 
  }

  return result;
}
