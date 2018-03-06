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

// #define D_SOCKET_CORE
#ifdef D_SOCKET_CORE
#define coreOut cout
#else
#define coreOut if(false) cout
#endif

TCPSocket::TCPSocket(TCPConnection* theConnection):
  myConnection(theConnection),
  myReadSemaphore(Semaphore::createQueueSemaphore("myReadSemaphore",0)),
  myWriteSemaphore(Semaphore::createQueueSemaphore("myWriteSemaphore",0)),
  eofFound(false)
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
  // byte* toSend = new byte[theLength];
  // memcpy(toSend, theData, theLength);
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
}

void
TCPSocket::Close()
{
  // Close the socket. When called the socket will close the connection by
  // calling myConnection->AppClose() and delete itself.
  // cout << "Socket for: " << myConnection->hisPort << " closed." << endl;
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
  coreOut << "Core::socketDataSent " << ax_coreleft_total() << endl;
  myWriteSemaphore->signal(); // The data has been acknowledged
  // cout << "transmitQueue: " << myConnection->transmitQueue << endl;
}

void
TCPSocket::socketEof()
{
  // Called from TCP when a FIN has been received in the established state.
  // Sets the eofFound flag and signals the read semaphore.
  eofFound = true;
  myReadLength = 0;
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
    coreOut << "Core::doit begin " << ax_coreleft_total() << endl;
    aData = mySocket->Read(aLength);
    // cout << "SimpleApplication::aData: " << aData << ":" << aLength << endl;
    if (aLength > 0 && aData != NULL)
    {
      // this is needed as Write will delete aData as soon as the call finishes
      // byte* first = new byte[1];
      char first = *aData;
      // cout << "first2: " << first2 << endl;
      // memcpy(first, aData, 1);
      mySocket->Write(aData, aLength);
      cout << "Core::Write() " << ax_coreleft_total() << endl;
      if (first == 'q')
      {
        done = true;
      }
      else if(first == 's')
      {
        cout << "s" << endl;
        byte* anAnswer = new byte[10000];
        char sequence[] = "abcdefghijklmnopqrstuvwxyz0123456789BASE";
        // cout << "strlen(sequence): " << strlen(sequence) << endl;
        for(int i = 0; i < 250; i++)
        {
          memcpy(anAnswer+i*strlen(sequence), sequence, strlen(sequence));
        }
        mySocket->Write(anAnswer, 10000);
        delete[] anAnswer;
      }
      else if (first == 'r')
      {
        cout << "r" << endl;
        byte* anAnswer = new byte[100000];
        char sequence[] = "abcdefghijklmnopqrstuvwxyz0123456789BASE";
        // cout << "strlen(sequence): " << strlen(sequence) << endl;
        for(int i = 0; i < 2500; i++)
        {
          memcpy(anAnswer+i*strlen(sequence), sequence, strlen(sequence));
        }
        for (int i = 0; i < 10; i++) {
          mySocket->Write(anAnswer, 100000);
        }
        delete[] anAnswer;
      }
      // delete first;
      delete[] aData;
      coreOut << "Core::doit end " << ax_coreleft_total() << endl;
    }
  }
  coreOut << "Core::doit close " << ax_coreleft_total() << endl;
  mySocket->Close();
}
