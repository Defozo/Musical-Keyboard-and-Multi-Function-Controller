#include <Arduino.h>
#include <BleKeyboard.h>
#include <TouchHandler.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#define FIRMWARE_VERSION "v1.0.1" // Define the firmware version

BleKeyboard bleKeyboard("MusicalKeyboard"); // Set the Bluetooth device name to "MusicalKeyboard"

#define NUM_TOUCH_PINS 8
const int touchPins[NUM_TOUCH_PINS] = {4, 12, 13, 14, 15, 27, 32, 33};
char keyCodes[NUM_TOUCH_PINS] = {'u', 'o', 'i', 'p', 't', 'a', 'y', 's'};

TouchHandler touchHandler(touchPins, NUM_TOUCH_PINS);

AsyncWebServer server(80);
Preferences preferences;

int debounceDelay;

void setup() {
  Serial.begin(115200);
  bleKeyboard.begin();

  WiFi.softAP("MusicalKeyboard");

  // Read stored settings
  preferences.begin("settings", false);
  for (int i = 0; i < NUM_TOUCH_PINS; i++) {
    String keyCodeKey = "keyCode" + String(i);
    keyCodes[i] = preferences.getChar(keyCodeKey.c_str(), keyCodes[i]);
  }

  // Read and apply TouchHandler settings
  int numInitSamples = preferences.getInt("numInitSamples", 10);
  int samplePeriod = preferences.getInt("samplePeriod", 10);
  int initPeriod = preferences.getInt("initPeriod", 5000);
  int filterPeriod = preferences.getInt("filterPeriod", 60000);
  float sensitivity = preferences.getFloat("sensitivity", 0.9);
  float factor = preferences.getFloat("factor", 1.5);
  float offset = preferences.getFloat("offset", 4.5);
  debounceDelay = preferences.getInt("debounceDelay", 50); // Read DEBOUNCE_DELAY from preferences

  touchHandler.setNumInitSamples(numInitSamples);
  touchHandler.setSamplePeriod(samplePeriod);
  touchHandler.setInitPeriod(initPeriod);
  touchHandler.setFilterPeriod(filterPeriod);
  touchHandler.setSensitivity(sensitivity);
  touchHandler.setFactor(factor);
  touchHandler.setOffset(offset);

  touchHandler.begin();

  // Set up web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>Firmware Version: " + String(FIRMWARE_VERSION) + "</h1>"; // Display the firmware version
    html += "<form action=\"/updateSettings\" method=\"post\">";
    for (int i = 0; i < NUM_TOUCH_PINS; i++) {
      html += "Key code for pin " + String(touchPins[i]) + ": ";
      html += "<input type=\"text\" name=\"keyCode" + String(i) + "\" value=\"" + String(keyCodes[i]) + "\" maxlength=\"1\">";
      html += "<br>";
    }
    html += "<br>";
    html += "Debounce Delay (ms): ";
    html += "<input type=\"number\" name=\"debounceDelay\" value=\"" + String(debounceDelay) + "\">"; // Input for DEBOUNCE_DELAY
    html += "<br>";
    html += "Number of initialization samples: ";
    html += "<input type=\"number\" name=\"numInitSamples\" value=\"" + String(touchHandler.getNumInitSamples()) + "\">";
    html += "<br>";
    html += "Sample period (ms): ";
    html += "<input type=\"number\" name=\"samplePeriod\" value=\"" + String(touchHandler.getSamplePeriod()) + "\">";
    html += "<br>";
    html += "Initialization period (ms): ";
    html += "<input type=\"number\" name=\"initPeriod\" value=\"" + String(touchHandler.getInitPeriod()) + "\">";
    html += "<br>";
    html += "Filter period (ms): ";
    html += "<input type=\"number\" name=\"filterPeriod\" value=\"" + String(touchHandler.getFilterPeriod()) + "\">";
    html += "<br>";
    html += "Sensitivity: ";
    html += "<input type=\"number\" step=\"0.01\" name=\"sensitivity\" min=\"0\" max=\"1\" value=\"" + String(touchHandler.getSensitivity()) + "\">";
    html += "<br>";
    html += "Factor: ";
    html += "<input type=\"number\" step=\"0.01\" name=\"factor\" value=\"" + String(touchHandler.getFactor()) + "\">";
    html += "<br>";
    html += "Offset: ";
    html += "<input type=\"number\" step=\"0.01\" name=\"offset\" value=\"" + String(touchHandler.getOffset()) + "\">";
    html += "<br>";
    html += "<input type=\"submit\" value=\"Update Settings\">";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
  });

  // Add new handler for updating all TouchHandler values and keyCodes
  server.on("/updateSettings", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool updated = false;

    // Update keyCodes
    for (int i = 0; i < NUM_TOUCH_PINS; i++) {
      if (request->hasParam("keyCode" + String(i), true)) {
        keyCodes[i] = request->getParam("keyCode" + String(i), true)->value().charAt(0);
        String keyCodeKey = "keyCode" + String(i);
        preferences.putChar(keyCodeKey.c_str(), keyCodes[i]);
        updated = true;
      }
    }

    // Update TouchHandler settings
    if (request->hasParam("numInitSamples", true)) {
      int numInitSamples = request->getParam("numInitSamples", true)->value().toInt();
      touchHandler.setNumInitSamples(numInitSamples);
      preferences.putInt("numInitSamples", numInitSamples);
      updated = true;
    }
    if (request->hasParam("samplePeriod", true)) {
      int samplePeriod = request->getParam("samplePeriod", true)->value().toInt();
      touchHandler.setSamplePeriod(samplePeriod);
      preferences.putInt("samplePeriod", samplePeriod);
      updated = true;
    }
    if (request->hasParam("initPeriod", true)) {
      int initPeriod = request->getParam("initPeriod", true)->value().toInt();
      touchHandler.setInitPeriod(initPeriod);
      preferences.putInt("initPeriod", initPeriod);
      updated = true;
    }
    if (request->hasParam("filterPeriod", true)) {
      int filterPeriod = request->getParam("filterPeriod", true)->value().toInt();
      touchHandler.setFilterPeriod(filterPeriod);
      preferences.putInt("filterPeriod", filterPeriod);
      updated = true;
    }
    if (request->hasParam("sensitivity", true)) {
      float sensitivity = request->getParam("sensitivity", true)->value().toFloat();
      touchHandler.setSensitivity(sensitivity);
      preferences.putFloat("sensitivity", sensitivity);
      updated = true;
    }
    if (request->hasParam("factor", true)) {
      float factor = request->getParam("factor", true)->value().toFloat();
      touchHandler.setFactor(factor);
      preferences.putFloat("factor", factor);
      updated = true;
    }
    if (request->hasParam("offset", true)) {
      float offset = request->getParam("offset", true)->value().toFloat();
      touchHandler.setOffset(offset);
      preferences.putFloat("offset", offset);
      updated = true;
    }
    if (request->hasParam("debounceDelay", true)) {
      debounceDelay = request->getParam("debounceDelay", true)->value().toInt(); // Update DEBOUNCE_DELAY
      preferences.putInt("debounceDelay", debounceDelay);
      updated = true;
    }

    if (updated) {
      touchHandler.begin();
      request->send(200, "text/html", "Settings updated. <a href=\"/\">Go back</a>");
    } else {
      request->send(400, "text/html", "Invalid request. <a href=\"/\">Go back</a>");
    }
  });

  server.begin();
}

bool touchStates[NUM_TOUCH_PINS] = {false};

unsigned long lastDebounceTime[NUM_TOUCH_PINS] = {0};

void loop() {
  touchHandler.update();

  for (int i = 0; i < NUM_TOUCH_PINS; i++) {
    int touchValue = touchHandler.getTouchValue(i);
    /*Serial.print(">");
    Serial.print("pin");
    Serial.print(i);
    Serial.print(":");
    Serial.println(touchValue);
    Serial.print(">");
    Serial.print("pin");
    Serial.print(i);
    Serial.print("-threshold:");
    Serial.println(touchHandler.getThreshold(i));*/
    bool currentTouchState = touchHandler.isTouched(i);

    // Check if the touch state has changed and if it's past the debounce delay
    if (currentTouchState != touchStates[i] && (millis() - lastDebounceTime[i]) > debounceDelay) {
      // Update the debounce timer
      lastDebounceTime[i] = millis();

      // Update the touch state
      touchStates[i] = currentTouchState;
      
      // If the touch state is true (touched), print the key
      if (currentTouchState) {
        char c = keyCodes[i];
        bleKeyboard.print(c);
        Serial.println("KEY: " + c);
      }
    }
  }
  delay(10);
}
