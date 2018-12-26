#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
//#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
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

// ===== SETUP
ESP8266WebServer server(80);

#ifdef ADAFRUIT_NEOPIXEL_H 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);
#else
CRGB leds[NUM_LEDS];
#endif

boolean inProgress = false;
byte selectedEffect=0;
boolean effectChanged = false;

typedef void (*PatternList[])();
// PatternList gPatterns = { eBouncingColoredBalls, eMeteorRain };

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void setup(void) {
  Serial.begin(115200);
  
  Serial.println();
  Serial.println("WS2812 initializing");
#ifdef ADAFRUIT_NEOPIXEL_H 
  strip.setBrightness(LED_BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
#else
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
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
      case 0: eRGBLoop(); break;
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
  effectChanged = true;

  String json = "{\n";
  json += " \"effect\":" + String(selectedEffect) + "\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}

// ==== LED Effect Functions

void eRGBLoop() {
  for(int j = 0; j < 3; j++ ) { 
    // Fade IN
    for(int k = 0; k < 256; k++) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      if(tick()) return;
      delay(3);
    }
    // Fade OUT
    for(int k = 255; k >= 0; k--) { 
      switch(j) { 
        case 0: setAll(k,0,0); break;
        case 1: setAll(0,k,0); break;
        case 2: setAll(0,0,k); break;
      }
      showStrip();
      if(tick()) return;
      delay(3);
    }
  }
}

// simple bouncingBalls not included, since BouncingColoredBalls can perform this as well as shown below
// BouncingColoredBalls - Number of balls, color (red, green, blue) array, continuous
// CAUTION: If set to continuous then this effect will never stop!!! 
void eBouncingColoredBalls() {
  // mimic BouncingBalls
  byte onecolor[1][3] = { {0xff, 0x00, 0x00} };
  BouncingColoredBalls(1, onecolor, false);
}

void eBouncingColoredBallsMC() {
  // multiple colored balls
  byte colors[3][3] = { {0xff, 0x00, 0x00}, 
                        {0xff, 0xff, 0xff}, 
                        {0x00, 0x00, 0xff} };
  BouncingColoredBalls(3, colors, false);
}

void BouncingColoredBalls(int BallCount, byte colors[][3], boolean continuous) {
  float Gravity = -9.81;
  int StartHeight = 1;
  
  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
  boolean ballBouncing[BallCount];
  boolean ballsStillBouncing = true;
  
  for (int i = 0 ; i < BallCount ; i++) {   
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0; 
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i)/pow(BallCount,2);
    ballBouncing[i]=true; 
  }

  while (ballsStillBouncing) {
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
  
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
  
        if ( ImpactVelocity[i] < 0.01 ) {
          if (continuous) {
            ImpactVelocity[i] = ImpactVelocityStart;
          } else {
            ballBouncing[i]=false;
          }
        }
      }
      Position[i] = round( Height[i] * (NUM_LEDS - 1) / StartHeight);
    }

    ballsStillBouncing = false; // assume no balls bouncing
    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i],colors[i][0],colors[i][1],colors[i][2]);
      if ( ballBouncing[i] ) {
        ballsStillBouncing = true;
      }
    }
    
    showStrip();
    if(tick()) return;
    setAll(0,0,0);
  }
}

void eFadeInOut(byte red, byte green, byte blue) {
  float r, g, b;
      
  for(int k = 0; k < 256; k=k+1) { 
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    showStrip();
    if(tick()) return;
  }
     
  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    showStrip();
    if(tick()) return;
  }
}

void eStrobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  for(int j = 0; j < StrobeCount; j++) {
    setAll(red,green,blue);
    showStrip();
    if(tick()) return;
    delay(FlashDelay);
    setAll(0,0,0);
    showStrip();
    if(tick()) return;
    delay(FlashDelay);
  }
 
 delay(EndPause);
}

