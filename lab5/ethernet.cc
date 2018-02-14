/*!***************************************************************************
*!
*! FILE NAME  : Ethernet.cc
*!
*! DESCRIPTION: Handles the Ethernet layer
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "msg.h"
#include "osys.h"
#include "system.h"
#include "etrax.h"
#include "xprintf.h"
}

#include "iostream.hh"
#include "frontpanel.hh"
#include "ethernet.hh"
#include "llc.hh"
#include "sp_alloc.h"

//#define D_ETHER
#ifdef D_ETHER
#define trace cout
#else
#define trace if(false) cout
#endif

/********************** LOCAL VARIABLE DECLARATION SECTION *****************/

static bool processingPacket    = FALSE;    /* TRUE when LLC has a packet */
static bool bufferFullCondition = FALSE;    /* TRUE when buffer is full   */
static bool firstInterrupt = TRUE;

/****************** Ethernet DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
Ethernet::Ethernet()
{
  trace << "Ethernet created." << endl;
  nextRxPage = 0;
  nextTxPage = 0;
  processingPacket = FALSE;

  // STUFF: Set myEthernetAddress to your "personnummer" here!
  myEthernetAddress = new EthernetAddress(0x00,0x95,0x08,0x02,0x17,0x31);

  this->initMemory();
  this->initEtrax();
  cout << "My node address is " << this->myAddress() << endl;
}

//----------------------------------------------------------------------------
//
Ethernet&
Ethernet::instance()
{
  static Ethernet myInstance;
  return myInstance;
}

//----------------------------------------------------------------------------
//
const EthernetAddress&
Ethernet::myAddress()
{
  return *myEthernetAddress;
}

//----------------------------------------------------------------------------
//
void
Ethernet::initMemory()
{
  int page;
  BufferPage* aPointer;

  trace << "initMemory" << endl;

  // STUFF: Set status byte on each page to 0 here!
  // Shall be done for both rx buffer and tx buffer.

  //TX
  aPointer = (BufferPage *) txStartAddress;
  for (page = 0; page < txBufferPages; page++) {
    aPointer->statusCommand = 0x00;
    aPointer++;
  }

  //RX
  aPointer = (BufferPage *) rxStartAddress;
  for (page = 0; page < rxBufferPages; page++) {
    aPointer->statusCommand = 0x00;
    aPointer++;
  }
}

//----------------------------------------------------------------------------
//
void
Ethernet::initEtrax()
{
  trace << "initEtrax" << endl;
  DISABLE_SAVE();

  *(volatile byte *)R_TR_MODE1 = (byte)0x20;
  *(volatile byte *)R_ANALOG   = (byte)0x00;

  *(volatile byte *)R_TR_START = (byte)0x00;
  // First page in the transmit buffer.
  *(volatile byte *)R_TR_POS   = (byte)0x00;
  // transmit buffer position.
  //                         |                     |
  //           20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
  // R_TR_START                0  0  0  0  0  0 0 0                 first page is 0
  // R_TR_POS   0  0  0  0  0  0  0  0                              0x40000000
  //
  // R_TR_START = 0x00 = bit 15:8 of txStartAddress.
  // R_TR_POS   = 0x00 = bit 20:13 of txStartAddress.

  *(volatile byte *)R_RT_SIZE = (byte)0x0a;
  // rx and tx buffer size 8kbyte (page 41, 43)
  // rx: xxxxxx10 OR tx: xxxx10xx = xxxx1010

  *(volatile byte *)R_REC_END  = (byte)0xff;
  // Last page _in_ the receive buffer.
  *(volatile byte *)R_REC_POS  = (byte)0x04;
  // receive buffer position.
  //                         |                     |               |
  // Bit:      20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
  // REC_END                   1  1  1  1  1  1 1 1                 last page in buffer is 31 (!)
  // R_REC_POS  0  0  0  0  0  1  0  0                              0x40008000
  //
  // REC_END = 0x3f = bit 15:8 of (rxStartAddress + 31*256).
  // R_REC_POS = 0x01 = bit 20:13 of rxStartAddress.

  *(volatile byte *)R_GA0 = 0xff;     /* Enable ALL multicast */
  *(volatile byte *)R_GA1 = 0xff;
  *(volatile byte *)R_GA2 = 0xff;
  *(volatile byte *)R_GA3 = 0xff;
  *(volatile byte *)R_GA4 = 0xff;
  *(volatile byte *)R_GA5 = 0xff;
  *(volatile byte *)R_GA6 = 0xff;
  *(volatile byte *)R_GA7 = 0xff;

  // Reverse the bit order of the ethernet address to etrax.
  byte aAddr[EthernetAddress::length];
  myEthernetAddress->writeTo((byte*)&aAddr);

  byte aReverseAddr[EthernetAddress::length];

  for (int i = 0; i < EthernetAddress::length; i++)
  {
    aReverseAddr[i] = 0;
    for (int j = 0; j < 8; j++)
    {
      aReverseAddr[i] = aReverseAddr[i] << 1;
      aReverseAddr[i] = aReverseAddr[i] | (aAddr[i] & 0x01);
      aAddr[i] = aAddr[i] >> 1;
    }
  }

  *(volatile byte *)R_MA0 = aReverseAddr[0];     /* Send this first */
  *(volatile byte *)R_MA1 = aReverseAddr[1];
  *(volatile byte *)R_MA2 = aReverseAddr[2];
  *(volatile byte *)R_MA3 = aReverseAddr[3];
  *(volatile byte *)R_MA4 = aReverseAddr[4];
  *(volatile byte *)R_MA5 = aReverseAddr[5];      /* Send this last */

  *(volatile byte *)R_TR_MODE1  = 0x30;   /* Ethernet mode, no loop back */
  *(volatile byte *)R_ETR_MASKC = 0xbf;   /* Disable all other etr interrupts*/
  *(volatile byte *)R_BUF_MASKS = 0x0f;   /* Enable all buf. interrupts */

  *(volatile byte *)R_TR_MODE2 = 0x14;    /* Start network interface */

  do { asm volatile ("movem sp,[0xc0002fff]");} while (FALSE);

  *(volatile byte *)R_REC_MODE = 0x06;    /* Clear receiver interrupts */

  RESTORE();
}

