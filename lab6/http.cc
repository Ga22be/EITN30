/*!***************************************************************************
*!
*! FILE NAME  : http.cc
*!
*! DESCRIPTION: HTTP, Hyper text transfer protocol.
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
}

#include "iostream.hh"
#include "tcpsocket.hh"
#include "http.hh"
#include "fs.hh"

//#define D_HTTP
#ifdef D_HTTP
#define trace cout
#else
#define trace if(false) cout
#endif

// #define D_SOCKET_CORE
#ifdef D_HTTP_CORE
#define coreOut cout
#else
#define coreOut if(false) cout
#endif

/****************** HTTPServer DEFINITION SECTION ***************************/
HTTPServer::HTTPServer(TCPSocket* theSocket):
  mySocket(theSocket)
{

}

//----------------------------------------------------------------------------
//
void
HTTPServer::doit() {
  //TODO LET THE FUN BEGIN
  udword aLength;
  byte* aData;
  bool done = false;
  while (!done && !mySocket->isEof())
  {
    coreOut << "Core::doit begin " << ax_coreleft_total() << endl;
    aData = mySocket->Read(aLength);
    if(aLength > 4){
      //  cout << aData << endl;
      char* type = extractString((char*)aData, 4);
      if(strncmp(type, "GET", 3) == 0 || strncmp(type, "HEAD", 4) == 0) {
        // GET
        cout << "GET/HEAD Received" << endl;
        bool isHead = (strncmp(type, "HEAD", 4) == 0);
        cout << unsigned(isHead) << endl;
        byte* firstHTTP = strstr(aData, "HTTP");
        uword aDataOffset = 4;
        if(isHead){
          aDataOffset++;
        }
        udword pathAndFileLength = (udword) ((firstHTTP-1)-(aData+aDataOffset));
        char* pathAndFile = extractString((char*)(aData+aDataOffset), pathAndFileLength);
        //  if(strcmp(pathAndFile, "/") == 0){
        udword fileLength = 0;
        char* path;
        char* fileName;
        if(strcmp(pathAndFile, "/") == 0) {
          cout << "Requested: root" << endl;
          path = NULL;
          fileName = "index.htm";
        } else {
          //  cout << "Requested: " << pathAndFile << endl;
          char* delimiter = strrchr(pathAndFile, '/');
          //  cout << "Delimiter: " << delimiter << endl;
          udword pathLength = (udword) (delimiter - pathAndFile);
          //  cout << "pathLength: " << pathLength << endl;
          path = extractString(pathAndFile+1, pathLength);
          path[strlen(path)-1] = 0xff;
          fileName = extractString(delimiter+1, pathAndFileLength-pathLength-1);
          //  cout << "Path: " << path << endl;
          //  cout << "File: " << fileName << endl;
        }

        //  See if file exists
        byte* file = FileSystem::instance().readFile(path, fileName, fileLength);
        char* response;
        udword responseLength = 0;
        if(file != 0){
          // HEADER
          char* header = new char[1000];
          buildHeader(header, strrchr(fileName, '.')+1, fileLength);

          //  char* header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
          udword headerLength = strlen(header);
          //  cout << header << endl;
          responseLength = headerLength + fileLength;
          if(isHead){
            responseLength = headerLength;
          }
          response = new char[responseLength];
          memcpy(response, header, headerLength);
          if(!isHead){
            memcpy(response+headerLength, file, fileLength);
          }
          //  cout << headerLength << ":" << fileLength << endl;
          //  cout << (strlen(header) + fileLength) << endl;
          //  cout << "response: " << response << endl;
          delete[] header;
        } else {
          cout << "HTTP/1.0 404 Not found" << endl;
          if(!isHead){
            response = "HTTP/1.0 404 Not found\r\nContent-type: text/html\r\nContent-Length: 90\r\n\r\n<html><head><title>File not found</title></head><body><h1>404 Not found</h1></body></html>";
          } else {
            response = "HTTP/1.0 404 Not found\r\nContent-type: text/html\r\nContent-Length: 90\r\n\r\n";
          }
          responseLength = strlen(response);
        }
        mySocket->Write((byte*) response, responseLength);
        delete[] response;

        delete[] path;
        delete[] fileName;
        delete[] pathAndFile;
      } else if (strncmp(aData, "HEAD", 4) == 0) {
        // HEAD
      }
      // BLACK MAGIC, no touchie
      // done = true;
      delete[] type;
    }

    delete[] aData;
    coreOut << "Core::doit end " << ax_coreleft_total() << endl;
  }
  coreOut << "Core::doit close " << ax_coreleft_total() << endl;
  cout << "HTTP server end: " << unsigned(mySocket->isEof()) << endl;
  mySocket->Close();
}

