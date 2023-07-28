// Clock using a seven segment display & NTP clock synchronisation.
//
// Designed specifically to work with the Adafruit LED 7-Segment backpacks
//
// ----> http://www.adafruit.com/products/881
// ----> http://www.adafruit.com/products/880
// ----> http://www.adafruit.com/products/879
// ----> http://www.adafruit.com/products/878
//
//
// original code written by Tony DiCola for Adafruit Industries.
// Released under a MIT license: https://opensource.org/licenses/MIT
// Significant modifications by Myk Dowling for specific personal use
//
// - Flawed gmtoffset method for setting time and daylight savings
//   replaced with proper time zone configuration
// - Implements a couple of OpenHAB REST API functions to get and set
//   the state of OpenHAB items
// - Main loop rewritten to watch a proximity sensor for gesture input
//   and trigger clock update every second
// - Proximity sensor used to toggle lights, utilising existing OpenHAB
//   rules for scene handling
// - Reads OpenHAB items generated from calendar items to generate
//   false urgency in the two hours before work shifts by putting the
//   clock forward by ten minutes using a fake timezone configuration
// - Corrected case for openhab3 item case normalisation
// - Added rest api logon details

#ifndef ESP32
#error This code is designed to run on ESP32 platform, not Arduino nor ESP8266! Please check your Tools->Board setting.
#endif

#include <set.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <ArduinoJson.h>

// Define some numbers to match hardware selections
#define IR_PIN 34
#define BRIGHT 8
#define DIM 0
#define REBOOT_TIME 43200000  // 12 hours

// WiFi credentials
const char* ssid = "network";
const char* password =  "network_password";

// HTTP CLient
HTTPClient http;

// Define NTP Client to get time
const char* ntpServer = "edge.tangonet";
const char* sydneyTZ = "AEST-10AEDT-11,M10.1.0/02:00:00,M4.1.0/03:00:00";
const char* urgentTZ = "AEST-10:10AEDT-11:10,M10.1.0/02:00:00,M4.1.0/03:00:00";

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      true

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

// Create display object.  This is a global variable that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();
int clockBrightness = 0;

//Create timeinfo object
struct tm timeinfo;

// Keep track of the hours, minutes, seconds displayed by the clock.
// Start off at 0:00:00 as a signal that the time should be read from
// NTP to initialise it
int countHours = 0;
int countMinutes = 0;
int countSeconds = 0;

// variables used to track the current millis() values in order to
// trigger clock update every second
unsigned long startMillis = 0;
unsigned long currentMillis = 0;

//OpenHAB REST constants
const char* OFF = "OFF";
const char* ON = "ON";
const char* SWITCHED_OFF = ",0";
const char* REST_USER = "user";
const char* REST_PASS = "password";
const char* isDark = "http://openhab.example:8080/rest/items/Is_Dark";
const char* workDay = "http://openhab.example:8080/rest/items/Is_Work_Day";
const char* workNight = "http://openhab.example:8080/rest/items/Is_Work_Night";
const char* showerLight = "http://openhab.example:8080/rest/items/Light_object";
const char* lightingScene = "http://openhab.example:8080/rest/items/Lighting_Scene";
const char* ensuiteOn = "ensuite_on";
const char* ensuiteOff = "ensuite_off";
Set morningHours;
Set eveningHours;
//JSON variable used to parse OpenHAB results
DynamicJsonDocument doc(1024);

// Wave detection variables
int sensorValue;
unsigned long closeDetected = 0;
int waveTime = 0;

// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = false;

void setup() {
  // Setup function runs once at startup to initialize the display
  // and Wifi connection
  // Start connection to the Serial output and Wifi network
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Clock starting!");
  Serial.print("System will reboot every  ");
  Serial.print(REBOOT_TIME);
  Serial.println(" milliseconds.");
  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);
  clockBrightness = BRIGHT;
  clockDisplay.setBrightness(clockBrightness);

  // Set system timezone
  configTime(0, 0, ntpServer);
  setenv("TZ", sydneyTZ, 1);
  tzset();

  // Loop until wifi connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }


  // Set hours in which to generate artificial ugency
  // Uncomment next line to test code - will trigger at startup
  //morningHours.add(0);
  morningHours.add(5);
  morningHours.add(6);
  eveningHours.add(17);
  eveningHours.add(18);

  // Set up IR distance sensor
  pinMode(IR_PIN, INPUT);

}