//----------------------------------------------------------------------------
//
extern "C" void
ethernet_interrupt()
{
  byte bufferStatus = *(volatile byte *)R_BUF_STATUS;

  if (firstInterrupt)
  {
    for(int i = 0; i < 100000; i++) {}
    firstInterrupt = FALSE;
  }
  /* Therer are four possible interrupts to handle here: */
  /* 1. Packet received                                  */
  /* 2. Receive buffer full                              */
  /* 3. Packet transmitted                               */
  /* 4. Exessive retry                                   */
  /* They have one bit each in the R_BUF_STATUS register */

  /*-------------------------------------------------------------------------*/
  /* 1. Packet received interrup. */
  if (BITTST(bufferStatus, BUF__PACKET_REC))
  {
    // FrontPanel::instance().packetReceived();
    // printf("packet recieved");
    *(volatile byte *)R_REC_MODE = 0x04;
    // cout << "Core " << ax_coreleft_total() << endl;
    // Ack packet received interupt
    if (!processingPacket)
    {
      if (Ethernet::instance().getReceiveBuffer())
      {
        processingPacket = TRUE;
        os_int_send(OTHER_INT_PROG, THREAD_MAIN, THREAD_PACKET_RECEIVED,
                    NO_DATA, 0, NULL);
      }
      else
      {
        processingPacket = FALSE;
      }
    }
  }

  /*--------------------------------------------------------------------------*/
  /* 2. When we receive a buffer full interrupt, we must not clear the inter- */
  /* rupt until there is more space in the receive buffer. This is done in    */
  /* return_rxbuf(). We clear the buffer full interrupt mask bit to disable   */
  /* more interrupts until things are taken care of in return_rxbuf().        */
  if (BITTST(bufferStatus, BUF__BUFFER_FULL))
  {
    cout << "buffer full" << endl;
    *(volatile byte *)R_BUF_MASKC = 0x04;
    /* Disable further interrupts until we */
    /* are given space in return_rxbuf()   */
    *(volatile byte*)R_REC_MODE = 0x02;
    /* Acknowledge buffer full interrupt */
    bufferFullCondition = TRUE;
  }

  /*--------------------------------------------------------------------------*/
  /* 3. This interrupt means that all packets in ring buffer have been sent   */
  if (BITTST(bufferStatus, BUF__PACKET_TR))
  {
    // printf("buffer sent");
    *(volatile byte *)R_TR_CMD = 0;
    /* Clear interrupt, nothing else */
  }

  /*--------------------------------------------------------------------------*/
  /* 4. If we get an excessive retry interrupt, we reset the transmitter.     */
  if (BITTST(bufferStatus, BUF__ABORT_INT))
  {
    // printf("buffer transmit reset");
    Ethernet::instance().resetTransmitter();
  }
}

