#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
//#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
//#include <iostream>
#include <ArduinoJson.h>

extern "C" {
  #include "user_interface.h"
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define MINVAL(A,B)   (((A) < (B)) ? (A) : (B))
#define MAXVAL(A,B)   (((A) > (B)) ? (A) : (B))
#define DEBUG true
#define Serial if(DEBUG)Serial

// ===== CONFIGURATION

#include "settings.h"

ESP8266WebServer server(80);

#ifdef ADAFRUIT_NEOPIXEL_H 
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);
#else
  CRGB leds[NUM_LEDS];
#endif

#include "effects.h"

uint8_t currentEffectIndex = 0;
boolean inProgress = false;

// ==== List of effects

EffectDetails effectDetails = {
  { eRGBLoop,                 "RGB Loop" },
  { eJuggle,                  "Juggle" },
  { eBPM,                     "BPM" },
  { eSinelon,                 "Sinelon" },
  { eBouncingColoredBalls,    "Bouncing Balls" },
  { eBouncingColoredBallsMC,  "Bouncing Balls Multiple" },
  { eCandle,                  "Candle" },
  { eFadeInOut,               "Fade-in Fade-out" },
  { eStrobe,                  "Strobe" },
  { eHalloweenEyes,           "Halloween Eyes" },
  { eCylonBounce,             "Cylon Bounce" },
  { eNewKITT,                 "New KITT" },
  { eTwinkle,                 "Twinkle" },
  { eTwinkleRandom,           "Twinkle Random" },
  { eSparkle,                 "Sparkle" },
  { eSnowSparkle,             "Sparkle Snow" },
  { eRunningLights,           "Running Lights" },
  { eColorWipe,               "Color Wipe" },
  { eRainbowCycle,            "Rainbow Cycle" },
  { eTheaterChase,            "Theater Chase" },
  { eTheaterChaseRainbow,     "Theater Chase Rainbow" },
  { eMeteorRain,              "Meteor Rain" },
  { eFire,                    "Fire" },
  { eConfetti,                "Confetti" }
  //{ eLightning,             "Lightning" },
  //{ eRing,                  "Ring" },
  //{ eDrop,                  "Drop" },
};

const uint8_t effectDetailsCount = ARRAY_SIZE(effectDetails);

// ==== SETUP and Main loop

void setup(void) {
  Serial.begin(115200);
  
  Serial.println();
  Serial.println("Initializing WS2812 ...");
#ifdef ADAFRUIT_NEOPIXEL_H 
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
#else
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
#endif
  setBrightness(100);
  delay(50);
  
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, 0);

  Serial.println("Initializing EEPROM ...");
  loadSettings();  

  SPIFFS.begin();
  {
    Serial.println("SPIFFS contents:");

    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  Serial.println("WiFi initializing");
  // WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);
  Serial.println();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if(MDNS.begin(host)) {
    Serial.println("MDNS responder started");
  }
  
  Serial.println("Controller initializing");
  server.on("/effects", HTTP_GET, controllerEffects);
  server.on("/status", HTTP_GET, controllerStatus);
  server.on("/settings", HTTP_POST, controllerSettings);
  server.on("/pixels/set", HTTP_POST, controllerPixelsSet);
  server.on("/pixels/reset", HTTP_POST, controllerPixelsReset);
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.onNotFound(exceptionNotFound);
  server.begin();

  Serial.println("Effects:" + String(effectDetailsCount));
  Serial.println("Service initialized");
}

long timeoutAutoplay = millis();
void loop(void) {
  // waiting fo a client
  server.handleClient();

  // POWER OFF
  if(settingsPowerOn == false) {
    setAll(0,0,0); 
    delay(10);
    return;
  }

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  //EVERY_N_SECONDS( settingsAutoplayDuration ) {  }
  long currentTime = millis();
  if(settingsAnimationMode == SETTING_ANIMATION_MODE_AUTOPLAY && (currentTime > timeoutAutoplay)) {
    // reset timeout
    timeoutAutoplay = currentTime + settingsAutoplayDuration * 1000;
    changeEffect();
    return; 
  }
    
  // return to ensure no 2 effects are running at same time
  if(inProgress == true)
    return;

  // Run the ePaint effect (not in list of effects)
  if(settingsAnimationMode == SETTING_ANIMATION_MODE_PAINT) {
    ePaint();
    return;
  }
  
  if(currentEffectIndex != settingsWantedEffectIndex) { 
    currentEffectIndex = settingsWantedEffectIndex;
    Serial.println("Switch to effect " + String(currentEffectIndex) + " " + String(effectDetails[currentEffectIndex].name));
  }

  // run the effect
  inProgress = true;
  effectDetails[currentEffectIndex].effect();
  inProgress = false;
}

boolean tick() {
  loop();

  if(settingsPowerOn == false)
    return true;
  
  if(currentEffectIndex != settingsWantedEffectIndex)
    return true;
  
  return false;
}

// ==== Tools

// ==== Web Controllers

void controllerStatus() {  
  String json = renderStatus("status");
  server.send(200, "application/json", json);
  json = String();
}