// HalloweenEyes - Color (red, green, blue), Size of eye, space between eyes, fade (true/false), steps, fade delay, end pause
void eHalloweenEyes(byte red, byte green, byte blue, int EyeWidth, int EyeSpace, boolean Fade, int Steps, int FadeDelay, int EndPause) {
  randomSeed(analogRead(0));
  
  int i;
  int StartPoint  = random( 0, NUM_LEDS - (2*EyeWidth) - EyeSpace );
  int Start2ndEye = StartPoint + EyeWidth + EyeSpace;
  
  for(i = 0; i < EyeWidth; i++) {
    setPixel(StartPoint + i, red, green, blue);
    setPixel(Start2ndEye + i, red, green, blue);
  }
  
  showStrip();
  if(tick()) return;
  
  if(Fade==true) {
    float r, g, b;
  
    for(int j = Steps; j >= 0; j--) {
      r = j*(red/Steps);
      g = j*(green/Steps);
      b = j*(blue/Steps);
      
      for(i = 0; i < EyeWidth; i++) {
        setPixel(StartPoint + i, r, g, b);
        setPixel(Start2ndEye + i, r, g, b);
      }
      
      showStrip();
      if(tick()) return;
      delay(FadeDelay);
    }
  }
  
  setAll(0,0,0); // Set all black
  
  delay(EndPause);
}

// CylonBounce - Color (red, green, blue), eye size, speed delay, end pause
void eCylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void eNewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

// used by NewKITT
void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i =((NUM_LEDS-EyeSize)/2); i>=0; i--) {
    setAll(0,0,0);
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

// used by NewKITT
void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i<=((NUM_LEDS-EyeSize)/2); i++) {
    setAll(0,0,0);
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

// used by NewKITT
void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

// used by NewKITT
void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void eTwinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0,0,0);
  
  for (int i=0; i<Count; i++) {
     setPixel(random(NUM_LEDS),red,green,blue);
     showStrip();
     if(tick()) return;
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAll(0,0,0); 
     }
   }
  
  delay(SpeedDelay);
}

void eTwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0,0,0);
  
  for (int i=0; i<Count; i++) {
     setPixel(random(NUM_LEDS),random(0,255),random(0,255),random(0,255));
     showStrip();
     if(tick()) return;
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAll(0,0,0); 
     }
   }
  
  delay(SpeedDelay);
}

void eSparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,red,green,blue);
  showStrip();
  if(tick()) return;
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
}

void eSnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red,green,blue);
  
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,0xff,0xff,0xff);
  showStrip();
  if(tick()) return;
  delay(SparkleDelay);
  setPixel(Pixel,red,green,blue);
  
  showStrip();
  if(tick()) return;
  delay(SpeedDelay);
}

void eRunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position=0;
  
  for(int i=0; i<NUM_LEDS*2; i++)
  {
      Position++; // = 0; //Position + Rate;
      for(int i=0; i<NUM_LEDS; i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //float level = sin(i+Position) * 127 + 128;
        setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }
      
      showStrip();
      if(tick()) return;
      delay(WaveDelay);
  }
}

void eColorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for(uint16_t i=0; i<NUM_LEDS; i++) {
      setPixel(i, red, green, blue);
      showStrip();
      if(tick()) return;
      delay(SpeedDelay);
  }
}

void addGlitter(int chanceOfGlitter) {
  if( random(255) < chanceOfGlitter) {
    #ifdef ADAFRUIT_NEOPIXEL_H
      // TODO 
    #else
      leds[ random16(NUM_LEDS) ] += CRGB::White;
    #endif
  }
}

void eRainbowCycle(int SpeedDelay, int chanceOfGlitter) {
  byte *c;
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) {
      c=Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    if(chanceOfGlitter > 0) 
      addGlitter(chanceOfGlitter);
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
}

// used by rainbowCycle and theaterChaseRainbow
byte * Wheel(byte WheelPos) {
  static byte c[3];
  
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }

  return c;
}

void eTheaterChase(boolean rainbow, byte red, byte green, byte blue, int SpeedDelay) {
  byte *c;
  
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < NUM_LEDS; i=i+3) {
          if(rainbow) {
            c = Wheel( (i+j) % 255);
            setPixel(i+q, *c, *(c+1), *(c+2)); //turn every third pixel on
          } else {
            setPixel(i+q, red, green, blue); //turn every third pixel on
          }
        }
        showStrip();
        if(tick()) return;
       
        delay(SpeedDelay);
       
        for (int i=0; i < NUM_LEDS; i=i+3) {
          setPixel(i+q, 0,0,0); //turn every third pixel off
        }
    }
  }
}

// Fire - Cooling rate, Sparking rate, speed delay
void eFire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  if(tick()) return;
  delay(SpeedDelay);
}