//----------------------------------------------------------------------------
//
bool
Ethernet::getReceiveBuffer()
{
  // STUFF: lots to do here!
  // The first page in the received packet is given by nextRxPage.
  // The first page starts at address 'rxStartAddress + (nextRxPage * 256)',
  // right?
  udword nextRxAddress = rxStartAddress + (nextRxPage * 256);

  BufferPage* nextRxBufferedPage = (BufferPage *)nextRxAddress;

  if ((nextRxBufferedPage->statusCommand == 0x01) || // Packet available
      (nextRxBufferedPage->statusCommand == 0x03))   // Packet available and buffer full
  {
    // use endptr to find out where the packet ends, and if it is wrapped.
    dword length = endPtrOffset + nextRxBufferedPage->endPointer - (udword)nextRxBufferedPage->data;
    // cout << "gRB length: " << length << endl;
    // cout << "gRB endPointer: " << nextRxBufferedPage->endPointer << endl;
    // cout << "gRB data: " << (udword)nextRxBufferedPage->data << endl;
    if (length >= 0)
    {
      // one chunk of data
      data1   = (byte *) nextRxBufferedPage->data;
      length1 = length;
      data2   = NULL;
      length2 = 0;
    }
    else
    {
      // two chunks of data
      data1   = (byte *) nextRxBufferedPage->data;
      length1 = (rxStartAddress+rxBufferSize) - (udword)nextRxBufferedPage->data;
      data2   = (byte *) rxStartAddress;
      length2 = endPtrOffset + nextRxBufferedPage->endPointer - rxStartAddress;
    }
    //  cout << "getReceiveBuffer: true" << endl;
    return true;
  }
#ifdef D_ETHER
  printf("No packet found.\r\n"); // cannot use cout in interrupt context...
#endif
  //  cout << "getReceiveBuffer: false" << '\n';
  return false;
}

//----------------------------------------------------------------------------
//
void
Ethernet::returnRXBuffer()
{
  DISABLE_SAVE();
  BufferPage* aPage = (BufferPage *)(rxStartAddress + (nextRxPage * 256));
  /* Pointer to beginning of packet to return. nextRxPage is the packet just */
  /* consumed by LLC.                                                        */

  uword endInBuffer = aPage->endPointer - rxBufferOffset;
  // end pointer from start of receive buffer. Wraps at rxBufferSize.

  if ((aPage->endPointer - rxBufferOffset) < (nextRxPage * 256))
  {
    trace << "Delete wrap copy" << endl;
    delete [] wrappedPacket;
  }

  // The R_REC_END should point to the begining of the page addressed by
  // endInBuffer.
  uword recEnd = (endInBuffer + rxBufferOffset) >> 8;
  *(volatile byte *)R_REC_END = recEnd;

  trace << "RetRx: aPage " << hex << (udword)aPage
        << " endMarker " << endInBuffer
        << " recEnd " << recEnd
        << endl;
  if (bufferFullCondition)
  {
    trace << "buffer was full" << endl;
    *(volatile byte *)R_BUF_MASKS = 0x04;
    /* Enable this interrupt again. */
    bufferFullCondition = FALSE;
  }

  // Adjust nextRxPage
  uword endPage = endInBuffer >> 8;
  nextRxPage = (endPage + 1) % rxBufferPages;
  trace << " endPage " << hex << endPage << " nextRxPage "
        << (uword)nextRxPage << endl;

  // Process next packet (if there is one ready)
  if (this->getReceiveBuffer())
  {
    trace << "Found pack in retrx" << endl;
    processingPacket = TRUE;
    RESTORE();
    os_send(OTHER_INT_PROG, THREAD_MAIN, THREAD_PACKET_RECEIVED,
                NO_DATA, 0, NULL);
  }
  else
  {
    processingPacket = FALSE;
    RESTORE();
  }
}

