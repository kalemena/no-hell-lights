typedef void (*Effect)();
typedef Effect Effects[];
typedef struct {
  Effect effect;
  String name;
} EffectDetail;
typedef EffectDetail EffectDetails[];

extern boolean tick(); // let some time for other threads
extern EffectDetails effectDetails;
extern uint8_t currentEffectIndex;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// ***************************************
// ** FastLed/NeoPixel Common Functions **
// ***************************************

void setBrightness(uint8_t value) {
  if (value > 255) value = 255;
  if (value < 0) value = 0;

  settingsBrightness = value;
#ifdef ADAFRUIT_NEOPIXEL_H
  strip.setBrightness(settingsBrightness);
#else
  FastLED.setBrightness(settingsBrightness);
#endif

  EEPROM.write(0, settingsBrightness);
  EEPROM.commit();
}

// Apply LED color changes
void showStrip() {
#ifdef ADAFRUIT_NEOPIXEL_H 
  strip.show();   // NeoPixel
#else
  FastLED.show(); // FastLED
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

void setPixel(int Pixel, long hexColor) {
  long r = hexColor >> 16;
  long g = hexColor >> 8 & 0xFF;
  long b = hexColor & 0xFF;
  setPixel(Pixel, r, g, b);
}

// Set all LEDs to a given color and apply it (visible)
void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  showStrip();
}

// ***************************************
// ** FastLed/NeoPixel Effects          **
// ***************************************

// Paint as requested
void ePaint() {
  // TODO
}

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

