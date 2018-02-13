/*!***************************************************************************
*!
*! FILE NAME  : LED.cc
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

/****************** LED DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//

byte LED::writeOutRegisterShadow = 0x78;

LED::LED(byte theLedNumber)
{
  // Constructor: initiate the bitmask 'myLedBit'.
  myLedBit = theLedNumber; /* convert LED number to bit weight */
}

//----------------------------------------------------------------------------
//
void
LED::on()
{
  uword led = 4 << myLedBit;  /* convert LED number to bit weight */
  *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow &= ~led;
  iAmOn = true;
}

void
LED::off()
{
  uword led = 4 << myLedBit;  /* convert LED number to bit weight */
  *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow |= led;
  iAmOn = false;
}

void
LED::toggle()
{
  // toggle this led'
  if(iAmOn) {
    this->off();
  }
  else {
    this->on();
  }
}

void
LED::flash()
{
  int i;
  for (i = 0; i < 60000; i++)
  {
    on();
  }
  for (i = 0; i < 60000; i++)
  {
    off();
  }
}

/****************** END OF FILE LED.cc ********************************/
