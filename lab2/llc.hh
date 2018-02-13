/*!***************************************************************************
*!
*! FILE NAME  : llc.hh
*! 
*! DESCRIPTION: LLC dummy
*! 
*!***************************************************************************/

#ifndef llc_hh
#define llc_hh

/****************** INCLUDE FILES SECTION ***********************************/

#include "inpacket.hh"
#include "ethernet.hh"

/****************** CLASS DEFINITION SECTION ********************************/

/*****************************************************************************
*%
*% CLASS NAME   : LLCInPacket
*%
*% BASE CLASSES : InPacket
*%
*% CLASS TYPE   : 
*%
*% DESCRIPTION  : Decode a LLC packet.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class LLCInPacket : public InPacket
{
 public:
  LLCInPacket(byte*           theData,
              udword          theLength,
              InPacket*       theFrame,
              EthernetAddress theDestinationAddress,
              EthernetAddress theSourceAddress,
              uword           theTypeLen);
  // Ethernet must provide the ethernet information
  void decode();
  // Will identify and answer to ICMP ECHO (ping) packets to this server
  void answer(byte* theData, udword theLength);
  uword headerOffset();
  
 private:
  EthernetAddress myDestinationAddress;
  EthernetAddress mySourceAddress;
  uword           myTypeLen;
};

#endif
/****************** END OF FILE llc.hh *************************************/

