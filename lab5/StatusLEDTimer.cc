/*!***************************************************************************
*!
*! FILE NAME  : StatusLEDTimer.cc
*!
*! DESCRIPTION:
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

/****************** StatusLEDTimer DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod)
{
  // Constructor: initiate and start timer
  //Call super class PeriodicTimer methods
  this->timerInterval(blinkPeriod);
  this->startPeriodicTimer();
}

//----------------------------------------------------------------------------
//
void
StatusLEDTimer::timerNotify()
{
  // notify FrontPanel that this timer has expired.
  // cout << "StatusLEDTimer timed out." << endl;
  FrontPanel::instance().notifyLedEvent(FrontPanel::statusLedId);
}

/****************** END OF FILE StatusLEDTimer.cc ********************************/
