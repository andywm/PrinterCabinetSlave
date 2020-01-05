/*------------------------------------------------------------------------------
    ()      File:   main.cpp
    /\      Copyright (c) 2020 Andrew Woodward-May - See legal.txt
   //\\       
  //  \\    Description:
              Printer enclosure sensor suite.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <LiquidCrystal_I2C.h>
#include "utility.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Forwards
//------------------------------------------------------------------------------
void periodicTasks();
void updateSensor();
void updateDoor();
void tempAndVocRender( const char* ptrtolines, int width, int height );

//------------------------------------------------------------------------------
// Enums
//------------------------------------------------------------------------------
struct Enums
{
  enum I2C_BusEnum { LocalBus, GlobalBus, I2C_MAX_WIRES = 2 };
  enum DisplayReadoutSlot { DisplayTemp=0, DisplayVOC=1, DisplayVOCSeverity=2, MAX_DISPLAY_SLOT=3 };

  enum Jobs
  {
    Job_None          = 0,
    Job_SensorRefresh = 0x1 << 1,
  };
};

//------------------------------------------------------------------------------
// Global
//------------------------------------------------------------------------------
// i2c Busses
TwoWire g_i2cBus[Enums::I2C_MAX_WIRES] = 
{ 
  TwoWire(1, I2C_FAST_MODE),
  TwoWire(2, I2C_FAST_MODE)
};

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

//------------------------------------------------------------------------------
// Arduino Setup Function
//------------------------------------------------------------------------------
void setup() 
{
  g_i2cBus[Enums::LocalBus].begin();
  g_i2cBus[Enums::GlobalBus].begin();
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
}

//------------------------------------------------------------------------------
// Arduino Loop Function
//------------------------------------------------------------------------------
void loop()
{
  periodicTasks();
}

//------------------------------------------------------------------------------
// For Periodic Tasks, once a timer for a task has elapsed, it raises a flag. 
// The periodic task will then be allowed to process, it must clear the flag to
// stop processing and reset the timer.
//------------------------------------------------------------------------------
void periodicTasks()
{
  //timers.
  unsigned long runtime = millis();
  if( g_sensorRefreshTimer.tick( runtime ) )

  //jobs.
  if( (g_jobs & Enums::Job_SensorRefresh) != 0 )
  {
    updateSensor();
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
    sprintf(&str[0], "Temp %5.2f*C",g_gasSensor.temperature );
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
void updateDoor()
{

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