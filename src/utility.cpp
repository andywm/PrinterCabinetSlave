/*------------------------------------------------------------------------------  
    ()      File: utility.cpp
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "utility.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// BME680 Lookup Table - String defines.
//------------------------------------------------------------------------------
const char * VOCTable::_asString[MAX] =
{
        ENUM_AS_STRING(Good),
        ENUM_AS_STRING(Average),
        ENUM_AS_STRING(Subpar),
        ENUM_AS_STRING(Bad),
        ENUM_AS_STRING(Awful),
        ENUM_AS_STRING(Severe)
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Timer
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Timer::Timer( unsigned long duration, int& jobs, int jobOnComplete )
        : m_duration( duration )
        , m_jobs( jobs )
        , m_jobOnComplete( jobOnComplete )
{}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Timer::reset( unsigned long time )
{
        m_beginTime = time;
        m_alreadyTriggered = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Timer::tick( unsigned long time )
{
        //has the time elapsed?
        if( (time - m_beginTime) < m_duration )
        {
                return false;
        }

        //is the flag not currently set?
        if( (m_jobs & m_jobOnComplete) != 0 )
        {
                return true;
        }

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