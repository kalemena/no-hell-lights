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
      
/*
  Serial.println("SPIFFS initializing");
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  delay(10);
*/ 
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
  server.on("/", controllerRoot);
  server.on("/effects", HTTP_GET, controllerEffects);
  server.on("/status", HTTP_GET, controllerStatus);
  server.on("/settings", HTTP_POST, controllerSettings);
  
  // static files
  //server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
   
  server.onNotFound(handle_NotFound);
  server.begin();

  Serial.println("Effects:" + String(effectDetailsCount));

  settingsAnimationMode = autoplay;
  Serial.println("Service initialized");
}

long timeoutAutoplay = millis();
void loop(void) {
  // waiting fo a client
  server.handleClient();

  if(settingsPowerOn == false) {
    setAll(0,0,0); 
    delay(10);
    return;
  }

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  //EVERY_N_SECONDS( settingsAutoplayDuration ) {  }
  long currentTime = millis();
  if(settingsAnimationMode == autoplay && (currentTime > timeoutAutoplay)) {
    // reset timeout
    timeoutAutoplay = currentTime + settingsAutoplayDuration * 1000;
    changeEffect(); 
  }
    
  // return to ensure no 2 effects are running at same time
  if(inProgress == true)
    return;
  
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

void controllerRoot() {
  Serial.println("Client connected");
  digitalWrite(STATUS_LED, 1);

  // building page
  String result = 
    "<!DOCTYPE HTML> \
     <html>\
      <head>\
       <meta http-equiv=\"Content-type\" content=\"text/html\"; charset=utf-8\">\
       <title>Web relays</title>\
       <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\
      </head>\
      <table>\
       <tr>\
        <th>Description</th>\
        <th>Status</th>\
       </tr>";

  result += "<tr>";
  result += "<td>" + String("Theater Chase Rainbow") + "</td>";
  result += "<td><a href=\"/scene?id=theaterchaserainbow\">Theater Chase Rainbow</button></a></td>";
  result += "</tr>";
  result += "</table>";
  result += "</html>";
  
  server.send(200, "text/html", result);
  // server.send ( 200, "text/html","<SCRIPT language='JavaScript'>window.location='/';</SCRIPT>");
  digitalWrite(STATUS_LED, 0);
}

void handle_NotFound() {
  digitalWrite(STATUS_LED, 1);
  
  //if(!handleFileRead(server.uri())) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
  
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
    }
  
    server.send ( 404, "text/plain", message );
  //}
    
  digitalWrite(STATUS_LED, 0);
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
  String animationMode = "autoplay";
  if(settingsAnimationMode == single) 
    animationMode = "single";
  else if(settingsAnimationMode == scene)
    animationMode = "scene";
  json += "   \"animation-mode\": \"" + animationMode + "\",\n";
  json += "   \"brightness\": " + String(settingsBrightness) + ",\n";
  json += "   \"autoplay-duration\": " + String(settingsAutoplayDuration) + "\n";
  json += "  },\n";
  json += " \"effects\": " + renderEffects();
  json += "}";
  return json;
}

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
    settingsAnimationMode = readAnimiationMode(value);
    httpCode = 200;
  }

  String json = renderStatus("properties updates");
  server.send(httpCode, "application/json", json);
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
