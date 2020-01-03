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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void setup() 
{
  g_i2cBus[LocalBus].begin();
  g_i2cBus[GlobalBus].begin();
  Serial.begin(9600);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void loop()
{
  float temp = g_gasSensor.readTemperature();
  char str[16];
  sprintf(&str[0], "Temp = %.2f",temp);
  Serial.println(str);
}
