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
uint8_t wantedEffectIndex = 0;
boolean inProgress = false;
boolean effectChanged = false;

enum Animation {
  automatic,
  single,
  scene
} animation;

// ==== List of effects

typedef void (*Effect)();
typedef Effect Effects[];
typedef struct {
  Effect effect;
  String name;
} EffectDetail;
typedef EffectDetail EffectDetails[];

EffectDetails effectDetails = {
  { eRGBLoop,                 "RGB loop" },
  { eJuggle,                  "Juggle" },
  { eBPM,                     "BPM" },
  { eSinelon,                 "Sinelon" },
  { eBouncingColoredBalls,    "Bouncing Colored Balls" },
  { eBouncingColoredBallsMC,  "Bouncing Colored Balls Multi-Color" },
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
  Serial.println("WS2812 initializing");
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
  server.on("/", handle_Root);
  server.on("/switch", HTTP_GET, handle_SwitchEffect);
  // server.on("/status", handle_Status);

  server.on("/set/brightness", HTTP_GET, []() {
    String value = server.arg("value");
    setBrightness(value.toInt());
    handle_Status();
  });
  
  // static files
  //server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
   
  server.onNotFound(handle_NotFound);
  server.begin();

  Serial.println("Effects:" + String(effectDetailsCount));

  animation = automatic;
  Serial.println("Service initialized");
}

void loop(void) {
  // waiting fo a client
  server.handleClient();

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  EVERY_N_SECONDS( 10 ) { wantedEffectIndex = (wantedEffectIndex+1) % effectDetailsCount; }
  
  if(animation == automatic && currentEffectIndex != wantedEffectIndex) { 
    currentEffectIndex = wantedEffectIndex;
    inProgress = false;
    Serial.println("Switch to " + String(effectDetails[currentEffectIndex].name));
  }
  
  if(inProgress == false) {
    inProgress = true;
    EEPROM.get(0,currentEffectIndex);
    effectDetails[currentEffectIndex].effect();
    inProgress = false;
  }
}

boolean tick() {
  loop();
  if(effectChanged) {
    effectChanged = false;
    return true;
  }
  return false;
}

// ==== Tools

// ==== Web Controllers

void handle_Root() {
  Serial.println("Client connected");
  digitalWrite(STATUS_LED, 1);
  
  // data from the colorpicker (e.g. #FF00FF)
  // String color = server.arg("c"); Serial.println("Color: " + color);
  // String scene = server.arg("s"); Serial.println("Scene: " + scene);

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

void handle_Status() {  
  String json = "{\n";
  json += " \"system\": {\n";
  json += "   \"heap\": " + String(ESP.getFreeHeap()) + ",\n";
  json += "   \"gpio\": " + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + "\n";
  json += "  },\n";
  json += " \"properties\": {\n";
  json += "   \"brightness\": " + String(gBrightness) + "\n";
  json += "  }\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}

void changeEffect() {
  currentEffectIndex++;
  if(currentEffectIndex >= effectDetailsCount) { 
    currentEffectIndex = 0;
  }
  wantedEffectIndex = currentEffectIndex;
  EEPROM.put(0, currentEffectIndex);
  //EEPROM.write(0, selectedEffect);
  //EEPROM.commit();
  effectChanged = true;
}

void handle_SwitchEffect() {
  changeEffect();

  String json = "{\n";
  json += " \"effect\":\n {\n";
  json += "  \"index\":" + String(currentEffectIndex) + ",\n";
  json += "  \"name\": \"" + String(effectDetails[currentEffectIndex].name) + "\"\n";
  json += " }\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}
