/*!***************************************************************************
*!
*! FILE NAME  : arp.cc
*!
*! DESCRIPTION:
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "ethernet.hh"
#include "iostream.hh"
#include "arp.hh"
#include "ip.hh"
#include "ipaddr.hh"
#include "inpacket.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef D_ARP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** ARPInPacket DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//

ARPInPacket::ARPInPacket(byte*      theData,
    udword     theLength,
    InPacket*  theFrame):
    InPacket(theData, theLength, theFrame) // Initiate base class InPacket
{
}

//----------------------------------------------------------------------------
//

void
ARPInPacket::decode() {
  ARPHeader* aHeader = (ARPHeader *) myData;
  // cout << "ARPInPacket::decode Bugspray1" << endl;
  // cout << aHeader->targetIPAddress << ":" << IP::instance().myAddress() << endl;
  if(aHeader->targetIPAddress == IP::instance().myAddress()) {
    // cout << "arp from: " << aHeader->senderEthAddress << endl;
    // cout << aHeader->targetIPAddress << endl;
    // cout << "length: " << myLength << endl;

    if(HILO(aHeader->hardType) == 1 &&
       HILO(aHeader->protType) == 0x0800 &&
       aHeader->hardSize == EthernetAddress::length &&
       aHeader->protSize == IPAddress::size &&
       HILO(aHeader->op) == 1)
    {
      cout << "arpRequestReceived" << endl;
      ARPHeader aNewHeader;
      aNewHeader.hardType = aHeader->hardType;
      aNewHeader.protType = aHeader->protType;
      aNewHeader.hardSize = aHeader->hardSize;
      aNewHeader.protSize = aHeader->protSize;
      aNewHeader.op = HILO(2);
      aNewHeader.senderEthAddress = Ethernet::instance().myAddress();
      aNewHeader.senderIPAddress = IP::instance().myAddress();
      aNewHeader.targetEthAddress = aHeader->senderEthAddress;
      aNewHeader.targetIPAddress = aHeader->senderIPAddress;

      // cout << "1: " << aHeader->hardType << endl;
      // cout << "2: " << aHeader->protType << endl;
      // cout << "3: " << aHeader->hardSize << endl;
      // cout << "4: " << aHeader->protSize << endl;
      // cout << "5: " << aHeader->op << endl;
      // cout << "6: " << aHeader->senderEthAddress << endl;
      // cout << "8: " << aHeader->senderIPAddress << endl;
      // cout << "9: " << aHeader->targetEthAddress << endl;
      // cout << "10: " << aHeader->targetIPAddress << endl;
      //
      // cout << "1: " << aNewHeader.hardType << endl;
      // cout << "2: " << aNewHeader.protType << endl;
      // cout << "3: " << aNewHeader.hardSize << endl;
      // cout << "4: " << aNewHeader.protSize << endl;
      // cout << "5: " << aNewHeader.op << endl;
      // cout << "6: " << aNewHeader.senderEthAddress << endl;
      // cout << "8: " << aNewHeader.senderIPAddress << endl;
      // cout << "9: " << aNewHeader.targetEthAddress << endl;
      // cout << "10: " << aNewHeader.targetIPAddress << endl;
      // EthernetAddress* sendEth;
      // IPAddress* sendIP;
      // memcpy(sendEth, &aHeader->senderEthAddress, 6);
      // memcpy(sendIP, &aHeader->senderIPAddress, 4);
      // aNewHeader->senderEthAddress = aHeader->targetEthAddress;
      // aNewHeader->senderEthAddress = aHeader->targetIPAddress;


      // aNewHeader.hardType = aHeader->hardType;

      this->answer((byte *) &aNewHeader, myLength);
    }
  }
}

//----------------------------------------------------------------------------
//

void
ARPInPacket::answer(byte* theData, udword theLength) {
  myFrame->answer(theData, theLength);
}

//----------------------------------------------------------------------------
//

uword
ARPInPacket::headerOffset() {
  return 1337;
}

// /****************** END OF FILE arp.cc *************************************/
