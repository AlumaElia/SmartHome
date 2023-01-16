#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Bounce2.h>

//LED pin for temperature monitor
const int boilerPin = 2;

//button on ESP8266 GPIO0
const int buttonPin = 0;

//topic to subscribe to for the temperature
char* tempTopic = "/house/temp_sensor";

//topic to publish to confirm that the boiler has been turned on for the python script to log
char* boilerConfirmTopic = "/house/boiler1confirm";

char* boilerTopic = "/house/boiler";

//create an instance of the bounce class
Bounce myButton = Bounce();

//EDIT THESE LINES TO MATCH YOUR SETUP

#define MQTT_SERVER "91.121.93.94"
const char* ssid = "ssid";
const char* password = "password";
char headerByte; 
String topicStr;
char byteToSend;
float byteToSend_float;
int numOfShowers;
static boolean isOn = false;  //static var to keep track of the intended current boiler state


// millis 
unsigned long currentMillis;
unsigned long startMillis;
const unsigned long period= 2000; // how much time?


// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void setup() {

 // initial start time
  startMillis=millis();
  
    //initialize the button pin as an input
  pinMode(buttonPin, INPUT);

  myButton.attach(buttonPin);
  myButton.interval(5);
  
  //setup pin as output
  pinMode(boilerPin, OUTPUT);
  digitalWrite(boilerPin, LOW);
  
  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);


  //start wifi subsystem
  WiFi.begin(ssid, password);

  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
      delay(2000);
}



void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}
  //maintain MQTT connection
  client.loop();

   //monitor the button
  checkButton();
  timer();
  
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
}

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  topicStr = topic; 
  byteToSend = 0;
  numOfShowers=3;
  
    //handle tempTopic updates
  if(topicStr.equals(tempTopic)){
    byteToSend = char(*payload);  //set byte to send to the payload
    byteToSend_float= byteToSend;
    Serial.println(byteToSend_float);
  }

    if(topicStr.equals(boilerTopic)){
    byteToSend = char(*payload);  //set byte to send to the payload
    byteToSend_float= byteToSend;
    
    payload[length] = 0;
    String recv_payload = String(( char *) payload);
    Serial.println(recv_payload);
    if (recv_payload == "update") {
      if (isOn == true) {
        client.publish(boilerConfirmTopic,"On");
        Serial.println("publish on");
      }
      else {
        client.publish(boilerConfirmTopic,"Off");
        Serial.println("publish off");
      }
    }

    else {
      if (recv_payload == "1"){
      numOfShowers = 1;
      Serial.println(numOfShowers);
      }
      else if (recv_payload == "2"){
             numOfShowers = 2;
             Serial.println(numOfShowers);
            }
            else if (recv_payload == "more"){
               numOfShowers = 3;
               Serial.println(numOfShowers);
              }
    boilerSwitchUpdate();
    }
  }
}

void checkButton(){
 
  if(myButton.update() && myButton.read() == HIGH){ //update the button and check for HIGH or LOW state
     Serial.println("button pressed");
     numOfShowers=3;
     boilerSwitchUpdate();
  }
}

void timer(){
   if (isOn == true) {
     currentMillis = millis();  //get the current "time" (the number of milliseconds since the program started)
     if (currentMillis - startMillis >= period*numOfShowers){  //test whether the period has elapsed
         Serial.print("the boiler was on for: ");
         Serial.println(currentMillis - startMillis);
         startMillis = millis();
         boilerSwitchUpdate();     
     }   
   }
}

void boilerSwitchUpdate(){
         //on false, the boiler is off so tell it to turn on and set the internal var to true
       
    if(isOn == false){
      digitalWrite(boilerPin, HIGH);
      client.publish(boilerConfirmTopic,"On");
      startMillis = millis();  //IMPORTANT to save the start time of the current LED state.
      isOn = true;
    }

    //else (on true), the light is on so tell it to turn off and set the internal var to false
    else{
      digitalWrite(boilerPin, LOW);
      client.publish(boilerConfirmTopic,"Off");
      isOn = false;
    }
}

//networking functions

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
   //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);
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
        
        client.subscribe(boilerTopic);
        client.subscribe(tempTopic);
        
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
