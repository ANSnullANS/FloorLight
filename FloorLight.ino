#include "RTClib.h"
#include <Dusk2Dawn.h>
#include <Adafruit_NeoPixel.h>

// Location-Settings for Graz/Austria
#define TIMEZONE  +1
#define LATITUDE  47.40
#define LONGITUDE 15.26
#define USE_DST   true

// Timing-Settings
#define SENSOR_READ_INTERVAL    100
#define LIGHT_TIMEOUT           30000
#define FADE_TIME               2
#define FADE_IN_TIME            1

// Light-Settings
#define MAX_BRIGHTNESS          240
#define BLUE_OFFSET             130   // Blue Light can interfere with sleep, so better reduce amount of blue...
#define NUMPIXELS     6

// Pin-Settings
#define SENSOR_LEFT   3
#define SENSOR_RIGHT  4
#define LED_LEFT      6
#define LED_RIGHT     7

// Debug-Settings
#define DEBUG         true
#define SET_DATE      false

// Dynamic Variables
unsigned long _Now = 0;
unsigned long _LastUpdate = 0;
unsigned long _LightStart = 0;

bool bLeftTrigger = false;
bool bRightTrigger = false;
bool bLightEnabled = false;
bool bIsDST = false;

int _LastSunUpdate = 0;
int _Sunrise;
int _Sunset;
int _Time;
int iBlue;
int iOffset = 0;

// Hardware/Helper Objects
RTC_DS3231 rtc;

DateTime _Date;

Dusk2Dawn Graz(LATITUDE, LONGITUDE, TIMEZONE);