void getItem(const char* &testUrl) {
  http.begin(testUrl);
  Serial.print("Get state from ");
  Serial.print(testUrl);
  http.addHeader("UserID", REST_USER);
  http.addHeader("Password", REST_PASS);
  int httpCode = http.GET();

  if (httpCode > 0) { //Check for the returning code

    String payload = http.getString();
    // Serial.println(httpCode);
    // Serial.println(payload);

    deserializeJson(doc, payload);
    const char* state = doc["state"];
    Serial.print(": ");
    Serial.println(state);
  }
  else {
    Serial.println("Error on HTTP request");
  }
}

void setItem(const char* &itemUrl, const char* &payload) {
  Serial.print("Set state for ");
  Serial.print(itemUrl);
  Serial.print(" to ");
  Serial.println(payload);
  http.begin(itemUrl);
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("UserID", REST_USER);
  http.addHeader("Password", REST_PASS);
  int httpCode = http.POST(payload);

  if (httpCode > 0) { //Check for the returning code

    String habResponse = http.getString();
    Serial.print(" : ");
    Serial.print(httpCode);
    Serial.print(" : ");
    Serial.println(habResponse);
  }
  else {
    Serial.println("Error on HTTP request");
  }
}
void printLocalTime() {
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S %Z");
}

bool lightsAreOff() {
  Serial.println("Are the lights off?");
  getItem(showerLight);
  String state = doc["state"];
  return state.substring(state.length() - 2) == SWITCHED_OFF;
}

bool isItDark() {
  Serial.println("Is it dark?");
  getItem(isDark);
  return doc["state"] == ON;
}

void setBrightness() {
  // Check OpenHAB for Is_Dark state and set brightness
  // TODO: also check light status and use full brightness when lights are on

  bool dark = isItDark();
  bool wantDim = dark;
  if ( dark) {
    wantDim = lightsAreOff();
  }
  if ( wantDim ) {
    Serial.println("It's dark and the lights are off");
  }
  bool dim = ( clockBrightness == DIM );

  if ( wantDim and !dim ) {
    Serial.println("Set brightness low");
    clockBrightness = DIM;
    clockDisplay.setBrightness(clockBrightness);
  }
  else if ( !wantDim and dim ) {
    Serial.println("Set brightness high");
    clockBrightness = BRIGHT;
    clockDisplay.setBrightness(clockBrightness);
  }

}

void fakeUrgency(const char* &testUrl) {
  //Check Openhab for Is_work_day and generate false urgency by setting the time ahead 10 minutes
  Serial.println("Is it Work?");

  getItem(testUrl);
  if (doc["state"] == ON) {
    generateUrgency();
  }
  else {
    noUrgency();
  }
}

void generateUrgency() {
  Serial.println("Generate urgency");
  setenv("TZ", urgentTZ, 1);
  tzset();
}

void noUrgency() {
  Serial.println("No urgency required");
  setenv("TZ", sydneyTZ, 1);
  tzset();
}

void checkNTP() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }
  printLocalTime();
  // Now set the hours and minutes.
  countHours = timeinfo.tm_hour;
  countMinutes = timeinfo.tm_min;
  countSeconds = timeinfo.tm_sec;
}

void displayTime() {
  // Show the time on the display by turning it into a numeric
  // value, like 3:30 turns into 330, by multiplying the hour by
  // 100 and then adding the minutes.
  int displayValue = countHours * 100 + countMinutes;
  Serial.print(millis());
  Serial.print(" - ");
  Serial.println(displayValue);

  // Do 24 hour to 12 hour format conversion when required.
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (countHours > 12) {
      displayValue -= 1200;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (countHours == 0) {
      displayValue += 1200;
    }
  }

  // Now print the time value to the display.
  clockDisplay.print(displayValue, DEC);
  //clockDisplay.print(displayString);

  // Add zero padding when in 24 hour mode and it's before 10:00
  // In this case the print function above won't have leading 0's
  // which can look confusing.  Go in and explicitly add these zeros.
  if (TIME_24_HOUR && countHours < 10) {
    // Pad hours.
    clockDisplay.writeDigitNum(0, 0);
    // Add extra zero padding when the hour is midnight, since the print
    // function will be missing even more digits.
    if (countHours == 0) {
      clockDisplay.writeDigitNum(1, 0);
      // Also pad when the 10's minute is 0 and should be padded.
      if (countMinutes < 10) {
        clockDisplay.writeDigitNum(3, 0);
      }
    }

  }

  // Blink the colon by flipping its value every loop iteration
  // (which happens every second).
  blinkColon = !blinkColon;
  clockDisplay.drawColon(blinkColon);

  // Now push out to the display the new values that were set above.
  clockDisplay.writeDisplay();
}

