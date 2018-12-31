#include "settings-wifi.h"

#define LED_PIN         D2
#define NUM_LEDS        60
#define MILLI_AMPS      2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define STATUS_LED      13

boolean settingsPowerOn = true;
uint8_t settingsAutoplayDuration = 10;
uint8_t gBrightness = 100;
