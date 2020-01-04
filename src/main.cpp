/*------------------------------------------------------------------------------
    ()      File:   main.cpp
    /\      Author: Andrew Woodward-May
   //\\       
  //  \\    Description:
              Initial technical demonstration for sensor cabinet suite over i2c.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <LiquidCrystal_I2C.h>
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//Define Busses
enum I2C_BUSSES_ENUM { LocalBus, GlobalBus, I2C_MAX_WIRES = 2 };
TwoWire g_i2cBus[I2C_MAX_WIRES] = 
{ 
  TwoWire(1, I2C_FAST_MODE),
  TwoWire(2, I2C_FAST_MODE)
};

//Gas Sensor
Adafruit_BME680 g_gasSensor(&g_i2cBus[LocalBus]);
LiquidCrystal_I2C g_lcd(0x27, 16, 2, LCD_5x8DOTS, g_i2cBus[LocalBus]);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void setup() 
{
  g_i2cBus[LocalBus].begin();
  g_i2cBus[GlobalBus].begin();
  Serial.begin(9600);

  // Set up oversampling and filter initialization
  g_gasSensor.begin(0x76, true);
  g_gasSensor.setTemperatureOversampling(BME680_OS_8X);
  g_gasSensor.setHumidityOversampling(BME680_OS_2X);
  g_gasSensor.setPressureOversampling(BME680_OS_4X);
  g_gasSensor.setIIRFilterSize(BME680_FILTER_SIZE_3);
  g_gasSensor.setGasHeater(320, 150); // 320*C for 150 ms

  g_lcd.begin();

	g_lcd.backlight();
	g_lcd.print("Hello, world!");
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void loop()
{
  Serial.println("loop");
  delay(1000);

  if( auto endTime = g_gasSensor.beginReading() == 0 )
  {
    return;
  }

  if (!g_gasSensor.endReading()) 
  {
    return;
  }

  char str[64];
  sprintf(&str[0], "Temp = %.2f, VOC %.2f kOhm",g_gasSensor.temperature, (g_gasSensor.gas_resistance/1000.0f) );
  Serial.println(str);
}
