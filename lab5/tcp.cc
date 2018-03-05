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

// #define D_TCP_CORE
#ifdef D_TCP_CORE
#define coreOut cout
#else
#define coreOut if(false) cout
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
  myRetransmitTimer = new RetransmitTimer(this, Clock::seconds*1);
  myTimeWaitTimer = new TimeWaitTimer(this);
  sentMaxSeq = 0;
  queueLength = 0;
  firstSeq = 0;
}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  coreOut << "Core::~TCPConnection begin " << ax_coreleft_total() << endl;
  trace << "TCP connection destroyed" << endl;
  if(mySocket != NULL){
    delete mySocket; // ========= TESTING ===========
  }
  if(myTCPSender != NULL) {
    delete myTCPSender;
  }
  myRetransmitTimer->cancel();
  delete myRetransmitTimer;
  delete myTimeWaitTimer;
  // if(transmitQueue != NULL){
    // delete[] transmitQueue;
  // }
  coreOut << "Core::~TCPConnection end " << ax_coreleft_total() << endl;
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
      // theConnection->firstSeq = theConnection->sendNext;
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
  // cout << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK
  theConnection->receiveNext++;
  theConnection->myTCPSender->sendFlags(ACK);

  // Go to NetClose wait state, inform application
  theConnection->mySocket->socketEof();
  theConnection->myState = CloseWaitState::instance();
  // cout << "Entered CloseWaitState" << endl;

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
  // MUCHO IMPORTANTO PRINTO
  // cout << "if info: " << (theConnection->firstSeq + theConnection->queueLength) << ":" << theAcknowledgementNumber << endl;
  if((theConnection->firstSeq + theConnection->queueLength) == theAcknowledgementNumber){
    cout << "All data sent" << endl;
    theConnection->firstSeq = 0;
    theConnection->queueLength = 0;
    theConnection->myRetransmitTimer->cancel();
    theConnection->mySocket->socketDataSent();
  } else if (theConnection->firstSeq > 0) {
    // cout << "EstablishedState::Acknowledge not all data sent" << endl;
    theConnection->myTCPSender->sendFromQueue();
  } else {
    //NEVER SHALL I EVER PUT SOMETHING HERE AGAIN
  }


  // THIS BREAKS STUFF
  // if(theConnection->sentMaxSeq > theConnection->sendNext)
  // {
  //   cout << "ACK of retransmisson received" << endl;
  //   theConnection->sendNext = theAcknowledgementNumber;
  //   theConnection->theOffset = theConnection->sendNext - theConnection->firstSeq;
  // }
  // if(theConnection->queueLength > (theConnection->theOffset + theConnection->theSendLength))
  // {
  //   theConnection->myTCPSender->sendFromQueue();
  // }
  // else if (theAcknowledgementNumber >= theConnection->sentMaxSeq)
  // {
  //   theConnection->myRetransmitTimer->cancel();
  //   theConnection->mySocket->socketDataSent();
  //   cout << "All data sent" << endl;
  // } else
  // {
  //   // cout << "we are missing a segment in transmission" << endl;
  // }
}

//----------------------------------------------------------------------------
//
void
EstablishedState::Send(TCPConnection* theConnection,
                       byte*  theData,
                       udword theLength)
{
  // Send outgoing data
  theConnection->transmitQueue = theData;
  theConnection->queueLength = theLength;
  theConnection->firstSeq = theConnection->sendNext;

  // cout << "TCPConnection::transmitQueue: " << transmitQueue << endl;
  // cout << "EstablishedState::Send sendFromQueue() before" << endl;
  theConnection->myTCPSender->sendFromQueue();
  // cout << "EstablishedState::Send sendFromQueue() after" << endl;

  // theConnection->myTCPSender->sendData(theData, theLength);
}

void
EstablishedState::AppClose(TCPConnection* theConnection)
{
  theConnection->myState = FinWait1State::instance();
  theConnection->myTCPSender->sendFlags(FIN|ACK);
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
  theConnection->myRetransmitTimer->cancel();
  theConnection->myTCPSender->sendFlags(FIN|ACK);
  theConnection->myState = LastAckState::instance();
  // cout << "CloseWaitState::AppClose Done" << endl;
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
  // cout << "LastAckState::Acknowledge Done" << endl;
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
    theConnection->myRetransmitTimer->cancel();
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
  theConnection->myTimeWaitTimer->start();
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
  delete[] anAnswer;
}

