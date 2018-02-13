/*!***************************************************************************
*!
*! FILE NAME  : ip.cc
*!
*! DESCRIPTION:
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include "ip.hh"
#include "ipaddr.hh"
#include "icmp.hh"
#include "tcp.hh"

#include "iostream.hh"
#include "sp_alloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef D_IP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** IP DEFINITION SECTION ***************************/

IP& IP::instance()
{
  static IP instance;
  return instance;
}

int identification = 0;

//----------------------------------------------------------------------------
//

IP::IP()
{
  // Holds the IP address of this server
  myIPAddress = new IPAddress(130, 235, 200, 101);
}

//----------------------------------------------------------------------------
//

const IPAddress& IP::myAddress()
{
  return *myIPAddress;
}


/****************** IPInPacket DEFINITION SECTION ***************************/

IPInPacket::IPInPacket(byte* theData,
    udword theLength,
    InPacket* theFrame):
    InPacket(theData, theLength, theFrame) // Initiate base class InPacket
{
}

//----------------------------------------------------------------------------
//

void
IPInPacket::decode()
{
  // cout << "ipPacketDecode" << endl;
  IPHeader* ipHeader = (IPHeader *) myData;
  // cout << "HILO: " << dec << HILO((ipHeader->versionNHeaderLength) << endl;
  // printf("%u\n", ipHeader->versionNHeaderLength);
  // printf("%u\n", ((ipHeader->versionNHeaderLength & 0xf0) >> 4));
  // printf("%u\n", (ipHeader->versionNHeaderLength & 0x0f));
  // cout << "fragment" << (HILO(ipHeader->fragmentFlagsNOffset) & 0x3FFF)<< endl;
  if(((ipHeader->versionNHeaderLength & 0xf0) >> 4) == 4 &&
       (HILO(ipHeader->fragmentFlagsNOffset) & 0x3FFF) == 0 &&
       ipHeader->destinationIPAddress == IP::instance().myAddress())
  {
    // uword ihl = (ipHeader->versionNHeaderLength & 0x0f) * 4;
    // cout << "IHL: " << dec << ihl << endl;
    uword totalLength = HILO(ipHeader->totalLength);
    // cout << "totalLength: " << dec << totalLength << endl;
    // uword identification = HILO(ipHeader->identification);
    // cout << "identification: " << dec << identification << endl;
    // uword fragmentFlagsNOffset = HILO(ipHeader->fragmentFlagsNOffset);

    // byte TTL = ipHeader->timeToLive;
    // cout << "TTL: " << unsigned(TTL) << endl;
    myProtocol = ipHeader->protocol;
    // cout << "protocol: " << unsigned(myProtocol) << endl;
    // uword headerChecksum = ipHeader->headerChecksum;
    // cout << "headerChecksum: " << hex << headerChecksum << endl;
    mySourceIPAddress = ipHeader->sourceIPAddress;
    // cout << "sourceIP: " << mySourceIPAddress << endl;
    // IPAddress destIP = ipHeader->destinationIPAddress;
    // cout << "destIP: " << destIP << endl;

    switch(myProtocol){
      case 1:{
        ICMPInPacket icmpPacket(myData+headerOffset(), totalLength-headerOffset(), this);
        icmpPacket.decode();
        break;
      }
      case 6:{
        TCPInPacket tcpPacket(myData+headerOffset(), totalLength-headerOffset(), this, mySourceIPAddress);
        tcpPacket.decode();
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
//

void
IPInPacket::answer(byte* theData, udword theLength)
{
  // cout << "IP ANSWER" << mySourceIPAddress << endl;
  byte* packet = new byte[theLength+headerOffset()]; //might leak
  IPHeader* ipResponseHeader = (IPHeader *) packet;
  ipResponseHeader->versionNHeaderLength = 0x45; // might need to calc for real
  // cout << "ipVersion: " << dec << unsigned(ipResponseHeader->versionNHeaderLength) << endl;
  ipResponseHeader->TypeOfService = 0;
  // cout << "length" << (IP::ipHeaderLength + theLength) << endl;
  ipResponseHeader->totalLength = HILO(IP::ipHeaderLength + theLength);
  ipResponseHeader->identification = identification++;
  ipResponseHeader->fragmentFlagsNOffset = 0;
  ipResponseHeader->timeToLive = 64;
  ipResponseHeader->protocol = myProtocol;
  ipResponseHeader->headerChecksum = 0;
  ipResponseHeader->sourceIPAddress = IP::instance().myAddress();
  ipResponseHeader->destinationIPAddress = mySourceIPAddress;
  ipResponseHeader->headerChecksum = calculateChecksum(packet, headerOffset());
  memcpy((packet+headerOffset()), theData, theLength);
  // cout << "ipVersion: " << dec << unsigned(ipResponseHeader->versionNHeaderLength) << endl;
  myFrame->answer(packet, theLength+headerOffset());
  delete[] packet;
}

//----------------------------------------------------------------------------
//

uword
IPInPacket::headerOffset()
{
  // IPHeader* ipHeader = (IPHeader *) myData;
  // return (ipHeader->versionNHeaderLength & 0x0f) * 4;
  return IP::ipHeaderLength;
}

//----------------------------------------------------------------------------
//

InPacket*
IPInPacket::copyAnswerChain()
{
  IPInPacket* anAnswerPacket = new IPInPacket(*this);
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain());
  return anAnswerPacket;
}

// /****************** END OF FILE ip.cc *************************************/