//----------------------------------------------------------------------------
//
void
Ethernet::decodeReceivedPacket()
{
  trace << "Found packet at:" << hex << (udword)data1 << dec << endl;
  // STUFF: Blink packet LED
  FrontPanel::instance().packetReceived();
  // STUFF: Create an EternetInPacket, two cases:
  EthernetInPacket* ethPacket;
  if (data2 == NULL)
  {
    // STUFF: Create an EternetInPacket
    ethPacket = new EthernetInPacket(data1, length1, 0);
  }
  else
  {
    trace << "Wrap copy" << endl;
    // When a wrapped buffer is received it will be copied into linear memory
    // this will simplify upper layers considerably...
    // There can be only one mirrored wrapped packet at any one time.
    wrappedPacket = new byte[length1 + length2];
    memcpy(wrappedPacket, data1, length1);
    memcpy((wrappedPacket + length1), data2, length2);
    // STUFF: Create an EternetInPacket
    ethPacket = new EthernetInPacket(wrappedPacket, length1+length2, 0);
  }
  // STUFF: Create and schedule an EthernetJob to decode the EthernetInPacket
  EthernetJob* ethJob = new EthernetJob(ethPacket);
  Job::schedule(ethJob);
}

//----------------------------------------------------------------------------
//
void
Ethernet::transmittPacket(byte *theData, udword theLength)
{
  trace << "transmitt" << endl;
  // cout << "transmittPacket" << endl;
  // Make sure the packet fits in the transmitt buffer.

  /* If the packet ends at a 256 byte boundary, the next buffer is skipped  */
  /* by the Etrax. E.g. a 508 byte packet will use exactly two buffers, but */
  /* buffer_use should be 3.                                                */
  uword nOfBufferPagesNeeded = ((theLength + commandLength) >> 8) + 1;

  word availablePages;
  // availablePages can be negative...
  // Make sure there is room for the packet in the transmitt buffer.
  uword timeOut = 0;
  do
  {
    /* Transmitter is now sending at page r_tr_start. */
    /* May not be the same as nextTxPage */
    /* Bits <15:8>, page number */
    byte r_tr_start  = *(volatile byte*)R_TR_START;

    // Set sendBoundary to the page before r_tr_start page
    word sendBoundary =  r_tr_start - 1;
    if (sendBoundary < 0)
    {
      sendBoundary = txBufferPages - 1;  /* Set to last page */
    }

    availablePages = sendBoundary - nextTxPage;
    /* In 256 byte buffers */
    if (availablePages < 0)
    {
      availablePages += txBufferPages;
      /* Boundary index is lower than nextTxPage */
    }
#ifdef ETHER_D
    ax_printf("sendBoundary:%2d, nextTxPage:%2d, availablePages:%2d, "
              "packet:%4d bytes = %d pages\n", sendBoundary,
              nextTxPage, availablePages, theLength, nOfBufferPagesNeeded);
#endif
    if((uword)availablePages < nOfBufferPagesNeeded)
    {
      cout << "Tx buffer full, waiting." << endl;
      os_delay(1);                                       /* One tic = 10 ms */
      timeOut++;
      /* If nothing happens for 3 seconds then do something drastical.      */
      if (timeOut > 3 * WAIT_ONE_SECOND)                       /* 300 tics */
      {
        cout << "Transmit buffer full for 3 seconds. Resetting.\n";
        this->resetTransmitter();
        timeOut = 0;
      }
    }
  }
  while((uword)availablePages < nOfBufferPagesNeeded);

  // There is room for the packet in the transmitt buffer
  // Copy the packet to the transmitt buffer
  // Remember: Undersized packets must be padded! Just advance the end pointer
  // accordingly.

  // STUFF: Find the first available page in the transmitt buffer
  udword nextTxAddress = txStartAddress + (256 * nextTxPage);
  BufferPage* nextTxBufferPage = (BufferPage *) nextTxAddress;

  if (nextTxPage + nOfBufferPagesNeeded <= txBufferPages)
  {
    // STUFF: Copy the packet to the transmitt buffer
    // Simple case, no wrap
    // Pad undersized packets
    // cout << "not wrapped" << endl;
    // cout << "theLength: " << theLength << endl;
    // cout << "theLength comp: " << Ethernet::ethernetHeaderLength + minPacketLength << endl;
    dword padLength = theLength - (Ethernet::ethernetHeaderLength + minPacketLength);
    // if(theLength >= (Ethernet::ethernetHeaderLength + minPacketLength)) {
    if(padLength >= 0) {
        // cout << "no padd" << endl;
        memcpy(nextTxBufferPage->data, theData, theLength);
        // nextTxBufferPage->data = theData;
        nextTxBufferPage->endPointer = (nextTxAddress - txStartAddress) + commandLength + theLength - 1;
    }
    else
    {
        memcpy(nextTxBufferPage->data, theData, theLength);
        nextTxBufferPage->endPointer = (nextTxAddress - txStartAddress) + commandLength + theLength - 1 - padLength;
        // dword packetLength = commandLength + theLength - padLength;
        // cout << "padded: " << theLength <<  " to: " << packetLength << endl;
    }
  }
  else
  {
    trace << "Wraped transmission" << endl;
    // cout << "Wrapped" << endl;
    // STUFF: Copy the two parts into the transmitt buffer, cannot be undersized
    udword length1 = (txStartAddress + txBufferSize) - nextTxAddress - commandLength;
    udword length2 = theLength - length1;
    // cout << "Wrapped lengths: " << length1 << " " << length2 << endl;
    memcpy(nextTxBufferPage->data, theData, length1);
    memcpy((udword *)txStartAddress, theData+length1, length2);
    nextTxBufferPage->endPointer = length2 - 1;
    // cout << "endPointer: " << nextTxBufferPage->endPointer << endl;
  }

  /* Now we can tell Etrax to send this packet. Unless it is already      */
  /* busy sending packets. In which case it will send this automatically  */

  // STUFF: Advance nextTxPage here!
  nextTxPage = (nextTxPage + nOfBufferPagesNeeded) % txBufferPages;

  // Tell Etrax there isn't a packet at nextTxPage.
  BufferPage* nextPage = (BufferPage*)(txStartAddress + (nextTxPage * 256));
  nextPage->statusCommand = 0;
  nextPage->endPointer = 0x00;

  // STUFF: Tell Etrax to start sending by setting the 'statusCommand' byte of
  // the first page in the packet to 0x10!
  nextTxBufferPage->statusCommand = 0x10;
  // cout << "endTransmittPacket" << endl;

  *(volatile byte*)R_TR_CMD = 0x12;
}

