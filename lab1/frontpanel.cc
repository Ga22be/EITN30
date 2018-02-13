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
  myCDLED(LED::LED(statusLedId)),
  myStatusLED(LED::LED(cdLedId))
  {
    // Övriga konstanter som myNetworkLED eller enum strukuren är automatiskt tillgängliga

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
  // myNetworkLED.on();
  cout << "Net blink: packetReceived" << endl;
  myNetworkLED.flash();
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

  // Initialize timers
  myNetworkLEDTimer = new NetworkLEDTimer(Clock::seconds*1);
  myCDLEDTimer = new CDLEDTimer(Clock::seconds*2);
  myStatusLEDTimer = new StatusLEDTimer(Clock::seconds*3);

  // cout << "doit: called" << endl;

  while(1){
    mySemaphore->wait();

    // cout << "doit: signaled" << endl;
    // cout << netLedEvent << endl;
    // cout << cdLedEvent << endl;
    // cout << statusLedEvent << endl;

    if(netLedEvent){
      // myNetworkLED.toggle();
      myNetworkLED.flash();
      cout << "Net blink: doit" << endl;
      netLedEvent = false;
    }

    if(cdLedEvent){
      // myCDLED.toggle();
      myCDLED.flash();
      cout << "CD blink: doit" << endl;
      cdLedEvent = false;
    }

    if(statusLedEvent){
      // myStatusLED.toggle();
      myStatusLED.flash();
      cout << "Stat blink: doit" << endl;
      statusLedEvent = false;
    }
  }

}


/****************** END OF FILE FrontPanel.cc ********************************/