void addASecond() {
  countSeconds += 1;
  // If the seconds go above 59 then the minutes should increase and
  // the seconds should wrap back to 0. (We'll also be doing an NTP up
  if (countSeconds > 59) {
    countSeconds = 0;
    countMinutes += 1;
    // Again if the minutes go above 59 then the hour should increase and
    // the minutes should wrap back to 0.
    if (countMinutes > 59) {
      countMinutes = 0;
      countHours += 1;
      // Note that when the minutes are 0 (i.e. it's the top of a new hour)
      // then the start of the loop will read the actual time from NTP
      // again.  Just to be safe though we'll also increment the hour and wrap
      // back to 0 if it goes above 23 (i.e. past midnight).
      if (countHours > 23) {
        countHours = 0;
      }
    }
  }
}

void updateDisplay() {
  // Triggered by timer interrupt every 1000 milliseconds
  Serial.print(".");
  // Get the current millis value for the main loop
  startMillis = millis();
  // At start of minute, check for Is_Dark and set brightness
  if  (countSeconds == 0) {
    // Check condition of sun and lighting, set screen brightness accordingly
    setBrightness();
    //Check NTP for an accurate time every minute
    checkNTP();
    // Check if it's the top of the hour
    if  (countMinutes == 0) {
      // If it's between 5 or 6 AM or PM, check to see if it's a work day
      // or night and generate fake urgency by offsetting the time by 10 minutes

      if (morningHours.has(countHours)) {
        fakeUrgency(workDay);
      }
      else if (eveningHours.has(countHours)) {
        fakeUrgency(workNight);
      }
      else {
        noUrgency();
      }
    }
  }
  // Refresh display (need to do every second if blinkColon is set)
  displayTime();
  // Now increase the seconds by one (also increases minutes and hours if necessary).
  addASecond();
}

void toggleLights() {
  if (lightsAreOff()) {
    setItem(lightingScene, ensuiteOn);
  }
  else {
    setItem(lightingScene, ensuiteOff);
  }
}

void handleWave () {
  if (countHours * 10000 + countMinutes * 100 + countSeconds > waveTime + 3) {
    waveTime = countHours * 10000 + countMinutes * 100 + countSeconds;
    Serial.println("*******************Do wave stuff***********************");
    toggleLights();
  }
}

void loop() {
  // Loop function runs over and over again to check the IR distance sensor
  currentMillis = millis();
  if ( currentMillis > REBOOT_TIME ) {
    Serial.println("Performing hard reset");
    ESP.restart();
  }
  if ( (currentMillis > startMillis + 1000) || (currentMillis < startMillis) ) {
    updateDisplay();
  }

  sensorValue = analogRead(IR_PIN);
  //Serial.print(currentMillis - startMillis);
  //Serial.print(" : ");
  //Serial.println(sensorValue);
  if (sensorValue > 2000) {
    if (closeDetected == 0) {
      closeDetected = currentMillis;
      Serial.println("*******Something*****");
    }
    else if (currentMillis - closeDetected > 300) {
      Serial.println("***************Too Long!**********");
    }
  }
  else if (closeDetected != 0) {
    if ((currentMillis - closeDetected > 0 ) && (currentMillis - closeDetected < 300 )) {
      closeDetected = 0;
      Serial.println("*******************Wave detected***********************");
      handleWave();
      delay(300);
    }
    else {
      closeDetected = 0;
      //Serial.println("*Nothing*");
    }
  }
  else {
    closeDetected = 0;
    //Serial.println("*Nothing*");
  }

  delay(5);

  // Loop code is finished, it will jump back to the start of the loop
  // function again!
}
