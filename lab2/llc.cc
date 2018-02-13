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
  if (myDestinationAddress == Ethernet::instance().myAddress())
  {
	trace << "length " << myLength << " typelen 0x" << hex << myTypeLen
		  << dec << " (" << myTypeLen << ")" << endl;
    if ((myTypeLen == 0x800) &&
		(myLength > 28) &&
		(*(myData + 20) == 8))
	{
    // check if is an ICMP ECHO request skip IP totally...
          uword icmpSeq = *(uword*)(myData + 26);
          icmpSeq = ((icmpSeq & 0xff00) >> 8) | ((icmpSeq & 0x00ff) << 8);
  	  trace << "icmp echo, icmp_seq=" << icmpSeq << endl;
	  // create a resonse...
          uword hoffs = myFrame->headerOffset();
          byte* temp = new byte[myLength + hoffs];
	  byte* aReply = temp + hoffs;
	  memcpy(aReply, myData, myLength);
	  // by reusing his ip packet (including id nr) we get the same checksum :)
      // Just reverse IP addresses.
      aReply[12] = myData[16];
      aReply[13] = myData[17];
      aReply[14] = myData[18];
      aReply[15] = myData[19];
      aReply[16] = myData[12];
      aReply[17] = myData[13];
      aReply[18] = myData[14];
      aReply[19] = myData[15];

	  // Change to reply
	  aReply[20] = 0;
	  // Adjust ICMP checksum...
      uword oldSum = *(uword*)(myData + 22);
	  uword newSum = oldSum + 0x8;
      *(uword*)(aReply + 22) = newSum;
	  this->answer(aReply, myLength);
    delete[] temp;
	}
  }
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::answer(byte *theData, udword theLength)
{
  myFrame->answer(theData, theLength);
}

uword
LLCInPacket::headerOffset()
{
  return myFrame->headerOffset() + 0;
}

/****************** END OF FILE Ethernet.cc *************************************/
