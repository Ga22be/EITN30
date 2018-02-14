/*!***************************************************************************
*!
*! FILE NAME  : tcp.cc
*!
*! DESCRIPTION: TCP, Transport control protocol
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
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "tcpsocket.hh"


// #define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** TCP DEFINITION SECTION *************************/

enum { FIN = 0x01,
       SYN = 0x02,
       RST = 0x04,
       PSH = 0x08,
       ACK = 0x10,
       URG = 0x20,
       ECE = 0x40,
       CWR = 0x80};
//----------------------------------------------------------------------------
//
TCP::TCP()
{
  trace << "TCP created." << endl;
}

//----------------------------------------------------------------------------
//
TCP&
TCP::instance()
{
  static TCP myInstance;
  return myInstance;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::getConnection(IPAddress& theSourceAddress,
                   uword      theSourcePort,
                   uword      theDestinationPort)
{
  TCPConnection* aConnection = NULL;
  // Find among open connections
  uword queueLength = myConnectionList.Length();
  myConnectionList.ResetIterator();
  bool connectionFound = false;
  while ((queueLength-- > 0) && !connectionFound)
  {
    aConnection = myConnectionList.Next();
    connectionFound = aConnection->tryConnection(theSourceAddress,
                                                 theSourcePort,
                                                 theDestinationPort);
  }
  if (!connectionFound)
  {
    trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    trace << "Found connection in queue" << endl;
  }
  return aConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::createConnection(IPAddress& theSourceAddress,
                      uword      theSourcePort,
                      uword      theDestinationPort,
                      InPacket*  theCreator)
{
  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);
  myConnectionList.Append(aConnection);
  return aConnection;
}

//----------------------------------------------------------------------------
//
void
TCP::connectionEstablished(TCPConnection *theConnection)
{
  if (theConnection->serverPortNumber() == 7)
  {
    TCPSocket* aSocket = new TCPSocket(theConnection);
    // Create a new TCPSocket.
    theConnection->registerSocket(aSocket);
    // Register the socket in the TCPConnection.
    Job::schedule(new SimpleApplication(aSocket));
    // Create and start an application for the connection.
  }
}

//----------------------------------------------------------------------------
//
bool
TCP::acceptConnection(uword portNo)
{
  // Is true when a connection is accepted on port portNo.
  return portNo == 7;
}

//----------------------------------------------------------------------------
//
void
TCP::deleteConnection(TCPConnection* theConnection)
{
  myConnectionList.Remove(theConnection);
  delete theConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort)
{
  trace << "TCP connection created" << endl;
  myTCPSender = new TCPSender(this, theCreator),
  myState = ListenState::instance();
}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  trace << "TCP connection destroyed" << endl;
  delete mySocket; // ========= TESTING ===========
  delete myTCPSender;
}

//----------------------------------------------------------------------------
//
bool
TCPConnection::tryConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort)
{
  return (theSourcePort      == hisPort   ) &&
         (theDestinationPort == myPort    ) &&
         (theSourceAddress   == hisAddress);
}