void setPixelHeatColor(int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void eMeteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
  
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
    
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      } 
    }
   
    showStrip();
    if(tick()) return;
    delay(SpeedDelay);
  }
}

// used by meteorrain
void fadeToBlack(int ledNo, byte fadeValue) {
 #ifdef ADAFRUIT_NEOPIXEL_H 
    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b;
    int value;
    
    oldColor = strip.getPixelColor(ledNo);
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
    
    strip.setPixelColor(ledNo, r,g,b);
 #else
   // FastLED
   leds[ledNo].fadeToBlackBy( fadeValue );
 #endif  
}

void eConfetti(int SpeedDelay) {
  while(true) {
    // random colored speckles that blink in and fade smoothly
 #ifdef ADAFRUIT_NEOPIXEL_H
    // TODO 
 #else
    fadeToBlackBy( leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
 #endif
    delay(SpeedDelay);
    showStrip();
    if(tick()) return;
  }
}

void eSinelon() {
  while(true) {
    // a colored dot sweeping back and forth, with fading trails
 #ifdef ADAFRUIT_NEOPIXEL_H
    // TODO 
 #else
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16( 13, 0, NUM_LEDS-1 );
    leds[pos] += CHSV( gHue, 255, 192);
 #endif
    showStrip();
    if(tick()) return;
  }
}

void eBPM() {
  while(true) {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
 #ifdef ADAFRUIT_NEOPIXEL_H
    // TODO 
 #else
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
 #endif
    showStrip();
    if(tick()) return;
  }
}

void eJuggle() {
  while(true) {
    // eight colored dots, weaving in and out of sync with each other
#ifdef ADAFRUIT_NEOPIXEL_H
    for(int j=0; j<NUM_LEDS; j++) {
      fadeToBlack(j, 20);
    }
    // TODO
#else
    fadeToBlackBy( leds, NUM_LEDS, 20);
    byte dothue = 0;
    for( int i = 0; i < 8; i++) {
      leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
#endif  
    showStrip();
    if(tick()) return;
  }
}
/*
void eDrop(uint8_t wait) {
  while(true) {
    uint16_t i, j;
  
    for(j=0; j<50; j++) {
      int start = random(0, NUM_LEDS);
      int direction = (random(0,10) < 5) ? -1 : 1;
      int length = random(5, 30);
      
      for(i=0; i<length+10; i++) {
        if(i < length) setPixel(i*direction+start, random(0,4)*60, random(0,4)*60, random(0,4)*60);
        setPixel((i-10)*direction+start, 0, 0, 0);
        showStrip();
        if(tick()) return;
        delay(1);
      }
     
      delay(wait);
    }
  }
}

void eRing(uint8_t wait) {
  while(true) {
    uint16_t i, j;
  
    for(j=0; j<2; j++) {
      int start = 300;
      for(i=0; i<=24; i++) {
        setPixel(i+start, 0,0,127);
        setPixel((i-1)+start, 0, 0, 0);
        showStrip();
        if(tick()) return;
        delay(50);
      }   
      delay(wait);
    }
  }
}

void eLightning(uint8_t wait) {
  while(true) {
    uint16_t i, j;
  
    for(j=0; j<3; j++) {
      
      int direction = 1; //(random(0,10) < 5) ? -1 : 1;
      int start = (direction==-1) ? NUM_LEDS : 0;
      
      //int color = strip.Color(random(0,4)*60, random(0,4)*60, random(0,4)*60);
      
      for(i=0; i < NUM_LEDS; i=i+10) {
        for(int j=0; j <10; j++) {
          setPixel((i+j)*direction+start, 0,0,127);
          setPixel(((i+j)-10)*direction+start, 0, 0, 0);
        }
        showStrip();
        if(tick()) return;
        //delay(1);
      }
     
      delay(wait);
    }
  }
}
*/
// ***************************************
// ** FastLed/NeoPixel Common Functions **
// ***************************************

// Apply LED color changes
void showStrip() {
#ifdef ADAFRUIT_NEOPIXEL_H 
  // NeoPixel
  strip.show();
#else
  // FastLED
  FastLED.show();
#endif
}

// Set a LED color (not yet visible)
void setPixel(int Pixel, byte red, byte green, byte blue) {
#ifdef ADAFRUIT_NEOPIXEL_H 
  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
#else
  // FastLED
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
#endif
}

// Set all LEDs to a given color and apply it (visible)
void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  showStrip();
}