Adafruit_NeoPixel pixelsLeft(NUMPIXELS, LED_LEFT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsRight(NUMPIXELS, LED_RIGHT, NEO_GRB + NEO_KHZ800);


void setup() {
  // Setup Sensor-Pins
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);

  // Setup Serial only in Debug-Mode
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  // Setup NeoPixels
  pixelsLeft.begin();
  pixelsRight.begin();
  pixelsLeft.clear();
  pixelsRight.clear();
  pixelsLeft.show();
  pixelsRight.show();

  // Setup RTC
  if (! rtc.begin()) {
    #ifdef DEBUG
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    #endif

    setErrorPixel(200,0,0);

    while (1) delay(10);
  }

  // Check if there's a need to set the date
  if (rtc.lostPower() || SET_DATE) {
    #ifdef DEBUG
    Serial.println(F("RTC lost power!"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    #endif

    setErrorPixel(0,200,0);
    delay(3000);
    killErrorPixel();
  }

  calculateDaytimes();
}


void loop() {
  _Now = millis();
  _Date = rtc.now();

  calculateDaytimes();
  
  if (_Now - _LastUpdate >= SENSOR_READ_INTERVAL) {
    #ifdef DEBUG
    Serial.print(_Date.year(), DEC);
    Serial.print("-");
    Serial.print(_Date.month(), DEC);
    Serial.print("-");
    Serial.print(_Date.day(), DEC);
    Serial.print(" ");
    Serial.print(_Date.hour(), DEC);
    Serial.print(":");
    Serial.print(_Date.minute(), DEC);
    Serial.print(":");
    Serial.println(_Date.second(), DEC);
    #endif
  
    updateSensor();
    _LastUpdate = _Now;

    _Time = _Date.hour() * 60 + _Date.minute();

    // Handle Light only during night-time
    if (_Time <= _Sunrise + 60 || _Time >= _Sunset - 60) {
      #ifdef DEBUG
      Serial.println(F("Night"));
      #endif

      if (bLeftTrigger || bRightTrigger) {
        if (bLightEnabled) {
          #ifdef DEBUG
          Serial.println(F("Light EXT"));
          #endif
          _LightStart = _Now;
          updateLight();
        }
        else {
          _LightStart = _Now;
          updateLight();
        }
      }
    }
    else // During day, only fade-out light if needed
    {
      #ifdef DEBUG
      Serial.println(F("Day"));
      #endif

      if (bLightEnabled) {
        fadeOut();
      }
    }
  }
}


void updateLight() {
  if (! bLightEnabled) {
    bLightEnabled = true;
    fadeIn();
    _LightStart = millis();
    return;
  }

  if (_Now - _LightStart >= LIGHT_TIMEOUT) {
    fadeOut();
  }
}


void fadeIn() {
  #ifdef DEBUG
  Serial.println(F("Light ON"));
  #endif

  // Check where to start from
  if (bLeftTrigger) {
    for (int i=NUMPIXELS-1; i>=0; i--) {
      for (int j=0; j<=MAX_BRIGHTNESS; j++) {
        setBlue(j);
        pixelsLeft.setPixelColor(i, pixelsLeft.Color(j, j, iBlue));
        pixelsLeft.show();
        delay(FADE_IN_TIME);
      }
    }
    for (int i=0; i<NUMPIXELS; i++) {
      for (int j=0; j<=MAX_BRIGHTNESS; j++) {
        setBlue(j);
        pixelsRight.setPixelColor(i, pixelsRight.Color(j, j, iBlue));
        pixelsRight.show();
        //delay(FADE_IN_TIME);
      }
    }
  }
  else {
    for (int i=NUMPIXELS-1; i>=0; i--) {
      for (int j=0; j<=MAX_BRIGHTNESS; j++) {
        setBlue(j);
        pixelsRight.setPixelColor(i, pixelsRight.Color(j, j, iBlue));
        pixelsRight.show();
        delay(FADE_IN_TIME);
      }
    }
    for (int i=0; i<NUMPIXELS; i++) {
      for (int j=0; j<=MAX_BRIGHTNESS; j++) {
        setBlue(j);
        pixelsLeft.setPixelColor(i, pixelsLeft.Color(j, j, iBlue));
        pixelsLeft.show();
        //delay(FADE_IN_TIME);
      }
    }
  }
}


int setBlue(int iInput) {
  iBlue = iInput - BLUE_OFFSET;
  if (iBlue < 0) {
    iBlue = 0;
  }
}


void fadeOut() {
  #ifdef DEBUG
  Serial.println(F("Light OFF"));
  #endif

  for (int i=NUMPIXELS-1; i>=0; i--) {
    for (int j=MAX_BRIGHTNESS; j>0; j--) {
      updateSensor();
      if (bLeftTrigger || bRightTrigger) {
        stopFadeOut(i, j);
        _LightStart = millis();
        bLightEnabled = true;
        return;
      }

      setBlue(j);      
      
      pixelsLeft.setPixelColor(i, pixelsLeft.Color(j, j, iBlue));
      pixelsRight.setPixelColor(i, pixelsRight.Color(j, j, iBlue));
      pixelsLeft.show();
      pixelsRight.show();
      delay(FADE_TIME);
    }
  }

  bLightEnabled = false;
}


void stopFadeOut(int iStartPixel, int iStartBrightness) {
  #ifdef DEBUG
  Serial.println(F("Fade-out Cancel"));
  #endif

  for (int i=iStartPixel; i<NUMPIXELS; i++) {
    iOffset = 0;
    if (i == iStartPixel) {
      iOffset = iStartBrightness;
    }
    
    for (int j=iOffset; j<=MAX_BRIGHTNESS; j++) {
      iBlue = setBlue(j);

      pixelsLeft.setPixelColor(i, pixelsLeft.Color(j, j, iBlue));
      pixelsRight.setPixelColor(i, pixelsRight.Color(j, j, iBlue));
      pixelsLeft.show();
      pixelsRight.show();
      delay(FADE_IN_TIME);
    }
  }
}


void calculateDaytimes() {
  if (_LastSunUpdate != _Date.day())
  {
    if (USE_DST) {
      if (_Date <= DateTime(_Date.year(), 10, 30, 01, 00, 00) && _Date >= DateTime(_Date.year(), 3, 26, 1, 0, 0)) {
        bIsDST = true;
      }
      else {
        bIsDST = false;
      }
    }
    
    _Sunrise = Graz.sunrise(_Date.year(), _Date.month(), _Date.day(), bIsDST);
    _Sunset = Graz.sunset(_Date.year(), _Date.month(), _Date.day(), bIsDST);

    #ifdef DEBUG
    Serial.print("Sunrise: ");
    Serial.println(_Sunrise);
    Serial.print("Sunset: ");
    Serial.println(_Sunset);
    #endif

    _LastSunUpdate = _Date.day();
  }
}


void updateSensor() {
  bLeftTrigger = digitalRead(SENSOR_LEFT);
  bRightTrigger = digitalRead(SENSOR_RIGHT);

  #ifdef DEBUG
    if (bLeftTrigger) {
      Serial.print(F("Left: "));
      Serial.println(bLeftTrigger);
    }
      if (bRightTrigger) {
      Serial.print(F("Right: "));
      Serial.println(bRightTrigger);
    }
  #endif
}


void setErrorPixel(int iRed, int iGreen, int iBlue) {
  pixelsLeft.setPixelColor(0, pixelsLeft.Color(iRed, iGreen, iBlue));
  pixelsRight.setPixelColor(0, pixelsRight.Color(iRed, iGreen, iBlue));
  pixelsLeft.show();
  pixelsRight.show();
}


void killErrorPixel() {
  pixelsLeft.setPixelColor(0, pixelsLeft.Color(0, 0, 0));
  pixelsRight.setPixelColor(0, pixelsRight.Color(0, 0, 0));
  pixelsLeft.show();
  pixelsRight.show();
}