//----------------------------------------------------------------------------
//
void
Ethernet::resetTransmitter()
{
  cout << "Transmit timeout. Resetting transmitter.\n" << endl;
  *(volatile byte*)R_TR_CMD = 0x01;             /* Reset */
  *(volatile byte*)R_TR_START = 0x00;  /* First to transmit */

  nextTxPage = 0;

  int page;
  BufferPage* aPointer;
  aPointer = (BufferPage *)txStartAddress;
  for (page = 0; page < txBufferPages; page++)
  {
    aPointer->statusCommand = 0;
    aPointer++;
  }
}

// STUFF: Add EthernetJob implementation
EthernetJob::EthernetJob(EthernetInPacket* thePacket)
{
  // Constructor: Initiate myPacket
  myPacket = thePacket;
}

void
EthernetJob::doit()
{
  // decode myPacket
  myPacket->decode();
  delete myPacket;
  // delete this;
}

//----------------------------------------------------------------------------
//
// STUFF: Add EthernetInPacket implementation
EthernetInPacket::EthernetInPacket(byte* theData,
	   udword theLength,
	   InPacket* theFrame):
     InPacket(theData, theLength, theFrame)
{
  // Constructor: Initiate base class InPacket
}

void
EthernetInPacket::decode()
{
  // Decode this ethernet packet.
  // Extract ethernet information and pass it up to LLC. This is done by
  // creating a LLCInPacket and decode it. Call returnRXBuffer when done.

  EthernetHeader* eHeader = (EthernetHeader *) InPacket::myData;
  myDestinationAddress = eHeader->destinationAddress;
  mySourceAddress = eHeader->sourceAddress;
  myTypeLen = ((eHeader->typeLen & 0x00ff) << 8) |
                     ((eHeader->typeLen & 0xff00) >> 8);
  // myTypeLen = eHeader->typeLen;

  LLCInPacket llcPacket(
    myData + headerOffset(),
    myLength - headerOffset() - Ethernet::crcLength,
    this,
    myDestinationAddress,
    mySourceAddress,
    myTypeLen
  );

  llcPacket.decode();

  Ethernet::instance().returnRXBuffer();
}

