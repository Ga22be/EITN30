/*!***************************************************************************
*!
*! FILE NAME  : tcp.hh
*!
*! DESCRIPTION: TCP, Transport control protocol
*!
*!***************************************************************************/

#ifndef tcp_hh
#define tcp_hh

/****************** INCLUDE FILES SECTION ***********************************/

#include "inpacket.hh"
#include "ipaddr.hh"
#include "queue.hh"

/****************** CLASS DEFINITION SECTION ********************************/

/*****************************************************************************
*%
*% CLASS NAME   : TCP
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   : Singleton
*%
*% DESCRIPTION  : Holds the TCP connections.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPConnection;
class TCP
{
 public:
  static TCP& instance();
  TCPConnection* getConnection(IPAddress& theSourceAddress,
                               uword      theSourcePort,
                               uword      theDestinationPort);
  // Find a connection in the connection list

  TCPConnection* createConnection(IPAddress& theSourceAddress,
                                  uword      theSourcePort,
                                  uword      theDestinationPort,
                                  InPacket*  theCreator);
  // Create a new connection and insert it in the connection list

  void deleteConnection(TCPConnection*);
  // Removes a connection from the list and deletes it

  enum { tcpHeaderLength = 20 };

 private:
  TCP();
  PQueue<TCPConnection*> myConnectionList;
  // All the TCP connections
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPConnection
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Holds the state of one TCP connection.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPState;
class TCPSender;
class TCPConnection
{
 public:
  TCPConnection(IPAddress& theSourceAddress,
                uword      theSourcePort,
                uword      theDestinationPort,
                InPacket*  theCreator);
  ~TCPConnection();

  bool tryConnection(IPAddress& theSourceAddress,
                     uword      theSourcePort,
                     uword      theDestinationPort);
  // Returns true if this connection matches the arguments

  void Synchronize(udword theSynchronizationNumber);
  // Handle an incoming SYN segment
  void NetClose();
  // Handle an incoming FIN segment
  void AppClose();
  // Handle close from application
  void Kill();
  // Handle an incoming RST segment, can also called in other error conditions
  void Receive(udword theSynchronizationNumber,
               byte*  theData,
               udword theLength);
  // Handle incoming data
  void Acknowledge(udword theAcknowledgementNumber);
  // Handle incoming Acknowledgement
  void Send(byte*  theData,
            udword theLength);
  // Send outgoing data

  //-------------------------------------------------------------------------
  //
  // Interface to TCPState
  //

  IPAddress  hisAddress;
  // Other hosts IP address
  uword      hisPort;
  // Other hosts port
  uword      myPort;
  // My port

  udword     receiveNext;
  // next expected sequence number from other host
  uword      receiveWindow;
  // Number of bytes it is posible to send without getting an ACK

  udword     sendNext;
  // next sequence number to send
  udword     sentUnAcked;
  // Data has been acknowledged up to this sequence number. What remains up to
  // sendNext is sent but not yet acked by the other host.

  TCPSender* myTCPSender;
  TCPState*  myState;
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   : Pure virtual
*%
*% DESCRIPTION  : Handle a state of one TCP connection.
*%
*% SUBCLASSING  : yes.
*%
*%***************************************************************************/
class TCPState
{
 public:
  virtual void Synchronize(TCPConnection* theConnection,
                           udword theSynchronizationNumber);
  // Handle an incoming SYN segment
  virtual void NetClose(TCPConnection* theConnection);
  // Handle an incoming FIN segment
  virtual void AppClose(TCPConnection* theConnection);
  // Handle close from application
  virtual void Kill(TCPConnection* theConnection);
  // Handle an incoming RST segment, can also called in other error conditions
  virtual void Receive(TCPConnection* theConnection,
                       udword theSynchronizationNumber,
                       byte*  theData,
                       udword theLength);
  // Handle incoming data
  virtual void Acknowledge(TCPConnection* theConnection,
                           udword theAcknowledgementNumber);
  // Handle incoming Acknowledgement
  virtual void Send(TCPConnection* theConnection,
                    byte*  theData,
                    udword theLength);
  // Send outgoing data
};

/*****************************************************************************
*%
*% CLASS NAME   : ListenState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Handle listen state.
*%
*% SUBCLASSING  : no.
*%
*%***************************************************************************/
class ListenState : public TCPState
{
 public:
  static ListenState* instance();

