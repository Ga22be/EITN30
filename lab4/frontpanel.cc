/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"
#include "sp_alloc.h"

#include <stdlib.h>
#include <stdio.h>

// #define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** FrontPanel DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//

FrontPanel& FrontPanel::instance()
{
  static FrontPanel instance;
  return instance;
}

FrontPanel::FrontPanel():
  mySemaphore(Semaphore::createQueueSemaphore("Frontpanel",0)), //How do we do this??
  myNetworkLED(LED::LED(networkLedId)),
  myCDLED(LED::LED(cdLedId)),
  myStatusLED(LED::LED(statusLedId))
  {
    // Övriga konstanter som myNetworkLED eller enum strukuren är automatiskt tillgängliga

    // Initialize timers
    myNetworkLEDTimer = new NetworkLEDTimer(2);
    myCDLEDTimer = new CDLEDTimer(Clock::seconds);
    myStatusLEDTimer = new StatusLEDTimer(Clock::seconds*5);

    // Flash the network LED to show it works ^^
    packetReceived();

    // netLedEvent = false;
    // cdLedEvent = false;
    // statusLedEvent = false;

    Job::schedule(this);
  }

//----------------------------------------------------------------------------
//
void FrontPanel::packetReceived()
{
  // turn Network led on and start network led timer

  // Calls Led class
  // cout << "Net blink: packetReceived" << endl;
  myNetworkLED.on();
  myNetworkLEDTimer->start();
}

void FrontPanel::notifyLedEvent(uword theLedId)
{
  // cout << "Notified" << endl;
  // cout << theLedId << endl;
  // bool result = theLedId == statusLedId;
  // cout << result << endl;

  // Called from the timers to notify that a timer has expired.
  // Sets an event flag and signals the semaphore.
  switch(theLedId){
    case networkLedId:{
      netLedEvent = true;
      break;
    }

    case cdLedId:{
      cdLedEvent = true;
      break;
    }

    case statusLedId:{
      statusLedEvent = true;
      break;
    }
  }

  mySemaphore->signal();
}

void
FrontPanel::doit()
{
  // Main thread loop of FrontPanel. Initializes the led timers and goes into
  // a perptual loop where it awaits the semaphore. When it wakes it checks
  // the event flags to see which leds to manipulate and manipulates them.


  // cout << "doit: called" << endl;

  while(1){
    mySemaphore->wait();

    // cout << "doit: signaled" << endl;
    // cout << netLedEvent << endl;
    // cout << cdLedEvent << endl;
    // cout << statusLedEvent << endl;

    if(netLedEvent){
      // myNetworkLED.toggle();
      myNetworkLED.toggle();
      // cout << "Net blink: doit" << endl;
      netLedEvent = false;
    }

    if(cdLedEvent){
      myCDLED.toggle();
      // cout << "CD blink: doit" << endl;
      cdLedEvent = false;
    }

    if(statusLedEvent){
      myStatusLED.toggle();
      // cout << "Stat blink: doit" << endl;
      cout << "Core " << ax_coreleft_total() << endl;
      // malloc(16);
      statusLedEvent = false;
    }
  }

}


/****************** END OF FILE FrontPanel.cc ********************************/