void controllerSettings() {
  Serial.println("Ctrl Settings");
  int httpCode = 404;

  if(server.hasArg("power") == true) {
    String value = server.arg("power");
    settingsPowerOn = (value.toInt() == 1);
    saveSettingsPowerOn(settingsPowerOn);
    httpCode = 200;
  }

  if(server.hasArg("brightness") == true) {
    String value = server.arg("brightness");
    setBrightness(value.toInt());
    saveSettingsBrightness(value.toInt());
    httpCode = 200;
  }

  if(server.hasArg("autoplay-duration") == true) {
    String value = server.arg("autoplay-duration");
    settingsAutoplayDuration = value.toInt();
    saveSettingsAutoplayDuration(settingsAutoplayDuration);
    httpCode = 200;
  }

  if(server.hasArg("animation-mode") == true) {
    String value = server.arg("animation-mode");
    Serial.println("Ctrl Pixels: animation-mode=" + value);
    settingsAnimationMode = readAnimiationMode(value);
    saveSettingsAnimationMode(settingsAnimationMode);
    changeEffect();
    httpCode = 200;
  }

  if(server.hasArg("effect") == true) {
    String value = server.arg("effect");
    settingsWantedEffectIndex = value.toInt();
    saveSettingsWantedEffectIndex(settingsWantedEffectIndex);
    httpCode = 200;
  }

  String json = renderStatus("properties updates");
  server.send(httpCode, "application/json", json);
  json = String();
}

void controllerPixelsSet() {
  Serial.println("Ctrl Pixels Set");
  int httpCode = 404;
  
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  if (!root.success()) {
    String errorMsg = "ParseObject() failed: root";
    exceptionParsingJson(errorMsg);
  }

  JsonArray& pixels = root["pixels"];
  if (!pixels.success()) {
    String errorMsg = "ParseObject() failed: pixels";
    exceptionParsingJson(errorMsg);
  }

  for (JsonObject& pixel: pixels) {
    String pIdx = pixel["index"];
    String pColorHexStr = pixel["color"];
    Serial.print("RGB color: "); Serial.println(pColorHexStr);
    long pColorHex = strtol( &pColorHexStr[0], NULL, 16);
    setPixel(pIdx.toInt(), pColorHex);
    showStrip();
  }
  
  /*String pixels = root["pixels"];
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  const char* sensor = root["sensor"];
  long time = root["time"];
  double latitude = root["data"][0];
  double longitude = root["data"][1];
  */
  
  //String json = renderStatus("properties updates");
  String json = "";
  server.send(204, "application/json", json);
  json = String();
}

void controllerPixelsReset() {
  Serial.println("Ctrl Pixels Reset");

  if(server.hasArg("color") == true) {
    String value = server.arg("color");
    long pColorHex = strtol( &value[0], NULL, 16);
    setAll(pColorHex);    
  } else {
    setAll(0,0,0);
  }
  
  String json = "";
  server.send(204, "application/json", json);
  json = String();
}

void controllerEffects() {  
  String json = "{\n";
  json += " \"effects\": " + renderEffects() + "}";
  server.send(200, "application/json", json);
  json = String();
}

void changeEffect() {
  settingsWantedEffectIndex = (settingsWantedEffectIndex+1) % effectDetailsCount;
  saveSettingsWantedEffectIndex(settingsWantedEffectIndex);
}

String renderEffects() {
  String json = "[\n";
  for(int i = 0; i < effectDetailsCount; i++) {
    json += "   { \"id\": " + String(i) + ", \"name\": \"" + effectDetails[i].name + "\"";
    if(i == settingsWantedEffectIndex)
      json += ", \"selected\":true";
    json += " }";
    if(i+1 < effectDetailsCount) json += ",";
    json += "\n";
  }
  json += "  ]\n";
  return json;
}

String renderStatus(String message) {
  String json = "{\n";
  json += " \"system\": {\n";
  json += "   \"heap\": " + String(ESP.getFreeHeap()) + ",\n";
  json += "   \"boot-version\": " + String(ESP.getBootVersion()) + ",\n";
  json += "   \"cpu-frequency\": " + String(system_get_cpu_freq()) + ",\n";
  json += "   \"sdk\": \"" + String(system_get_sdk_version()) + "\",\n";
  json += "   \"chip-id\": " + String(system_get_chip_id()) + ",\n";
  json += "   \"flash-id\": " + String(spi_flash_get_id()) + ",\n";
  json += "   \"flash-size\": " + String(ESP.getFlashChipRealSize()) + ",\n";
  json += "   \"vcc\": " + String(ESP.getVcc()) + ",\n";
  json += "   \"gpio\": " + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + "\n";
  json += "  },\n";
  json += " \"message\": \"" + message + "\",\n";
  json += " \"properties\": {\n";
  json += "   \"power\": " + String(settingsPowerOn) + ",\n";
  String animationMode = renderAnimiationMode(settingsAnimationMode);
  json += "   \"animation-mode\": \"" + animationMode + "\",\n";
  json += "   \"brightness\": " + String(settingsBrightness) + ",\n";
  json += "   \"autoplay-duration\": " + String(settingsAutoplayDuration) + "\n";
  json += "  },\n";
  json += " \"effects\": " + renderEffects();
  json += "}";
  return json;
}

void exceptionParsingJson(String errorMsg) {
  Serial.println();
  String msg = "{\n";
  msg += " \"error\": \"" + errorMsg + "\"\n";
  msg += "}";
  server.send ( 500, "application/json", msg );
}

void exceptionNotFound() {
  digitalWrite(STATUS_LED, 1);
  
  //if(!handleFileRead(server.uri())) {
  String msg = "{\n";
  msg += " \"error\": \"File not found\",\n";
  msg += " \"properties\": {\n";
  msg += "  \"uri\": \"" + server.uri() + "\",\n";
  String methodStr = (server.method() == HTTP_GET) ? "GET" : "POST";
  msg += "  \"method\": \"" + methodStr + "\",\n";
  msg += "  \"arguments\": {\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    msg += "   \"" + server.argName (i) + "\": \"" + server.arg (i) + "\",\n";
    if(i+1 < server.args()) msg += ",";
    msg += "\n";
  }
  msg += "\n  }\n";
  msg += " }\n";
  msg += "}";
  server.send(404, "application/json", msg );
    
  digitalWrite(STATUS_LED, 0);
}
