/*------------------------------------------------------------------------------  
    ()      File: utility.h
    /\      Copyright (c) 2020 Andrew Woodward-May - See legal.txt
   //\\     
  //  \\    Description:
              Collection of utility functions, helper structures, and tables
              for the program.         
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
#pragma once
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#if defined (IS_BLUEPILL_BUILD)
#include <cstring>
#else
#include <string.h>
#endif //defined(IS_BLUEPILL_BUILD)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//Defines
#define ENUM_AS_STRING(enumvar) #enumvar
//------------------------------------------------------------------------------
// Maths Utility
//------------------------------------------------------------------------------
template<typename Type>
static inline const Type & Max( const Type & A, const Type & B)
{
    return (A > B)? A : B;
}

template<typename Type>
static inline const Type & Min( const Type & A, const Type & B)
{
    return (A < B)? A : B;
}

//------------------------------------------------------------------------------
// BME680 Lookup Table
// Source for values - https://forums.pimoroni.com/t/bme680-observed-gas-ohms-readings/6608
//------------------------------------------------------------------------------
struct VOCTable
{
    enum                                        { Good,     Average,    Subpar,   Bad,      Awful,  Severe,     MAX };
    static constexpr uint32_t _table[MAX] =     { 431331,   213212,     108042,   54586,    27080,  13591 };
    static const char * _asString[MAX];
};

//------------------------------------------------------------------------------
// Helper timer class for periodic tasks.
//------------------------------------------------------------------------------
class Timer
{
public:
    Timer( unsigned long duration, int& jobs, int jobOnComplete );
    void reset( unsigned long time );
    bool tick( unsigned long time );

 private:
    unsigned long m_beginTime = 0;
    unsigned long m_duration = 0;
    
    int& m_jobs;
    int m_jobOnComplete = 0;
    bool m_alreadyTriggered = false;
};

//------------------------------------------------------------------------------
// Enclosure State
//------------------------------------------------------------------------------
struct EnvironmentInfo
{
    float temperature = 0.0f;
    float voc = 0.0f;
    bool doorOpen = false;
};

//------------------------------------------------------------------------------
// Display Helper
//------------------------------------------------------------------------------
//nb, do I _really_ need this to be a template? What other displays are you possible
//expecting.. not fond of this whole thing being in a header.
//------------------------------------------------------------------------------
template<uint8_t Width, uint8_t Height, uint8_t MaxReserved = 2>
class Display
{
public:
    using callback = void (*)(const char*, int,int);

public:
    Display( callback writeCbk )
        : m_drawFunction( writeCbk )
    {
        memset( &m_state[0], '\0', Height*(Width+1) -1 );
        clear();
    }

    void reserve( uint8_t id, uint8_t line, uint8_t begin_inclusive, uint8_t end_inclusive )
    {
        if( id >= MaxReserved )
           return;

        m_ranges[id].line = line;
        m_ranges[id].begin = begin_inclusive;
        m_ranges[id].end = end_inclusive;
    }

    void update( uint8_t id, const char * str )
    {
        const uint8_t line = m_ranges[id].line;
        const uint8_t begin = m_ranges[id].begin;
        const uint8_t end = m_ranges[id].end;

        const int range = end-begin+1;
        const uint8_t size = Min(range, (int)strlen(str));

        //fill
        memset(&(m_state[line][begin]), ' ', range );
        strncpy(&(m_state[line][begin]), str, size );
    }

    void draw()
    {
        m_drawFunction(&m_state[0][0], Width, Height);
    }

    void clear()
    {
        for(int line=0; line < Height; line++)
        {
            memset( &m_state[line], ' ', Width );
        }
    }

private:
  struct Range 
  { 
      uint8_t line = 0;
      uint8_t begin =0;
      uint8_t end =0;
  } m_ranges[MaxReserved];

  callback m_drawFunction;
  char m_state[Height][Width+1];
};
