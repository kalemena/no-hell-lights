#include "settings-wifi.h"

#define LED_PIN         D2
#define NUM_LEDS        60
#define MILLI_AMPS      2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define STATUS_LED      13

#define SETTING_POWER 0
boolean settingsPowerOn = true;

#define SETTING_AUTOPLAY_DURATION 1
uint8_t settingsAutoplayDuration = 10;

#define SETTING_BRIGHTNESS 2
uint8_t settingsBrightness = 100;

#define SETTING_WANTED_EFFECT_INDEX 3
uint8_t settingsWantedEffectIndex = 0;

#define SETTING_ANIMATION_MODE 4
#define SETTING_ANIMATION_MODE_AUTOPLAY 0
#define SETTING_ANIMATION_MODE_SINGLE   1
#define SETTING_ANIMATION_MODE_PAINT    2
uint8_t settingsAnimationMode = SETTING_ANIMATION_MODE_AUTOPLAY;

// Reading JSON values
uint8_t readAnimiationMode(String valueString) {
  Serial.println("Read: animation-mode=" + valueString);
  uint8_t valueAM = SETTING_ANIMATION_MODE_AUTOPLAY;
  if(valueString == "single" || valueString == String(SETTING_ANIMATION_MODE_SINGLE)) {
    valueAM = SETTING_ANIMATION_MODE_SINGLE;
  } else if(valueString == "paint" || valueString == String(SETTING_ANIMATION_MODE_PAINT)) {
    valueAM = SETTING_ANIMATION_MODE_PAINT;
  } else if(valueString == "autoplay" || valueString == String(SETTING_ANIMATION_MODE_AUTOPLAY)) {
    valueAM = SETTING_ANIMATION_MODE_AUTOPLAY;
  } else {
    Serial.println("Read: animation-mode=error");
  }
  return valueAM;
}

// Rendering String value
String renderAnimiationMode(uint8_t valueAM) {
  Serial.println("Render: animation-mode=" + String(valueAM));
  String valueString = "autoplay";
  if(valueAM == SETTING_ANIMATION_MODE_SINGLE) {
    valueString = "single";
  } else if(valueAM == SETTING_ANIMATION_MODE_PAINT) {
    valueString = "paint";
  } else if(valueAM == SETTING_ANIMATION_MODE_AUTOPLAY) {
    valueString = "autoplay";
  } else {
    Serial.println("Render: animation-mode=error");
  }
  return valueString;
}

// Manage EEPROM
void loadSettings() {
  EEPROM.begin(512);
  settingsPowerOn = EEPROM.read(SETTING_POWER);
  Serial.println("  PowerOn = " + String(settingsPowerOn));
  settingsAutoplayDuration = EEPROM.read(SETTING_AUTOPLAY_DURATION);
  Serial.println("  PlayDuration = " + String(settingsAutoplayDuration) );
  settingsBrightness = EEPROM.read(SETTING_BRIGHTNESS);
  Serial.println("  Brightness = " + String(settingsBrightness) );
  settingsAnimationMode = readAnimiationMode(String(EEPROM.read(SETTING_ANIMATION_MODE)));
  Serial.println("  Animation Mode = " + String(settingsAnimationMode) );
  settingsWantedEffectIndex = EEPROM.read(SETTING_WANTED_EFFECT_INDEX);
  Serial.println("  Wanted Effect Index = " + String(settingsWantedEffectIndex) );
}

void saveEEPROM(int idx, uint8_t value) {
  //EEPROM.put(0, value);
  EEPROM.write(idx, value);
  EEPROM.commit();
}

void saveSettingsPowerOn(uint8_t value) {
  saveEEPROM(SETTING_POWER, value);
}

void saveSettingsAutoplayDuration(uint8_t value) {
  saveEEPROM(SETTING_AUTOPLAY_DURATION, value);
}

void saveSettingsBrightness(uint8_t value) {
  saveEEPROM(SETTING_BRIGHTNESS, value);
}

void saveSettingsWantedEffectIndex(uint8_t value) {
  saveEEPROM(SETTING_WANTED_EFFECT_INDEX, value);
}

void saveSettingsAnimationMode(uint8_t value) {
  Serial.println("Save settings: animation-mode=" + String(value));
  saveEEPROM(SETTING_ANIMATION_MODE, value);
}
