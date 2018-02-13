/*!***************************************************************************
*!
*! FILE NAME  : NetworkLEDTimer.cc
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

/****************** NetworkLEDTimer DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime)
{
  // Constructor: initiate myBlinkTime
  myBlinkTime = blinkTime;
}

//----------------------------------------------------------------------------
//
void
NetworkLEDTimer::start()
{
  // Start timer
  this->timeOutAfter(myBlinkTime);
}

//----------------------------------------------------------------------------
//
void
NetworkLEDTimer::timeOut()
{
  // notify FrontPanel that this timer has expired.
  // cout << "NetworkLEDTimer timed out." << endl;
  FrontPanel::instance().notifyLedEvent(FrontPanel::networkLedId);
}

/****************** END OF FILE NetworkLEDTimer.cc ********************************/
