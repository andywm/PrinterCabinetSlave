/*------------------------------------------------------------------------------
    ()      File:   main.cpp
    /\      Copyright (c) 2020 Andrew Woodward-May
   //\\       
  //  \\    Description:
              Printer enclosure sensor suite.
------------------------------
------------------------------
License Text - The MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the 
following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>
#include "pinconfig.h"
#include "utility.h"

#if defined(IS_NANO_BUILD)
// AVR LIBC sprintf is godawful.
#include <printf.h>
#endif //defined(IS_NANO_BUILD)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Forwards
//------------------------------------------------------------------------------
void periodicTasks();
void updateSensor();
void updateLights();
void tempAndVocRender( const char* ptrtolines, int width, int height );

//------------------------------------------------------------------------------
// Enums
//------------------------------------------------------------------------------
struct Enums
{
#if defined(IS_BLUEPILL_BUILD)
  enum I2C_BusEnum { LocalBus, GlobalBus, I2C_MAX_WIRES = 2 };
#elif defined(IS_NANO_BUILD)
  enum I2C_BusEnum { LocalBus, I2C_MAX_WIRES = 1 };
#endif //defined(IS_NANO_BUILD)

  enum DisplayReadoutSlot { DisplayTemp=0, DisplayVOC=1, DisplayVOCSeverity=2, MAX_DISPLAY_SLOT=3 };

  enum Jobs
  {
    Job_None          = 0x0,
    Job_SensorRefresh = 0x1 << 1,
    Job_LightsRefresh = 0x1 << 2,
  }; 
};

//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------
// i2c Busses
#if defined( IS_BLUEPILL_BUILD )
TwoWire g_i2cBus[Enums::I2C_MAX_WIRES] = 
{ 
  TwoWire(1, I2C_FAST_MODE),
  TwoWire(2, I2C_FAST_MODE)
};
#elif defined(IS_NANO_BUILD)
TwoWire g_i2cBus[Enums::I2C_MAX_WIRES] = 
{
  TwoWire()
};
#endif //defined( IS_NANO_BUILD)

//Jobs
int g_jobs = Enums::Job_None;

// Periodic Tasks
Timer g_sensorRefreshTimer( 1000, g_jobs, Enums::Job_SensorRefresh );

//Gas Sensor
Adafruit_BME680 g_gasSensor(&g_i2cBus[Enums::LocalBus]);

//LCD Panel
LiquidCrystal_I2C g_lcd(0x27, 16, 2, LCD_5x8DOTS, g_i2cBus[Enums::LocalBus]);
Display<16,2,Enums::MAX_DISPLAY_SLOT> g_displayHelper(tempAndVocRender);

//State
EnvironmentInfo g_environmentInfo;

//Door LEDs
const unsigned int LED_COUNT = LEDCOUNT;
#if defined( IS_BLUEPILL_BUILD )
WS2812B g_leds = WS2812B(LED_COUNT);
#elif defined(IS_NANO_BUILD)
CRGB g_leds[LED_COUNT];
#endif //defined( IS_NANO_BUILD)


//------------------------------------------------------------------------------
// Arduino Setup Function
//------------------------------------------------------------------------------
void setup() 
{
  g_i2cBus[Enums::LocalBus].begin();
#if defined( IS_BLUEPILLL_BUILD )
  g_i2cBus[Enums::GlobalBus].begin();
#endif //defined( IS_BLUEPILLL_BUILD )
  Serial.begin(9600);

  // Set up oversampling and filter initialization
  g_gasSensor.begin(0x76, true);
  g_gasSensor.setTemperatureOversampling(BME680_OS_8X);
  g_gasSensor.setHumidityOversampling(BME680_OS_2X);
  g_gasSensor.setPressureOversampling(BME680_OS_4X);
  g_gasSensor.setIIRFilterSize(BME680_FILTER_SIZE_3);
  g_gasSensor.setGasHeater(320, 150); // 320*C for 150 ms

  //display
  g_lcd.begin();
	g_lcd.backlight();

  // Bit Alocation. 16 by 2.
  // x - temp, format 4sf, at least 1 decimal.
  // y - air quality, as a percentage of resistance on the BME680
  // z - air quality hint from lookup table.
  // | 00| 01| 02| 03| 04| 05| 06| 07| 08| 09| 10| 11| 12| 13| 14| 15|
  //0| T | e | m | p | _ | x | x | x | x | x | * | C | _ | _ | _ | _ |
  //1| V | O | C | _ | y | y | y | % | _  |z | z | z | z | z | z | z |
  g_displayHelper.reserve(Enums::DisplayTemp, 0, 0, 15);
  g_displayHelper.reserve(Enums::DisplayVOC, 1, 0, 7);
  g_displayHelper.reserve(Enums::DisplayVOCSeverity, 1, 9, 15);
  
  //LEDS
#if defined( IS_BLUEPILL_BUILD )
  g_leds.begin();
  g_leds.show();
#elif defined(IS_NANO_BUILD)
  FastLED.addLeds<NEOPIXEL, LEDPIN>(g_leds, LED_COUNT);
#endif //defined( IS_NANO_BUILD)

  pinMode(DOORPIN, INPUT_PULLUP);
  g_jobs |= Enums::Job_LightsRefresh;
}

//------------------------------------------------------------------------------
// Arduino Loop Function
//------------------------------------------------------------------------------
void loop()
{
  //timers.
  unsigned long runtime = millis();
  g_sensorRefreshTimer.tick( runtime );

  const bool doorOpen = digitalRead(DOORPIN);
  if( g_environmentInfo.doorOpen != doorOpen )
  {
     g_environmentInfo.doorOpen = doorOpen;
     g_jobs |= Enums::Job_LightsRefresh;
  }

  periodicTasks();
}

//------------------------------------------------------------------------------
// For Periodic Tasks, once a timer for a task has elapsed, it raises a flag. 
// The periodic task will then be allowed to process, it must clear the flag to
// stop processing and reset the timer.
//------------------------------------------------------------------------------
void periodicTasks()
{
  //jobs. 

  if( (g_jobs & Enums::Job_LightsRefresh) != 0 )
  {
    updateLights();
    delay(30);
  }

  if( (g_jobs & Enums::Job_SensorRefresh) != 0 )
  {
    updateSensor();
    return;
  }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void updateSensor()
{
  const int readingTime = g_gasSensor.remainingReadingMillis();

  if( readingTime == Adafruit_BME680::reading_not_started )
  {
    g_gasSensor.beginReading();
  }
  else if( readingTime == Adafruit_BME680::reading_complete )
  {
    // update info.
    g_gasSensor.endReading();
    g_environmentInfo.temperature = g_gasSensor.temperature;
    g_environmentInfo.voc = g_gasSensor.gas_resistance;

    char str[16];
    // display temperature.
    sprintf(&str[0], "Temp %5.2f*C", (double)g_gasSensor.temperature );
    g_displayHelper.update(Enums::DisplayTemp, str );

    // display air quality.
    float airQuality = ((float)g_gasSensor.gas_resistance/(float)VOCTable::_table[VOCTable::Good])*100.0f;
    airQuality = Min( airQuality, 100.0f );

    sprintf(&str[0], "VOC %03.0f%%", airQuality );
    g_displayHelper.update(Enums::DisplayVOC, str );

    // display severity hint.
    for( int severity = VOCTable::Good; severity < VOCTable::MAX; severity++ )
    {
      if( g_gasSensor.gas_resistance > VOCTable::_table[severity] )
      {
        g_displayHelper.update(Enums::DisplayVOCSeverity, VOCTable::_asString[severity] );
        break;
      }
    }
    
    g_displayHelper.draw();
    g_jobs &= ~Enums::Job_SensorRefresh;
  }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void updateLights()
{
#if defined(IS_BLUEPILL_BUILD)
  const uint32_t colours[] = { WS2812B::Color(255,0,0), WS2812B::Color(255,255,255) };
  
  for( unsigned int led=0; led<LED_COUNT; led++)
  {
    g_leds.setPixelColor(colours[g_environmentInfo.doorOpen? 0 : 1], led);
  }
  g_leds.show();
#elif defined(IS_NANO_BUILD)
  const CRGB colours[] = { CRGB(255,255,255), CRGB(255,0,0) }; 
  const CRGB& colour = colours[g_environmentInfo.doorOpen? 0 : 1];

  for( unsigned int i=0; i<LED_COUNT; i++ )
  {   
      g_leds[i] = colour;
  }
  FastLED.show(); 
#endif //defined(IS_NANO_BUILD)

  g_jobs &= ~Enums::Job_LightsRefresh;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void tempAndVocRender( const char * ptrtolines, int width, int height )
{
  g_lcd.clear();
  for(int line=0; line<height; line++)
  {
    g_lcd.setCursor(0,line);
    g_lcd.printstr(&ptrtolines[line*(width+1)]);
  }
}