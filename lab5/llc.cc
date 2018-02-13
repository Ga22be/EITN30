/*!***************************************************************************
*!
*! FILE NAME  : llc.cc
*!
*! DESCRIPTION: LLC dummy
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "ethernet.hh"
#include "llc.hh"
#include "arp.hh"
#include "ip.hh"
#include "icmp.hh"

//#define D_LLC
#ifdef D_LLC
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** LLC DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
LLCInPacket::LLCInPacket(byte*           theData,
                         udword          theLength,
						 InPacket*       theFrame,
                         EthernetAddress theDestinationAddress,
                         EthernetAddress theSourceAddress,
                         uword           theTypeLen):
InPacket(theData, theLength, theFrame),
myDestinationAddress(theDestinationAddress),
mySourceAddress(theSourceAddress),
myTypeLen(theTypeLen)
{
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::decode()
{
  // trace << "to " << myDestinationAddress << " from " << mySourceAddress
  //  << " typeLen " << myTypeLen << endl;
  if (myDestinationAddress == Ethernet::instance().myAddress() && myTypeLen == 0x800)
  {
    //IP PACKET
    IPInPacket ipPacket(myData, myLength, this);
    ipPacket.decode();
  }

  EthernetAddress broadcast(0xff,0xff,0xff,0xff,0xff,0xff);
	if (myDestinationAddress == broadcast && myTypeLen == 0x806) {
    ARPInPacket arpPacket(myData, 28, this);
    arpPacket.decode();
  }
    // ARPHeader* aHeader = (ARPHeader *) myData;
    // if(aHeader->targetIPAddress == IP::instance().myAddress()) {
    //   cout << "arp from: " << mySourceAddress << endl;
    //   cout << aHeader->targetIPAddress << endl;
    // }
}



//----------------------------------------------------------------------------
//
void
LLCInPacket::answer(byte *theData, udword theLength)
{
  myFrame->answer(theData, theLength);
}

//----------------------------------------------------------------------------
//
uword
LLCInPacket::headerOffset()
{
  return myFrame->headerOffset() + 0;
}

//----------------------------------------------------------------------------
//
InPacket*
LLCInPacket::copyAnswerChain()
{
  LLCInPacket* anAnswerPacket = new LLCInPacket(*this);
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain());
  return anAnswerPacket;
}

/****************** END OF FILE Ethernet.cc *************************************/