// TCPConnection cont...
//----------------------------------------------------------------------------
//
void
TCPConnection::Synchronize(udword theSynchronizationNumber)
{
  // Handle an incoming SYN segment
  myState->Synchronize(this, theSynchronizationNumber);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::NetClose()
{
  // Handle an incoming FIN segment
  myState->NetClose(this);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::AppClose()
{
  // Handle close from application
  myState->AppClose(this);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::Kill()
{
  // Handle an incoming RST segment, can also called in other error conditions
  myState->Kill(this);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::Receive(udword theSynchronizationNumber,
                       byte*  theData,
                       udword theLength)
{
  // Handle incoming data
  myState->Receive(this, theSynchronizationNumber, theData, theLength);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::Acknowledge(udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  myState->Acknowledge(this, theAcknowledgementNumber);
}

//----------------------------------------------------------------------------
//
void
TCPConnection::Send(byte*  theData,
                    udword theLength)
{
  myState->Send(this, theData, theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPConnection::serverPortNumber()
{
  return myPort;
}

//----------------------------------------------------------------------------
//
void
TCPConnection::registerSocket(TCPSocket* theSocket)
{
  mySocket = theSocket;
}

//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.


//----------------------------------------------------------------------------
//
void
TCPState::Kill(TCPConnection* theConnection)
{
  trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}

//----------------------------------------------------------------------------
//
void
TCPState::Synchronize(TCPConnection* theConnection,
                         udword theSynchronizationNumber)
{
  // Handle an incoming SYN segment
  trace << "TCPState::Synchronize" << endl;
}

//----------------------------------------------------------------------------
//
void
TCPState::NetClose(TCPConnection* theConnection)
{
  // Handle an incoming FIN segment
  trace << "TCPState::NetClose" << endl;
}

//----------------------------------------------------------------------------
//
void
TCPState::AppClose(TCPConnection* theConnection)
{
  // Handle close from application
  trace << "TCPState::AppClose" << endl;
}

//----------------------------------------------------------------------------
//
void
TCPState::Receive(TCPConnection* theConnection,
                     udword theSynchronizationNumber,
                     byte*  theData,
                     udword theLength)
{
  // Handle incoming data
  trace << "TCPState::Receive" << endl;
}

//----------------------------------------------------------------------------
//
void
TCPState::Acknowledge(TCPConnection* theConnection,
                         udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  trace << "TCPState::Acknowledge" << endl;
}

//----------------------------------------------------------------------------
//
void
TCPState::Send(TCPConnection* theConnection,
                  byte*  theData,
                  udword theLength)
{
  // Send outgoing data
  trace << "TCPState::Send" << endl;
}

//----------------------------------------------------------------------------
//
ListenState*
ListenState::instance()
{
  static ListenState myInstance;
  trace << "ListenState::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
ListenState::Synchronize(TCPConnection* theConnection,
                         udword theSynchronizationNumber)
{
  if(TCP::instance().acceptConnection(theConnection->myPort))
  {
    switch (theConnection->myPort)
    {
      case 7:
      trace << "got SYN on ECHO port" << endl;
      theConnection->receiveNext = theSynchronizationNumber + 1;
      theConnection->receiveWindow = 8*1024;
      theConnection->sendNext = get_time();
      // Next reply to be sent.
      theConnection->sentUnAcked = theConnection->sendNext;
      // Send a segment with the SYN and ACK flags set.
      theConnection->myTCPSender->sendFlags(SYN|ACK);
      // Prepare for the next send operation.
      theConnection->sendNext += 1;
      // Change state
      theConnection->myState = SynRecvdState::instance();
      break;
      default:
      trace << "send RST..." << endl;
      theConnection->sendNext = 0;
      // Send a segment with the RST flag set.
      theConnection->myTCPSender->sendFlags(0x04);
      TCP::instance().deleteConnection(theConnection);
      break;
    }
  }
}

//----------------------------------------------------------------------------
//
SynRecvdState*
SynRecvdState::instance()
{
  static SynRecvdState myInstance;
  trace << "SynRecvdState::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
SynRecvdState::Acknowledge(TCPConnection* theConnection,
                           udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  trace << "SynRecvdState::Acknowledge" << endl;

  if(theAcknowledgementNumber == theConnection->sendNext){
    theConnection->sentUnAcked= theConnection->sendNext;
    TCP::instance().connectionEstablished(theConnection);
    theConnection->myState = EstablishedState::instance();
  }
}

//----------------------------------------------------------------------------
//
EstablishedState*
EstablishedState::instance()
{
  static EstablishedState myInstance;
  trace << "EstablishedState::instance" << endl;
  return &myInstance;
}


//----------------------------------------------------------------------------
//
void
EstablishedState::NetClose(TCPConnection* theConnection)
{
  trace << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK
  theConnection->receiveNext++;
  theConnection->myTCPSender->sendFlags(ACK);

  // Go to NetClose wait state, inform application
  theConnection->mySocket->socketEof();
  theConnection->myState = CloseWaitState::instance();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...
  // theConnection->AppClose();
}

//----------------------------------------------------------------------------
//
void
EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  trace << "EstablishedState::Receive" << endl;

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment,
  theConnection->receiveNext += theLength;
  theConnection->myTCPSender->sendFlags(ACK);

  //then echo the data using Send.
  // theConnection->Send(theData, theLength);
  theConnection->mySocket->socketDataReceived(theData, theLength);

}

//----------------------------------------------------------------------------
//
void
EstablishedState::Acknowledge(TCPConnection* theConnection,
                              udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  trace << "EstablishedState::Acknowledge" << endl;
  theConnection->sentUnAcked = theAcknowledgementNumber-1;
  // theConnection->sendNext = theAcknowledgementNumber;
  theConnection->mySocket->socketDataSent();
}

//----------------------------------------------------------------------------
//
void
EstablishedState::Send(TCPConnection* theConnection,
                       byte*  theData,
                       udword theLength)
{
  // Send outgoing data
  theConnection->myTCPSender->sendData(theData, theLength);
  theConnection->sendNext += theLength;
}

void
EstablishedState::AppClose(TCPConnection* theConnection)
{
  theConnection->myTCPSender->sendFlags(FIN|ACK);
  theConnection->myState = FinWait1State::instance();
}

//----------------------------------------------------------------------------
//
CloseWaitState*
CloseWaitState::instance()
{
  static CloseWaitState myInstance;
  trace << "CloseWaitState::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
CloseWaitState::AppClose(TCPConnection* theConnection)
{
  theConnection->myTCPSender->sendFlags(FIN|ACK);
  theConnection->myState = LastAckState::instance();
  // Handle close from application
}

//----------------------------------------------------------------------------
//
LastAckState*
LastAckState::instance()
{
  static LastAckState myInstance;
  trace << "LastAckState::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
LastAckState::Acknowledge(TCPConnection* theConnection,
                          udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  theConnection->Kill();
}

//----------------------------------------------------------------------------
//
FinWait1State*
FinWait1State::instance()
{
  static FinWait1State myInstance;
  trace << "FinWait1State::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
FinWait1State::Acknowledge(TCPConnection* theConnection,
                          udword theAcknowledgementNumber)
{
  // Handle incoming Acknowledgement
  trace << "FinWait1State::Acknowledge" << endl;
  if(theAcknowledgementNumber == theConnection->sendNext+1)
  {
    theConnection->sentUnAcked = theAcknowledgementNumber-1;
    theConnection->sendNext = theAcknowledgementNumber;
    //theConnection->myTCPSender->sendFlags(ACK);
    theConnection->myState = FinWait2State::instance();
    // theConnection->NetClose();
  } else
  {
    theConnection->sentUnAcked = theAcknowledgementNumber-1;
    theConnection->sendNext = theAcknowledgementNumber;
  }
}

//----------------------------------------------------------------------------
//
FinWait2State*
FinWait2State::instance()
{
  static FinWait2State myInstance;
  trace << "FinWait2State::instance" << endl;
  return &myInstance;
}

//----------------------------------------------------------------------------
//
void
FinWait2State::NetClose(TCPConnection* theConnection)
{
  // Close the connection
  theConnection->receiveNext++;
  theConnection->myTCPSender->sendFlags(ACK);
  // theConnection->Kill(); //TODO WAIT TIME
  TimeWaitTimer* timer = new TimeWaitTimer(theConnection);
  timer->start();
}

//----------------------------------------------------------------------------
//
TCPSender::TCPSender(TCPConnection* theConnection,
                     InPacket*      theCreator):
        myConnection(theConnection),
        myAnswerChain(theCreator->copyAnswerChain()) // Copies InPacket chain!
{
}

//----------------------------------------------------------------------------
//
TCPSender::~TCPSender()
{
  myAnswerChain->deleteAnswerChain();
}

//----------------------------------------------------------------------------
//
void
TCPSender::sendFlags(byte theFlags)
{
  // Decide on the value of the length totalSegmentLength.
  // Allocate a TCP segment.
  trace << "TCPSender::sendFlags" << endl;
  byte totalSegmentLength = TCP::tcpHeaderLength;

  byte* anAnswer = new byte[totalSegmentLength];
  // Calculate the pseudo header checksum
  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  // cout << "pseudosum: " << pseudosum << endl;
  delete aPseudoHeader;
  // Create the TCP segment.
  TCPHeader* tcpHeader = (TCPHeader *) anAnswer;
  tcpHeader->sourcePort = HILO(myConnection->myPort);
  tcpHeader->destinationPort = HILO(myConnection->hisPort);
  tcpHeader->sequenceNumber = LHILO(myConnection->sendNext);
  if((theFlags & ACK) != 0){
    tcpHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
  } else
  {
    tcpHeader->acknowledgementNumber = LHILO(0);
  }
  tcpHeader->headerLength = 0x50;
  tcpHeader->flags = theFlags;
  tcpHeader->windowSize = HILO(myConnection->receiveWindow);
  tcpHeader->checksum = 0;
  tcpHeader->urgentPointer = 0;

  // Calculate the final checksum.
  tcpHeader->checksum = calculateChecksum(anAnswer,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.
  myAnswerChain->answer(anAnswer,
                        totalSegmentLength);
  // Deallocate the dynamic memory
  delete anAnswer;
}

//----------------------------------------------------------------------------
//
void
TCPSender::sendData(byte*  theData,
                    udword theLength)
{
  //TODO totalSegmentLength is never allowed to be larger than 1472, then we
  // to send several packages!
  // Send a data segment. PSH and ACK flags are set.
  trace << "TCPSender::sendData" << endl;
  byte totalSegmentLength = TCP::tcpHeaderLength+theLength;

  byte* anAnswer = new byte[totalSegmentLength];
  // Calculate the pseudo header checksum
  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  delete aPseudoHeader;

  // Create the TCP segment.
  TCPHeader* tcpHeader = (TCPHeader *) anAnswer;
  tcpHeader->sourcePort = HILO(myConnection->myPort);
  tcpHeader->destinationPort = HILO(myConnection->hisPort);
  tcpHeader->sequenceNumber = LHILO(myConnection->sendNext);
  tcpHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
  tcpHeader->headerLength = 0x50;
  tcpHeader->flags = PSH|ACK;
  tcpHeader->windowSize = HILO(myConnection->receiveWindow);
  tcpHeader->checksum = 0;
  tcpHeader->urgentPointer = 0;

  memcpy(anAnswer+TCP::tcpHeaderLength, theData, theLength);

  // Calculate the final checksum.
  tcpHeader->checksum = calculateChecksum(anAnswer,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.
  myAnswerChain->answer(anAnswer,
                        totalSegmentLength);
  // Deallocate the dynamic memory
  delete anAnswer;
}

//----------------------------------------------------------------------------
//
TCPInPacket::TCPInPacket(byte*           theData,
                         udword          theLength,
                         InPacket*       theFrame,
                         IPAddress&      theSourceAddress):
        InPacket(theData, theLength, theFrame),
        mySourceAddress(theSourceAddress)
{
}

//----------------------------------------------------------------------------
//
void
TCPInPacket::decode()
{
  // cout << "tcp DECODE" << endl;
  // Extract the parameters from the TCP header which define the
  // connection.
  TCPHeader* tcpHeader = (TCPHeader *) myData;
  mySourcePort = HILO(tcpHeader->sourcePort);
  // cout << "sourcePort: " << mySourcePort << endl;
  myDestinationPort = HILO(tcpHeader->destinationPort);
  // cout << "destPort: " << myDestinationPort << endl;
  mySequenceNumber = LHILO(tcpHeader->sequenceNumber);
  // cout << "sequenceNumber: " << mySequenceNumber << endl;
  myAcknowledgementNumber = LHILO(tcpHeader->acknowledgementNumber);
  // cout << "ackNumber: " << myAcknowledgementNumber << endl;

  TCPConnection* aConnection =
           TCP::instance().getConnection(mySourceAddress,
                                         mySourcePort,
                                         myDestinationPort);
  if (!aConnection)
  {
    // Establish a new connection.
    aConnection =
         TCP::instance().createConnection(mySourceAddress,
                                          mySourcePort,
                                          myDestinationPort,
                                          this);
    if ((tcpHeader->flags & SYN) != 0)
    {
      // State LISTEN. Received a SYN flag.
      // cout << "TCPInPacket::ListenState" << endl;
      aConnection->Synchronize(mySequenceNumber);
    }
    else
    {
      trace << "TCPInPacket::NotListenState" << endl;
      // State LISTEN. No SYN flag. Impossible to continue.
      aConnection->Kill();
    }
  }
  else
  {
    // cout << "TCPInPacket::OkConnection" << endl;
    // Connection was established. Handle all states.
    if((tcpHeader->flags & ACK) != 0)
    {
      aConnection->Acknowledge(myAcknowledgementNumber);
      // cout << "ACK: " << myAcknowledgementNumber << endl;
    }
    if((tcpHeader->flags & FIN) != 0)
    {
      aConnection->NetClose();
      // aConnection->mySocket->Close();
    }
    if((tcpHeader->flags & PSH) != 0)
    {
      aConnection->Receive(mySequenceNumber, myData+headerOffset(), myLength-headerOffset());
    }
  }
}

//----------------------------------------------------------------------------
//
void
TCPInPacket::answer(byte* theData, udword theLength)
{
  // STUFF THINGS SHOULD HAPPEN HERE
}

//----------------------------------------------------------------------------
//
uword
TCPInPacket::headerOffset()
{
  return TCP::tcpHeaderLength;
}

//----------------------------------------------------------------------------
//
InPacket*
TCPInPacket::copyAnswerChain()
{
  return myFrame->copyAnswerChain();
}

//----------------------------------------------------------------------------
//
TCPPseudoHeader::TCPPseudoHeader(IPAddress& theDestination,
                                 uword theLength):
        sourceIPAddress(IP::instance().myAddress()),
        destinationIPAddress(theDestination),
        zero(0),
        protocol(6)
{
  tcpLength = HILO(theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPPseudoHeader::checksum()
{
  return calculateChecksum((byte*)this, 12, 0);
}

//----------------------------------------------------------------------------
//
TimeWaitTimer::TimeWaitTimer(TCPConnection* theConnection):
  myTCPConnection(theConnection)
{
  // Constructor: initiate myBlinkTime
  myWaitTime = Clock::seconds*2;
}

//----------------------------------------------------------------------------
//
void
TimeWaitTimer::start()
{
  // Start timer
  this->timeOutAfter(myWaitTime);
  cout << "timerStart" << endl;
}

//----------------------------------------------------------------------------
//
void
TimeWaitTimer::timeOut()
{
  // Kill the connection after timeout
  cout << "timerEnd" << endl;
  myTCPConnection->Kill();
  delete this;
}

/****************** END OF FILE tcp.cc *************************************/
