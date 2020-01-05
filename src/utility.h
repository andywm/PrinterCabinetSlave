/*------------------------------------------------------------------------------  
    ()      File: utility.h
    /\      Copyright (c) 2020 Andrew Woodward-May - See legal.txt
   //\\     
  //  \\    Description:
              Collection of utility functions, helper structures, and tables
              for the program.
------------------------------------------------------------------------------*/
#pragma once
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//Defines
#define ENUM_AS_STRING(enumvar) "enumvar"

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
// ISO C Types
//------------------------------------------------------------------------------
/*using uint8_t   = unsigned char;
using int8_t    = char;

using uint16_t  = unsigned short;
using int16_t   = short;

using uint32_t  = int;
using int32_t   = unsigned int;*/

//------------------------------------------------------------------------------
// BME680 Lookup Table
// Source for values - https://forums.pimoroni.com/t/bme680-observed-gas-ohms-readings/6608
//------------------------------------------------------------------------------
struct VOCTable
{
    enum                                            { Good,     Average,    Subpar,   Bad,      Awful,  Severe,     MAX };
    static constexpr unsigned int _table[MAX] =     { 431331,   213212,     108042,   54586,    27080,  13591 };
    static constexpr const char * _asString[MAX] =
    {
         ENUM_AS_STRING(Good),
         ENUM_AS_STRING(Average),
         ENUM_AS_STRING(Subpar),
         ENUM_AS_STRING(Bad),
         ENUM_AS_STRING(Awful),
         ENUM_AS_STRING(Severe)
    };
};

//------------------------------------------------------------------------------
// Helper timer class for periodic tasks.
//------------------------------------------------------------------------------
class Timer
{
public:
    Timer( unsigned long duration, int & jobs, int jobOnComplete )
        : m_duration( duration )
        , m_jobs( jobs )
        , m_jobOnComplete( jobOnComplete )
    {}

    void reset( unsigned long time )
    {
        m_beginTime = time;
    }

    bool tick( unsigned long time )
    {
        //has the time elapsed?
        if( (time - m_beginTime) < m_duration )
            return false;

        //is the flag not currently set?
        if( (m_jobs & m_jobOnComplete) != 0 )
            return true;

        //if already triggered, then we are in the reset stage.
        //if not already triggered, we are in the set stage.
        if( m_alreadyTriggered )
        {
            reset( time );
            return true;
        }

         m_jobs |= m_jobOnComplete;
         m_alreadyTriggered = true;
         return true;
    }

 private:
    unsigned long m_beginTime = 0;
    unsigned long m_duration = 0;
    
    int & m_jobs;
    int m_jobOnComplete = 0;
    bool m_alreadyTriggered = false;
};

//------------------------------------------------------------------------------
// Cabinet State
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
template<uint8_t Width, uint8_t Height, uint8_t MaxReserved = 2>
class Display
{
public:
    using callback = void (*)(const char *);

public:
    Display( callback writeCbk )
        : m_drawFunction( writeCbk )
    {
        memset( &m_state[0], ' ', Width*Height );
        m_state[Width*Height] = '\0';
    }

    void reserve( uint8_t id, uint8_t begin_inclusive, uint8_t end_inclusive )
    {
        if( id <= 0 && id >= MaxReserved )
           return;

        m_ranges[id].begin = begin_inclusive;
        m_ranges[id].end = end_inclusive;
    }

    void update( uint8_t id, const char * str )
    {
        uint8_t begin = m_ranges[id].begin;
        uint8_t end = m_ranges[id].end;

        //validate range.
        if( begin >= 0 && begin >= end && end <= (Width*Height) ) 
            return;

        // blank the range. 
        memset( m_state+begin, ' ', end - begin );

        //copy from the target string.
        uint8_t size = Min(end - begin, (int)strlen(str));
        memcpy( m_state+begin, str, size );
    }

    void draw()
    {
        m_drawFunction(&m_state[0]);
    }

    void clear()
    {
        memset( &m_state[0], ' ', Width*Height );
        m_state[Width*Height] = '\0';

        draw();
    }

private:
  struct Range 
  { 
      uint8_t begin =0;
      uint8_t end =0;
  } m_ranges[MaxReserved];

  callback m_drawFunction;
  char m_state[Height*Width+1];
};