// MULLEMECK BYGGER HEADER
void
HTTPServer::buildHeader(char* header, char* suffixDelimiter, udword fileLength){
  header[0] = '\0';
  //STATUS LINE
  header = strcat(header, "HTTP/1.0 200 OK\r\n");
  //CONTENT TYPE
  if(strcmp(suffixDelimiter, "gif") == 0){
    // cout << "gif" << endl;
    header = strcat(header, "Content-Type: image/gif\r\n");
  } else if (strcmp(suffixDelimiter, "jpg") == 0) {
    // cout << "jpg" << endl;
    header = strcat(header, "Content-Type: image/jpg\r\n");
  } else if (strcmp(suffixDelimiter, "htm") == 0 ) {
    // cout << "htm" << endl;
    header = strcat(header, "Content-Type: text/html\r\n");
  }
  //CONTENT LENGTH
  if(fileLength > 0) {
    header = strcat(header, "Content-Length: ");
    sprintf(header+strlen(header),"%d",fileLength);
    header = strcat(header, "\r\n");
  }
  //END OF HEADER
  header = strcat(header, "\r\n");
}
//----------------------------------------------------------------------------
//
// Allocates a new null terminated string containing a copy of the data at
// 'thePosition', 'theLength' characters long. The string must be deleted by
// the caller.
//
char*
HTTPServer::extractString(char* thePosition, udword theLength)
{
  char* aString = new char[theLength + 1];
  strncpy(aString, thePosition, theLength);
  aString[theLength] = '\0';
  return aString;
}

//----------------------------------------------------------------------------
//
// Will look for the 'Content-Length' field in the request header and convert
// the length to a udword
// theData is a pointer to the request. theLength is the total length of the
// request.
//
udword
HTTPServer::contentLength(char* theData, udword theLength)
{
  udword index = 0;
  bool   lenFound = false;
  const char* aSearchString = "Content-Length: ";
  while ((index++ < theLength) && !lenFound)
  {
    lenFound = (strncmp(theData + index,
                        aSearchString,
                        strlen(aSearchString)) == 0);
  }
  if (!lenFound)
  {
    return 0;
  }
  trace << "Found Content-Length!" << endl;
  index += strlen(aSearchString) - 1;
  char* lenStart = theData + index;
  char* lenEnd = strchr(theData + index, '\r');
  char* lenString = this->extractString(lenStart, lenEnd - lenStart);
  udword contLen = atoi(lenString);
  trace << "lenString: " << lenString << " is len: " << contLen << endl;
  delete [] lenString;
  return contLen;
}

//----------------------------------------------------------------------------
//
// Decode user and password for basic authentication.
// returns a decoded string that must be deleted by the caller.
//
char*
HTTPServer::decodeBase64(char* theEncodedString)
{
  static const char* someValidCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int aCharsToDecode;
  int k = 0;
  char  aTmpStorage[4];
  int aValue;
  char* aResult = new char[80];

  // Original code by JH, found on the net years later (!).
  // Modify on your own risk.

  for (unsigned int i = 0; i < strlen(theEncodedString); i += 4)
  {
    aValue = 0;
    aCharsToDecode = 3;
    if (theEncodedString[i+2] == '=')
    {
      aCharsToDecode = 1;
    }
    else if (theEncodedString[i+3] == '=')
    {
      aCharsToDecode = 2;
    }

    for (int j = 0; j <= aCharsToDecode; j++)
    {
      int aDecodedValue;
      aDecodedValue = strchr(someValidCharacters,theEncodedString[i+j])
        - someValidCharacters;
      aDecodedValue <<= ((3-j)*6);
      aValue += aDecodedValue;
    }
    for (int jj = 2; jj >= 0; jj--)
    {
      aTmpStorage[jj] = aValue & 255;
      aValue >>= 8;
    }
    aResult[k++] = aTmpStorage[0];
    aResult[k++] = aTmpStorage[1];
    aResult[k++] = aTmpStorage[2];
  }
  aResult[k] = 0; // zero terminate string

  return aResult;
}

//------------------------------------------------------------------------
//
// Decode the URL encoded data submitted in a POST.
//
char*
HTTPServer::decodeForm(char* theEncodedForm)
{
  char* anEncodedFile = strchr(theEncodedForm,'=');
  anEncodedFile++;
  char* aForm = new char[strlen(anEncodedFile) * 2];
  // Serious overkill, but what the heck, we've got plenty of memory here!
  udword aSourceIndex = 0;
  udword aDestIndex = 0;

  while (aSourceIndex < strlen(anEncodedFile))
  {
    char aChar = *(anEncodedFile + aSourceIndex++);
    switch (aChar)
    {
     case '&':
       *(aForm + aDestIndex++) = '\r';
       *(aForm + aDestIndex++) = '\n';
       break;
     case '+':
       *(aForm + aDestIndex++) = ' ';
       break;
     case '%':
       char aTemp[5];
       aTemp[0] = '0';
       aTemp[1] = 'x';
       aTemp[2] = *(anEncodedFile + aSourceIndex++);
       aTemp[3] = *(anEncodedFile + aSourceIndex++);
       aTemp[4] = '\0';
       udword anUdword;
       anUdword = strtoul((char*)&aTemp,0,0);
       *(aForm + aDestIndex++) = (char)anUdword;
       break;
     default:
       *(aForm + aDestIndex++) = aChar;
       break;
    }
  }
  *(aForm + aDestIndex++) = '\0';
  return aForm;
}

/************** END OF FILE http.cc *************************************/
