#include <Arduino.h>
#include <fauxmoESP.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
                                  // https://youtu.be/A-P20vC7zq4

#define SERIAL_BAUDRATE 115200    // serial monitor speed
#define OUTPUTPIN D5              // wemos output pin
 
fauxmoESP fauxmo;

int pinStatus = 0;

static unsigned long last = millis();
static unsigned long lastChanged = millis();

// notes: setup libraries like this: https://learn.adafruit.com/easy-alexa-or-echo-control-of-your-esp8266-huzzah/software-setup
// after setup and wifi configuration on phone make sure to hard reset the device before alexa discovery
// go to the Alexa app and look for "devices", then attempt to "discover new devices"
// when you have found your named device ("blackjack" in this example)

//device name
const char* deviceName = "blackjack"; 
const char* wifiSetupSSID = "blackjackSSID"; 

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------
 
void wifiSetup() {
    Serial.println("STARTING WIFI SETUP");

    //onboard led off
    digitalWrite(BUILTIN_LED, HIGH);

    WiFiManager wifiManager;
    wifiManager.autoConnect(wifiSetupSSID);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println();
    Serial.println("CONNECTED");
 
    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s, device: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), deviceName);

    // turn LOW to turn onboard LED on when wifi is connected
    digitalWrite(BUILTIN_LED, LOW);
}
 
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); // going HIGH turns off onboard blue led on start, until wifi is connected

  // LED
  pinMode(OUTPUTPIN, OUTPUT);
  digitalWrite(OUTPUTPIN, LOW);

  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);

  // Wifi
  wifiSetup();
 
  // Fauxmo
  fauxmo.addDevice(deviceName);
  Serial.print("device added: ");
  Serial.println(deviceName);
  
  // Gen3 Devices or above
  fauxmo.setPort(80);
 
  // Allow the FauxMo to be discovered
  fauxmo.enable(true);
  
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.println("SetState() called");
    Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
    
    if(device_id==0){
      pinStatus = state;
      if(state){
        digitalWrite(OUTPUTPIN, HIGH);
        lastChanged = millis();
      }else{
        digitalWrite(OUTPUTPIN, LOW);
      }

      // flash onboard led
      digitalWrite(BUILTIN_LED, HIGH);
      delay(200);
      digitalWrite(BUILTIN_LED, LOW);
      delay(200);
      digitalWrite(BUILTIN_LED, HIGH);
      delay(200);
      digitalWrite(BUILTIN_LED, LOW);
      // remove later
      
    }else{
      Serial.println("unknown device");
    }
  });
}
 
void loop() {
  fauxmo.handle();    

  // log messages to serial monitor every 20 seconds to show status of the board
  if (millis() - last > 10000) {
      last = millis();
      Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
      if (WiFi.status() != WL_CONNECTED) {
         // going HIGH turns OFF onboard blue led
        digitalWrite(BUILTIN_LED, HIGH);
        Serial.println("NOT CONNECTED");
        wifiSetup();
      }else{
        // going LOW turns OFF onboard blue led
        digitalWrite(BUILTIN_LED, LOW);
        Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s, device: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), deviceName);
      }
  }

  // reset the pin, since there is only the "on" state coming from the Alexa routine
  if (millis() - lastChanged > 1000 && pinStatus){
    digitalWrite(OUTPUTPIN, LOW);
    pinStatus = false;
  }
}