void efFadeInOut(byte red, byte green, byte blue) {
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

void eFadeInOut() {
  efFadeInOut(0xff, 0x00, 0x00); efFadeInOut(0xff, 0xff, 0xff); efFadeInOut(0x00, 0x00, 0xff);  
}

void efStrobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
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

void eStrobe() {
  efStrobe(0xff, 0xff, 0xff, 10, 50, 1000);  
}

// HalloweenEyes - Color (red, green, blue), Size of eye, space between eyes, fade (true/false), steps, fade delay, end pause
void efHalloweenEyes(byte red, byte green, byte blue, int EyeWidth, int EyeSpace, boolean Fade, int Steps, int FadeDelay, int EndPause) {
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

void eHalloweenEyes() {
  efHalloweenEyes(0xff, 0x00, 0x00, 1, 4, true, random(5,50), random(50,150), random(1000, 10000));
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

// CylonBounce - Color (red, green, blue), eye size, speed delay, end pause
void efCylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void eCylonBounce() {
  efCylonBounce(0xff, 0x00, 0x00, 4, 10, 50);
}

void efNewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void eNewKITT() {
  efNewKITT(0xff, 0x00, 0x00, 8, 10, 50);
}

void efTwinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
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

void eTwinkle() {
  efTwinkle(0xff, 0x00, 0x00, 10, 100, false);
}

void efTwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
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

void eTwinkleRandom() {
  efTwinkleRandom(20, 100, false);
}

void efSparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = random(NUM_LEDS);
  setPixel(Pixel,red,green,blue);
  showStrip();
  if(tick()) return;
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
}

void eSparkle() {
  efSparkle(0xff, 0xff, 0xff, 0);
}

void efSnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
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

void eSnowSparkle() {
  efSnowSparkle(0x10, 0x10, 0x10, 20, random(100,1000));
}

void efRunningLights(byte red, byte green, byte blue, int WaveDelay) {
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

void eRunningLights() {
  efRunningLights(0xff,0x00,0x00, 50); 
  efRunningLights(0xff,0xff,0xff, 50); 
  efRunningLights(0x00,0x00,0xff, 50);
}

void efColorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for(uint16_t i=0; i<NUM_LEDS; i++) {
      setPixel(i, red, green, blue);
      showStrip();
      if(tick()) return;
      delay(SpeedDelay);
  }
}

void eColorWipe() {
  efColorWipe(0x00,0xff,0x00, 50); 
  efColorWipe(0x00,0x00,0x00, 50);
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

void efRainbowCycle(int SpeedDelay, int chanceOfGlitter) {
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

void eRainbowCycle() {
  efRainbowCycle(20,80);
}

void efTheaterChase(boolean rainbow, byte red, byte green, byte blue, int SpeedDelay) {
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

void eTheaterChase() {
  efTheaterChase(false, 0xff,0,0,50);
}

void eTheaterChaseRainbow() {
  efTheaterChase(true, 0xff,0,0,50);
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

// Fire - Cooling rate, Sparking rate, speed delay
void efFire(int Cooling, int Sparking, int SpeedDelay) {
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

void eFire() {
  efFire(55,120,15);
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

void efMeteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
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

void eMeteorRain() {
  efMeteorRain(0xff,0xff,0xff,10, 64, true, 30);
}

void efConfetti(int SpeedDelay) {
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

void eConfetti() {
  efConfetti(10);
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
void eDrop() {
  efDrop(500);
}

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

// The LED can be in only one of these states at any given time
#define BRIGHT                  0
#define UP                      1
#define DOWN                    2
#define DIM                     3
#define BRIGHT_HOLD             4
#define DIM_HOLD                5

#define INDEX_BOTTOM_PERCENT    10 // Percent chance the LED will suddenly fall to minimum brightness
#define INDEX_BOTTOM            128// Absolute minimum red value (green value is a function of red's value)
#define INDEX_MIN               192// Minimum red value during "normal" flickering (not a dramatic change)
#define INDEX_MAX               255// Maximum red value
#define DOWN_MIN_MSECS          20 // Decreasing brightness will take place over a number of milliseconds in this range
#define DOWN_MAX_MSECS          250
#define UP_MIN_MSECS            20 // Increasing brightness will take place over a number of milliseconds in this range
#define UP_MAX_MSECS            250
#define BRIGHT_HOLD_PERCENT     20 // Percent chance the color will hold unchanged after brightening
#define BRIGHT_HOLD_MIN_MSECS   0  // When holding after brightening, hold for a number of milliseconds in this range
#define BRIGHT_HOLD_MAX_MSECS   100
#define DIM_HOLD_PERCENT        5  // Percent chance the color will hold unchanged after dimming
#define DIM_HOLD_MIN_MSECS      0  // When holding after dimming, hold for a number of milliseconds in this range
#define DIM_HOLD_MAX_MSECS      50
 
#define CANDLEPIXELS 60

byte state, index_start, index_end;
unsigned long flicker_msecs;
unsigned long flicker_start;

void set_color(byte index) {
  int i; 
  index = MAXVAL(MINVAL(index, INDEX_MAX), INDEX_BOTTOM);
  if (index >= INDEX_MIN) {
    for(i=0;i<CANDLEPIXELS;i++) {
      setPixel(i, index, (index * 3) / 8, 0);      
    }
  } else if (index < INDEX_MIN) {
    for(i=0;i<CANDLEPIXELS;i++) {
      setPixel(i, index, (index * 3.25) / 8, 0);
    }
  } 
  return;
}

void eCandle() {
  while(true) {
    unsigned long current_time; 
    current_time = millis();
  
    switch(state) {
      case BRIGHT:   
        flicker_msecs = random(DOWN_MAX_MSECS - DOWN_MIN_MSECS) + DOWN_MIN_MSECS;
        flicker_start = current_time;
        index_start = index_end;
        if ((index_start > INDEX_BOTTOM) && (random(100) < INDEX_BOTTOM_PERCENT))
          index_end = random(index_start - INDEX_BOTTOM) + INDEX_BOTTOM;
        else
          index_end = random(index_start - INDEX_MIN) + INDEX_MIN;
        state = DOWN;
        break;  
      
      case DIM:
        flicker_msecs = random(UP_MAX_MSECS - UP_MIN_MSECS) + UP_MIN_MSECS;
        flicker_start = current_time;
        index_start = index_end;
        index_end = random(INDEX_MAX - index_start) + INDEX_MIN;
        state = UP;
        break;
    
      case BRIGHT_HOLD:  
      case DIM_HOLD:
         if (current_time >= (flicker_start + flicker_msecs))
          state = (state == BRIGHT_HOLD) ? BRIGHT : DIM; 
         break;
   
      case UP:
      case DOWN:
        if (current_time < (flicker_start + flicker_msecs)) {
          set_color(index_start + ((index_end - index_start) * (((current_time - flicker_start) * 1.0) / flicker_msecs)));
          break;
        }
       
        set_color(index_end);
        if (state == DOWN) {
          if (random(100) < DIM_HOLD_PERCENT) {
            flicker_start = current_time;
            flicker_msecs = random(DIM_HOLD_MAX_MSECS - DIM_HOLD_MIN_MSECS) + DIM_HOLD_MIN_MSECS;
            state = DIM_HOLD;
          } else {
            state = DIM;
          }
        } else {
          if (random(100) < BRIGHT_HOLD_PERCENT) {
            flicker_start = current_time;
            flicker_msecs = random(BRIGHT_HOLD_MAX_MSECS - BRIGHT_HOLD_MIN_MSECS) + BRIGHT_HOLD_MIN_MSECS;
            state = BRIGHT_HOLD;
          } else {
            state = BRIGHT;
          }
        }
        break; 
    }
    showStrip();
    if(tick()) return;
  }
}
