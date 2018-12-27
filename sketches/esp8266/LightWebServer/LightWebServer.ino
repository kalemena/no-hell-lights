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

// ===== CONFIGURATION
#include "settings-wifi.h"

#define LED_PIN D2
#define NUM_LEDS 60
#define LED_BRIGHTNESS 90
#define MILLI_AMPS         2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)

#define STATUS_LED 13

#define DEBUG true
#define Serial if(DEBUG)Serial

// ===== SETUP
ESP8266WebServer server(80);

#ifdef ADAFRUIT_NEOPIXEL_H 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);
#else
CRGB leds[NUM_LEDS];
#endif

#include "effects.h"

boolean inProgress = false;
byte selectedEffect=0;
boolean effectChanged = false;

typedef void (*PatternList[])();
// PatternList gPatterns = { eBouncingColoredBalls, eMeteorRain };

void setup(void) {
  Serial.begin(115200);
  
  Serial.println();
  Serial.println("WS2812 initializing");
#ifdef ADAFRUIT_NEOPIXEL_H 
  strip.setBrightness(LED_BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
#else
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
#endif
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
  
  // static files
  //server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
   
  server.onNotFound(handle_NotFound);
  server.begin();
  
  Serial.println("Service initialized");
}

void loop(void) {
  // waiting fo a client
  server.handleClient();

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  // EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically

  if(inProgress == false) {
    // record last effect
    EEPROM.get(0,selectedEffect);
    
    inProgress = true;
    switch(selectedEffect) {
      case 0: eCandle(); break;
      case 1: eFadeInOut(0xff, 0x00, 0x00); eFadeInOut(0xff, 0xff, 0xff); eFadeInOut(0x00, 0x00, 0xff); break;
      case 2: eStrobe(0xff, 0xff, 0xff, 10, 50, 1000); break;
      case 3: eHalloweenEyes(0xff, 0x00, 0x00, 1, 4, true, random(5,50), random(50,150), random(1000, 10000));
              eHalloweenEyes(0xff, 0x00, 0x00, 1, 4, true, random(5,50), random(50,150), random(1000, 10000));
              break;
      case 4: eCylonBounce(0xff, 0x00, 0x00, 4, 10, 50); break;              
      case 5: eNewKITT(0xff, 0x00, 0x00, 8, 10, 50); break;
      case 6: eTwinkle(0xff, 0x00, 0x00, 10, 100, false); break;
      case 7: eTwinkleRandom(20, 100, false); break;
      case 8: eSparkle(0xff, 0xff, 0xff, 0); break;
      case 9: eSnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000)); break;
      case 10: eRunningLights(0xff,0x00,0x00, 50); eRunningLights(0xff,0xff,0xff, 50); eRunningLights(0x00,0x00,0xff, 50); break;
      case 11: eColorWipe(0x00,0xff,0x00, 50); eColorWipe(0x00,0x00,0x00, 50); break;
      case 12: eRainbowCycle(20,80); break;
      case 13: eTheaterChase(false, 0xff,0,0,50); break;
      case 14: eTheaterChase(true, 0,0,0, 50); break;
      case 15: eBouncingColoredBalls(); break;
      case 16: eBouncingColoredBallsMC(); break;
      case 17: eMeteorRain(0xff,0xff,0xff,10, 64, true, 30); break;
      case 18: eJuggle(); break;
      case 19: eBPM(); break;
      case 20: eSinelon(); break;
      case 21: eConfetti(10); break;
      //case 22: eLightning(500); break;
      //case 23: eRing(500); break;
      //case 24: eDrop(500); break;
      case 22: eFire(55,120,15); break;
      case 23: eRGBLoop(); break;
    }
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
  json += " \"heap\":" + String(ESP.getFreeHeap()) + ",\n";
  json += " \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + ",\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}

void handle_SwitchEffect() {
  selectedEffect++;
  if(selectedEffect>26) { 
    selectedEffect=0;
  }
  EEPROM.put(0, selectedEffect);
  //EEPROM.write(0, selectedEffect);
  //EEPROM.commit();
  effectChanged = true;

  String json = "{\n";
  json += " \"effect\":" + String(selectedEffect) + "\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}
