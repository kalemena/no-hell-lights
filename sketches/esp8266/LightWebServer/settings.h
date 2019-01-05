#include "settings-wifi.h"

#define LED_PIN         D2
#define NUM_LEDS        60
#define MILLI_AMPS      2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define STATUS_LED      13

#define SETTING_ANIMATION_MODE 4
enum AnimationMode {
  autoplay,
  single,
  paint
} settingsAnimationMode;

#define SETTING_POWER 0
boolean settingsPowerOn = true;
#define SETTING_AUTOPLAY_DURATION 1
uint8_t settingsAutoplayDuration = 10;
#define SETTING_BRIGHTNESS 2
uint8_t settingsBrightness = 100;
#define SETTING_WANTED_EFFECT_INDEX 3
uint8_t settingsWantedEffectIndex = 0;

// Reading JSON values
AnimationMode readAnimiationMode(String valueString) {
  AnimationMode valueAM = autoplay;
  if(valueString == "single")
    valueAM = single;
  else if(valueString == "paint")
    valueAM = paint;
  else
    valueAM = autoplay;
  return valueAM;
}

// Rendering String value
String writeAnimiationMode(AnimationMode valueAM) {
  String valueString = "autoplay";
  if(valueAM == single)
    valueString == "single";
  else if(valueAM == paint)
    valueString == "paint";
  else
    valueString = "autoplay";
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
