// Define pins for LEDs

#include <PubSubClient.h>
#include <ESP8266WiFi.h>


//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "91.121.93.94"
const char* ssid = "Hodaya";
const char* password = "12345678";

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

// millis 
unsigned long currentMillis;
unsigned long startMillis;
const unsigned long period= 10000; // how much time? 

bool isOn = false;
bool isMove= false;
 
//motion sensor status
int detectedLED ; 
 
// Input from Motion Sensor on ESP8266 GPIO2
const int pirPin = 2; 

// Variable to store value from PIR
int pirValue; 

//topic to publish to for controlling the other ESP module
const char* lightTopic = "/house/light1";

//topic to subscribe to the light
const char* lightConfirmTopic = "/house/light1confirm";


WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);
 
void setup() {
  
  // initial start time
  startMillis=millis();
 
  // Setup PIR as Input
  pinMode(pirPin, INPUT);
   
 // Initial 1 Minute Delay to stabilize sensor
 digitalWrite(detectedLED, LOW);
 Serial.println("wait");
 // delay(600); 
 Serial.println("ready");

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


void loop() {

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();
  Serial.println(isOn);
  checkMotion(); 
  
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
}

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
 //convert topic to string to make it easier to work with
  String topicStr = topic; 

  //Print out some debugging info
  //Serial.println("Callback update.");
 // Serial.print("Topic: ");
  //Serial.println(topicStr);

  payload[length] = 0;
  String recv_payload = String(( char *) payload);
   
  //switch is turn on 
  if(recv_payload == "On"){
     isOn=true;
     Serial.println("2"); 
  }
 // switch is turn off 
  if (recv_payload == "Off") {
    isOn=false;
    isMove= false; 
    Serial.println("4"); 
  }
 }

void checkMotion(){
// Get value from Light sensor
 //ldrValue = digitalRead(ldrPin);

  
    // Get value from motion sensor
    pirValue = digitalRead(pirPin);
    
    // See if motion Detected for the first time or the user turn the switch off
    if (pirValue == 1){
       isMove=true;
       if (isOn == false) { 
           client.publish(lightTopic, "On"); //publish movment to the broker, 
           Serial.println("11");
        }
    }
    if (pirValue == 0) {
       Serial.println("3");  
     currentMillis = millis();  //get the current "time" (the number of milliseconds since the program started)
     if (currentMillis - startMillis >= period){  //test whether the period has elapsed
         if(isMove==false){
            //publish no movment to the broker, 
            client.publish(lightTopic, "Off"); 
         }
         startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
     }   
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
        client.subscribe(lightConfirmTopic);
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