  void Synchronize(TCPConnection* theConnection,
                   udword theSynchronizationNumber);
  // Handle an incoming SYN segment

 protected:
  ListenState() {}
};

/*****************************************************************************
*%
*% CLASS NAME   : SynRecvdState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Handle SYN received state.
*%
*% SUBCLASSING  : no.
*%
*%***************************************************************************/
class SynRecvdState : public TCPState
{
 public:
  static SynRecvdState* instance();

  void Acknowledge(TCPConnection* theConnection,
                   udword theAcknowledgementNumber);
  // Handle incoming Acknowledgement

 protected:
  SynRecvdState() {}
};

/*****************************************************************************
*%
*% CLASS NAME   : EstablishedState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Handle established state.
*%
*% SUBCLASSING  : no.
*%
*%***************************************************************************/
class EstablishedState : public TCPState
{
 public:
  static EstablishedState* instance();

  void NetClose(TCPConnection* theConnection);
  // Handle an incoming FIN segment
  void Receive(TCPConnection* theConnection,
               udword theSynchronizationNumber,
               byte*  theData,
               udword theLength);
  // Handle incoming data
  void Acknowledge(TCPConnection* theConnection,
                   udword theAcknowledgementNumber);
  // Handle incoming Acknowledgement
  void Send(TCPConnection* theConnection,
            byte*  theData,
            udword theLength);
  // Send outgoing data

 protected:
  EstablishedState() {}
};

/*****************************************************************************
*%
*% CLASS NAME   : CloseWaitState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Handle established state.
*%
*% SUBCLASSING  : no.
*%
*%***************************************************************************/
class CloseWaitState : public TCPState
{
 public:
  static CloseWaitState* instance();

  void AppClose(TCPConnection* theConnection);
  // Handle close from application

 protected:
  CloseWaitState() {}
};

/*****************************************************************************
*%
*% CLASS NAME   : LastAckState
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Handle established state.
*%
*% SUBCLASSING  : no.
*%
*%***************************************************************************/
class LastAckState : public TCPState
{
 public:
  static LastAckState* instance();

  void Acknowledge(TCPConnection* theConnection,
                   udword theAcknowledgementNumber);
  // Handle incoming Acknowledgement

 protected:
  LastAckState() {}
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPSender
*%
*% BASE CLASSES :
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Send a TCP segment.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPSender
{
 public:
  TCPSender(TCPConnection* theConnection,
	        InPacket*      theCreator);
  ~TCPSender();

  void sendFlags(byte theFlags);
  // Send a flag segment without data.
  void sendData(byte*  theData,
                udword theLength);
  // Send a data segment. PSH and ACK flags are set.

 private:
  TCPConnection* myConnection;
  InPacket*      myAnswerChain;
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPInPacket
*%
*% BASE CLASSES : InPacket
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Decode a TCP packet.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPInPacket : public InPacket
{
 public:
  TCPInPacket(byte*           theData,
              udword          theLength,
              InPacket*       theFrame,
              IPAddress&      theSourceAddress);
  void decode();
  void answer(byte* theData, udword theLength);
  uword headerOffset();

  InPacket* copyAnswerChain();

 private:
  IPAddress mySourceAddress;
  uword     mySourcePort;
  uword     myDestinationPort;
  udword    mySequenceNumber;
  udword    myAcknowledgementNumber;
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPHeader
*%
*% BASE CLASSES : none
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : Describes the fields of a TCP packet.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPHeader
{
  public:
  TCPHeader() {}

  uword  sourcePort;
  uword  destinationPort;
  udword sequenceNumber;
  udword acknowledgementNumber;
  byte   headerLength;
  // 6 reserved bits omitted.
  byte   flags;
  uword  windowSize;
  uword  checksum;
  uword  urgentPointer;
};

/*****************************************************************************
*%
*% CLASS NAME   : TCPPseudoHeader
*%
*% BASE CLASSES : none
*%
*% CLASS TYPE   :
*%
*% DESCRIPTION  : The TCP pseudo header used for checksum calculation.
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class TCPPseudoHeader
{
 public:
  TCPPseudoHeader(IPAddress& theDestination, uword theLength);
  uword checksum();

 protected:
  IPAddress sourceIPAddress;
  IPAddress destinationIPAddress;
  byte      zero;
  byte      protocol;
  uword     tcpLength;
};

#endif
/****************** END OF FILE tcp.hh *************************************/
