/*!***************************************************************************
*!
*! FILE NAME  : CDLEDTimer.cc
*!
*! DESCRIPTION: Represents one LED
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** CDLEDTimer DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
CDLEDTimer::CDLEDTimer(Duration blinkPeriod)
{
  // Constructor: initiate and start timer
  //Call super class PeriodicTimer methods
  PeriodicTimed::timerInterval(blinkPeriod);
  PeriodicTimed::startPeriodicTimer();
}

//----------------------------------------------------------------------------
//
void
CDLEDTimer::timerNotify()
{
  // notify FrontPanel that this timer has expired.
  // cout << "CDLEDTimer timed out." << endl;
  FrontPanel::instance().notifyLedEvent(FrontPanel::cdLedId);
}

/****************** END OF FILE CDLEDTimer.cc ********************************/