void
EthernetInPacket::answer(byte* theData, udword theLength)
{
  // Upper layers may choose to send an answer to the sender of this packet
  // prepend the appropriate ethernet information and send the packet
  // cout << "answer" << endl;
  udword newLength = theLength + headerOffset();

  byte* newData = new byte[newLength];
  memcpy(newData + headerOffset(), theData, theLength);
  EthernetHeader* eHeader = (EthernetHeader *) newData;
  eHeader->destinationAddress = mySourceAddress;
  eHeader->sourceAddress = Ethernet::instance().myAddress();
  eHeader->typeLen = ((myTypeLen & 0x00ff) << 8) |
                     ((myTypeLen & 0xff00) >> 8);;

  // cout << "ethHeader: " << theLength << "->" << newLength << endl;
  Ethernet::instance().transmittPacket(newData, newLength);
  delete[] newData;
}

//----------------------------------------------------------------------------
//
uword
EthernetInPacket::headerOffset()
{
  // Return the length of the ethernet header,
  // see Ethernet::ethernetHeaderLength
  return Ethernet::ethernetHeaderLength;
}

//----------------------------------------------------------------------------
//
InPacket*
EthernetInPacket::copyAnswerChain()
{
  EthernetInPacket* anAnswerPacket = new EthernetInPacket(*this);
  anAnswerPacket->setNewFrame(NULL);
  return anAnswerPacket;
}

//----------------------------------------------------------------------------
//
EthernetAddress::EthernetAddress()
{
}

//----------------------------------------------------------------------------
//
EthernetAddress::EthernetAddress(byte a0, byte a1, byte a2, byte a3, byte a4, byte a5)
{
  myAddress[0] = a0;
  myAddress[1] = a1;
  myAddress[2] = a2;
  myAddress[3] = a3;
  myAddress[4] = a4;
  myAddress[5] = a5;
}

//----------------------------------------------------------------------------
//
bool
EthernetAddress::operator == (const EthernetAddress& theAddress) const
{
  for (int i=0; i < length; i++)
  {
    if (theAddress.myAddress[i] != myAddress[i])
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
//
void
EthernetAddress::writeTo(byte* theData)
{
  for (int i = 0; i < length; i++)
  {
    theData[i] = myAddress[i];
  }
}

//----------------------------------------------------------------------------
//
ostream& operator <<(ostream& theStream, const EthernetAddress& theAddress)
{
  static char aString[6*2+1];
  int i;

  aString[0] = '\0';
  for (i = 0; i < 5; i++)
  {
    sprintf(aString, "%s%02x:", aString, theAddress.myAddress[i]);
  }
  sprintf(aString, "%s%02x", aString, theAddress.myAddress[i]);
  theStream << aString;
  return theStream;
}

//----------------------------------------------------------------------------
//
EthernetHeader::EthernetHeader()
{
}

/****************** END OF FILE Ethernet.cc *********************************/
