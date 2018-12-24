#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
//#include <FS.h>

// ===== CONFIGURATION
#include "settings-wifi.h"

#define LED_PIN D2
#define NUM_LEDS 60
#define LED_BRIGHTNESS 90

#define STATUS_LED 13

#define DEBUG true
#define Serial if(DEBUG)Serial

// ==== STATIC

#define SCENE_THEATER_CHASE_RAINBOW "theaterchaserainbow"

// ===== SETUP
ESP8266WebServer server(80);

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

String scene = "color";
boolean inProgress = false;

void setup(void) {
  Serial.begin(115200);
  
  Serial.println();
  Serial.println("WS2812 initializing");
  strip.setBrightness(LED_BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
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
  server.on("/scene", HTTP_GET, handle_Scene);
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

  if(inProgress == false) {
    if(scene == SCENE_THEATER_CHASE_RAINBOW) {
      inProgress = true;
      process_scene_theaterchaserainbow(50);
    }
    inProgress = false;
  }
}

// ==== Tools

// ==== Controllers

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

void handle_Scene() {
  if(!server.hasArg("id")) {
    handle_Root();
    return;
  }

  scene = server.arg("id"); Serial.println("Scene: " + scene);
  
  String json = "{}\n";
  server.send(200, "application/json", json);
  json = String();
}

// ==== Scenes

void process_scene_color(String color){
  Serial.print("Processing request");

  // converting Hex to Int
  int number = (int) strtol( &color[1], NULL, 16);
  
  // splitting into three parts
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  
  // DEBUG
  Serial.print("RGB: ");
  Serial.print(r, DEC);
  Serial.print(" ");
  Serial.print(g, DEC);
  Serial.print(" ");
  Serial.print(b, DEC);
  Serial.println(" ");
  
  // setting whole strip to the given color
  for(int i=0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color( g, r, b ) );
  }
  // init
  strip.show();
  
  Serial.println("on.");
}

//Theatre-style crawling lights with rainbow effect
void process_scene_theaterchaserainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);
      
      loop();
      if(scene != SCENE_THEATER_CHASE_RAINBOW)
        return;

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
