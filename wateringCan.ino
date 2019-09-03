#include <BlynkSimpleEsp8266.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

/*
GPIO00 - BUTTON
GPIO12 - RELAY
GPIO13 - LED1
GPIO03 - RX PIN
GPIO01 - TX PIN
Use Port COM5
TO upload:
-hold down button upon plug in
To run firmware:
Unplug and repluf without holding button
On lowest setting, takes 90sec to fill a solo cup (90,000 miliseconds), 1ml per 180milliseconds
want to add notification:
*/


#define BUTTON 0
#define RELAY 12 //pump control
#define LED 13
//#define LED 2 //used only for testing unit

#define BSWITCH V0 // turn off watering in blynk app
#define BWATERAMOUNT V1 //water amount from blynk app in milliliters
#define BWATERTIME V2 //watering time of day in seconds(3600 * hours + 60 * minutes) from Blynk app
#define BWATERBUTTON V3 //manual watering button


//times getting stuff
const char *ssid     = "your ssid here";
const char *pass = "your pass here";
const char auth[] = "your Blynk authorization code"; //blynk auth code
const long utcOffsetInSeconds = -14400;  //Put UTC hour offset here in seconds (this is -4 * 3600)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", utcOffsetInSeconds);

//button stuff
int state = 0;
long debounce = 200;
unsigned long dbtimer = 0;
int prev = 1;

//to many gobal variables
int prevSec = -1; //way to save last second
int autoWatering = 1; //is automatic watering on or off? (controlled by Blynk)
int waterAmount = 20; //water amount in ml
int waterTime = 0; //time of day to autowater
long timeInSec = 0; //the time of day in seconds
long startWater = 0; //for conrolling how long the pump is on
int currentlyWatering = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  
  digitalWrite(LED, HIGH); //LED is off when HIGH
  digitalWrite(RELAY, LOW);

  Blynk.begin(auth, ssid, pass);
  
  timeClient.begin();
  prevSec = timeClient.getSeconds();
}

  
void loop() {
  Blynk.run();
  timeClient.update();
  timeInSec = (timeClient.getHours() * 3600 + timeClient.getMinutes() * 60 + timeClient.getSeconds()); //convert time into seconds

  //Automatic watering at waterTime
  if (autoWatering && ((waterTime == timeInSec) || (waterTime == timeInSec - 1) || (waterTime == timeInSec + 1)) && !currentlyWatering) { //three comparisons because I'm lazy and timeInSec is sometimes off by 1
    water();
  }
  
  //check if enough time has elapsed and stop watering
  if ((timeClient.getEpochTime() - startWater >= round(waterAmount / 5.6)) && currentlyWatering) {  //5.6ml water is pumped roughly every second
    stopWater();
  }
  
  //for debug purposes, print time to serial
  if (timeClient.getSeconds() == prevSec + 1 || (prevSec == 59 && timeClient.getSeconds() == 0)) { //limit reports to 1 per second
    getTime();
  }

   //button toggles motor and LED
  if (digitalRead(BUTTON) != prev && (millis() - dbtimer) >= debounce){ // Button state is LOW/0 when pressed
    dbtimer = millis();  
    state = 1 - state;
  }
  
  digitalWrite(LED, 1 - state); //LED IS OFF WHEN HIGH
  digitalWrite(RELAY, state);
  prev = digitalRead(BUTTON);
}


//PRINTS TIME TO SERIAL
void getTime() {
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
    prevSec = timeClient.getSeconds();
    Serial.println(timeInSec);
}

//waters plants for set amount
void water() {
    Serial.print("Watering...\n");
    state = 1;
    startWater = timeClient.getEpochTime();
    currentlyWatering = 1;
}
void stopWater() {
    state = 0;   
    Serial.print("Cease watering\n"); 
    Blynk.notify("Your plants have been watered, Sir.");
    currentlyWatering = 0;
}


//Blynk stuff below
BLYNK_WRITE(BSWITCH) {   // turns automatic watering on/off
  autoWatering = param.asInt();
  Serial.print("AutoWatering switched to ");
  Serial.println(autoWatering);
}
BLYNK_WRITE(BWATERAMOUNT) { //the amount of water in ml
  waterAmount = param.asInt(); 
  Serial.println(waterAmount);
  Serial.println(waterAmount / 5.6);
}
BLYNK_WRITE(BWATERTIME) { //time of day for autowatering
  waterTime = param.asInt();
  Serial.print("New waterTime setting: ");
  Serial.println(waterTime);
}
BLYNK_WRITE(BWATERBUTTON) { //manual does one cylce of watering
  if (param.asInt() && !currentlyWatering) {
    water(); }
}
