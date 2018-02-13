/*!***************************************************************************
*!
*! FILE NAME  : icmp.cc
*!
*! DESCRIPTION:
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "icmp.hh"
#include "sp_alloc.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef D_ICMP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** ICMPInPacket DEFINITION SECTION ***************************/

ICMPInPacket::ICMPInPacket(byte*    theData,
    udword   theLength,
    InPacket*    theFrame):
    InPacket(theData, theLength, theFrame) // Initiate base class InPacket
{

}

//----------------------------------------------------------------------------
//

void
ICMPInPacket::decode() {
  /* code */
  // cout << "icmpPacketReceived" << endl;
  ICMPHeader* icmpHeader = (ICMPHeader *) myData;
  byte type = icmpHeader->type;
  // cout << "type: " << unsigned(type) << endl;
  // byte code = icmpHeader->code;
  // cout << "code: " << unsigned(code) << endl;
  // uword checksum = icmpHeader->checksum;
  // cout << "checksum: " << hex << checksum << endl;
  if(type == 8)
  {
    this->answer(myData, myLength);
  }
}

//----------------------------------------------------------------------------
//

void
ICMPInPacket::answer(byte* theData, udword theLength) {
  ICMPHeader* icmpResponseHeader = (ICMPHeader *) theData;
  if(icmpResponseHeader->type == 8){
    // cout << "ECHO REQUEST" << endl;
    icmpResponseHeader->type = 0;
    // Adjust ICMP checksum...
    icmpResponseHeader->checksum = icmpResponseHeader->checksum + 0x8;

    myFrame->answer(theData, theLength);
  }
}

//----------------------------------------------------------------------------
//

uword
ICMPInPacket::headerOffset() {
  return 1337;
}
/****************** END OF FILE icmp.cc *************************************/