//----------------------------------------------------------------------------
//
// static int throwAwayCounter = 0;
void
TCPSender::sendData(byte*  theData,
                    udword theLength)
{
  coreOut << "Core::sendData " << ax_coreleft_total() << endl;
  //TODO totalSegmentLength is never allowed to be larger than 1460, then we
  // to send several packages!
  // Send a data segment. PSH and ACK flags are set.
  trace << "TCPSender::sendData" << endl;
  uword totalSegmentLength = TCP::tcpHeaderLength+theLength;
  // cout << "totalSegmentLength: " << totalSegmentLength << endl;

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
  // if(throwAwayCounter++ < 30){
    myAnswerChain->answer(anAnswer, totalSegmentLength);
    // cout << "send next: " << myConnection->sendNext << endl;
  // } else {
    // throwAwayCounter = 0;
    // cout << "drop the bass: " << myConnection->sendNext << endl;
  // }

  // Deallocate the dynamic memory
  myConnection->sendNext += theLength;
  // myConnection->sentMaxSeq = myConnection->sendNext;
  delete[] anAnswer;
  coreOut << "Core::sendData " << ax_coreleft_total() << endl;
}

//----------------------------------------------------------------------------
//
void
TCPSender::sendFromQueue()
{
  coreOut << "Core::sendFromQueue begin " << ax_coreleft_total() << endl;

  // maintain the queue and send smaller sets of data

  // cout << "sendNext:firstSeq" << myConnection->sendNext << ":" << myConnection->firstSeq << endl;
  // cout << "sentMaxSeq: " << myConnection->sentMaxSeq << endl;
  myConnection->theOffset = myConnection->sendNext - myConnection->firstSeq;
  myConnection->theFirst = myConnection->transmitQueue + myConnection->theOffset;
  udword remaining = myConnection->queueLength - myConnection->theOffset;
  udword toSend = MIN(remaining, 1460);
  udword remainingWindowSize = myConnection->myWindowSize -
    (myConnection->sendNext - myConnection->sentUnAcked);

  udword theSendLength = MIN(toSend, remainingWindowSize);
  // cout << "theSendLength: " << theSendLength << endl;
  if(theSendLength > 0)
  {
    // cout << "theOffset: " << myConnection->theOffset << endl;
    // cout << "theFirst: " << unsigned(myConnection->theFirst) << endl;
    // cout << "remaining: " << remaining << endl;
    // cout << "remainingWindowSize: " << remainingWindowSize << endl;
    sendData(myConnection->theFirst, theSendLength);
    myConnection->myRetransmitTimer->start();
    if (myConnection->sendNext >= myConnection->sentMaxSeq)
    {
      myConnection->sentMaxSeq = myConnection->sendNext;
    } else
    {
      myConnection->sendNext = myConnection->sentMaxSeq;
    }
    sendFromQueue();
    // if(myConnection->sentMaxSeq > myConnection->sendNext)
    // {
    //   // cout << "Retransmission" << endl;
    //   myConnection->theSendLength = toSend;
    //   myConnection->sendNext = myConnection->sentMaxSeq + 1;
    //   // cout << "Retransmitted" << endl;
    // } else
    // {
    //   // cout << "sendFromQueue()" << endl;
    //   myConnection->theSendLength = toSend;
    //
    //   byte* temp = new byte[myConnection->theSendLength];
    //   memcpy(temp, myConnection->theFirst, myConnection->theSendLength);
    //   cout << "theSendLength: " << myConnection->theSendLength << endl;
    //   cout << "message: " << temp << endl;
    //   delete[] temp;
    //
    //   sendData(myConnection->theFirst, myConnection->theSendLength);
    //   // This sets the max value that has been sent, hence "- 1" from the next to send
    //   myConnection->sentMaxSeq = myConnection->sendNext - 1;
    //   myConnection->myRetransmitTimer->start();
    // }
    // // sendFromQueue();
  }
  coreOut << "Core::sendFromQueue end " << ax_coreleft_total() << endl;
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
    aConnection->myWindowSize = HILO(tcpHeader->windowSize);
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
  // cout << "timerStart" << endl;
  coreOut << "Core::timerStart " << ax_coreleft_total() << endl;
}

//----------------------------------------------------------------------------
//
void
TimeWaitTimer::timeOut()
{
  // Kill the connection after timeout
  // cout << "timerEnd" << endl;
  coreOut << "Core::timerEnd " << ax_coreleft_total() << endl;
  myTCPConnection->Kill();
}

//----------------------------------------------------------------------------
//
RetransmitTimer::RetransmitTimer(TCPConnection* theConnection,
                                  Duration retransmitTime):
  myConnection(theConnection),
  myRetransmitTime(retransmitTime)
{

}

//----------------------------------------------------------------------------
//
void
RetransmitTimer::start()
{
  // Start timer
  // cout << "RetransmitTimerSTART" << endl;
  this->timeOutAfter(myRetransmitTime);
}

void
RetransmitTimer::cancel()
{
  this->resetTimeOut();
}

//----------------------------------------------------------------------------
//
void
RetransmitTimer::timeOut()
{
  cout << "RetransmitTimerEND" << endl;
  myConnection->sendNext = myConnection->sentUnAcked + 1;
  myConnection->myTCPSender->sendFromQueue();
}


/****************** END OF FILE tcp.cc *************************************/
