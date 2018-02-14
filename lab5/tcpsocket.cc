#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
#include "timr.h"
}

#include "threads.hh"
#include "iostream.hh"
#include "tcp.hh"

TCPSocket::TCPSocket(TCPConnection* theConnection):
  myConnection(theConnection),
  myReadSemaphore(Semaphore::createQueueSemaphore("myReadSemaphore",0)),
  myWriteSemaphore(Semaphore::createQueueSemaphore("myWriteSemaphore",0))
{
  // Constructor. The socket is created by class TCP when a connection is
  // established. create the semaphores
}

TCPSocket::~TCPSocket()
{
  // Destructor. destroy the semaphores.
  delete myReadSemaphore;
  delete myWriteSemaphore;
}

//
// Interface to application
//

byte*
TCPSocket::Read(udword& theLength)
{
  // Read incoming data segment. This call will block on the read semaphore
  // until data is available.
  // theLength returns the amount of data read.
  myReadSemaphore->wait(); // Wait for available data
  theLength = myReadLength;
  byte* aData = myReadData;
  myReadLength = 0;
  myReadData = 0;
  return aData;
}

bool
TCPSocket::isEof()
{
  // True if a FIN has been received from the remote host.
  return eofFound;
}

void
TCPSocket::Write(byte* theData, udword theLength)
{
  // Write data to remote host. This call will block on the write semaphore
  // until all the data has been sent to, and acknowledged by, the remote host.
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
}

void
TCPSocket::Close()
{
  // Close the socket. When called the socket will close the connection by
  // calling myConnection->AppClose() and delete itself.
  myConnection->AppClose();
}

//
// Interface to TCPConnection
//

void
TCPSocket::socketDataReceived(byte* theData, udword theLength)
{
  // Called from TCP when data has been received. signals the read semaphore.
  myReadData = new byte[theLength];
  memcpy(myReadData, theData, theLength);
  myReadLength = theLength;
  myReadSemaphore->signal(); // Data is available
}

void
TCPSocket::socketDataSent()
{
  // Called from TCP when all data has been sent and acknowledged. signals
  // the write semaphore.
  myWriteSemaphore->signal(); // The data has been acknowledged
}

void
TCPSocket::socketEof()
{
  // Called from TCP when a FIN has been received in the established state.
  // Sets the eofFound flag and signals the read semaphore.
  eofFound = true;
  myReadSemaphore->signal();
}




SimpleApplication::SimpleApplication(TCPSocket* theSocket):
  mySocket(theSocket)
{
  // Constructor. The application is created by class TCP when a connection is
  // established.
}

void
SimpleApplication::doit()
{
  // Gets called when the application thread begins execution.
  // The SimpleApplication job is scheduled by TCP when a connection is
  // established.
  udword aLength;
  byte* aData;
  bool done = false;
  while (!done && !mySocket->isEof())
  {
    aData = mySocket->Read(aLength);
    if (aLength > 0)
    {
      mySocket->Write(aData, aLength);
      if ((char)*aData == 'q')
      {
        done = true;
      }
      delete aData;
    }
  }
  mySocket->Close();
}
