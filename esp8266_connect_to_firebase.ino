#include <ESP8266WiFi.h>        // Include the Wi-Fi library
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "kerberos"
#define WIFI_PASSWORD "touchdown"
#define sensorPin 4 //gpio4

// Insert Firebase project API Key
#define API_KEY "AIzaSyBlCFd-wMevT06tl-Tvt1pF9JvbIpAdtmc"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp8266-arduino-2f497-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* ssid     = "kerberos";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "touchdown";     // The password of the Wi-Fi network
//Firebase object = Firebase.get("runner"); //reference the path to get commands from
//Firebase debug = Firebase.get("debug"); 
String oldcmd = "";
String oldrtn = "1";

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0

void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  pinMode(sensorPin, INPUT);//set the sensor pin as input
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  // Set the name of the board and other data
    if (Firebase.RTDB.setString(&fbdo, "config/board", "ESP8266")){
      Firebase.RTDB.setString(&fbdo, "config/author", "kerberos");
      Firebase.RTDB.setString(&fbdo, "config/project", "ESP8266 live-wire triggered security system");
      Firebase.RTDB.setString(&fbdo, "config/date", "Sunday,26th June");
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
    }
    //get last value
    int lives = 0;
    if (Firebase.RTDB.getInt(&fbdo, "debug/lives")) {
                  if (fbdo.dataType() == "int") {
                      lives = fbdo.intData();
                      Serial.println(lives);
                  }
                }
                else {
                  Serial.println(fbdo.errorReason());
                }
    int push_lives = lives + 1;
    //String s = lexical_cast<string>(push_lives);
    //for debug purposes
    if (Firebase.RTDB.setInt(&fbdo, "debug/lives", push_lives)){
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
    }
    
}
  


void loop() {
    ////////////////////////////////////////////////////////////////////
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    count++;
    
    //read the command from database
    String cmd = "";
    if (Firebase.RTDB.getString(&fbdo, "runner/command")) {
      if (fbdo.dataType() == "string") {
        cmd = fbdo.stringData();
        Serial.println(cmd);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    
    //as long as this value changes,the system is online.
    if (Firebase.RTDB.setInt(&fbdo, "config/live", count)){
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
    }

    int sensorValue = digitalRead(sensorPin);
    Serial.println("sensor value is :%d" + sensorValue);
    if(sensorValue == 0){
      //if continously 1 the system is live
      if (Firebase.RTDB.setInt(&fbdo, "debug/trigger", sensorValue)){
        //system has been triggered-turn on buzzer 
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
    }
      }
      if(oldcmd != cmd){
        runner(cmd);
        oldcmd = cmd;
        }
    
  }

}

void runner(String command){
            if(command == "reset"){
              if (Firebase.RTDB.setString(&fbdo, "runner/rtn", "reset true")){
                  Serial.println("TYPE: " + fbdo.dataType());
                  }
              else {
                  Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
                  }
                  if (Firebase.RTDB.setString(&fbdo, "runner/command", "reset true")){
                  Serial.println("TYPE: " + fbdo.dataType());
                  }
              else {
                  Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
                  }
              resetFunc(); 
            }else if(command == "start hotspot"){
              String bs = "";
              if (Firebase.RTDB.getString(&fbdo, "runner/bssid")) {
                  if (fbdo.dataType() == "string") {
                      bs = fbdo.stringData();
                      Serial.println(bs);
                  }
                }
                else {
                  Serial.println(fbdo.errorReason());
                }
              Serial.println(bs);
              String ps = "";
              if (Firebase.RTDB.getString(&fbdo, "runner/password")) {
                  if (fbdo.dataType() == "string") {
                      ps = fbdo.stringData();
                      Serial.println(ps);
                  }
                }
                else {
                  Serial.println(fbdo.errorReason());
                }
              starthotspot(bs,ps);
            }
}

void starthotspot(String bssid,String pass){
  
  WiFi.softAP(bssid, pass);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer
  if (Firebase.RTDB.setString(&fbdo, "runner/rtn", "hotspot true")){
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED WITH REASON: " + fbdo.errorReason());
    }

}
