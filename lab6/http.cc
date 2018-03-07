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

// #define D_HTTP_CORE
#ifdef D_HTTP_CORE
#define coreOut cout
#else
#define coreOut if(false) cout
#endif

/****************** HTTPServer DEFINITION SECTION ***************************/
HTTPServer::HTTPServer(TCPSocket* theSocket):
  mySocket(theSocket),
  admin("admin:admin")
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
      char* type = extractString((char*)aData, 4);
      bool isHead = (strncmp(type, "HEAD", 4) == 0);
      bool isPOST = (strncmp(type, "POST", 4) == 0);

      byte* firstHTTP = (byte*) strstr((char*) aData, "HTTP");
      uword aDataOffset = 4;
      if(isHead || isPOST){
        aDataOffset++;
      }
      udword pathAndFileLength = (udword) ((firstHTTP-1)-(aData+aDataOffset));
      char* pathAndFile = extractString((char*)(aData+aDataOffset), pathAndFileLength);
      //  if(strcmp(pathAndFile, "/") == 0){
      udword fileLength = 0;
      char* path;
      char* fileName;
      if(strcmp(pathAndFile, "/") == 0) {
        trace << "Requested: root" << endl;
        path = NULL;
        fileName = "index.htm";
      } else {
        trace << "Requested: " << pathAndFile << endl;
        char* delimiter = strrchr(pathAndFile, '/');
        udword pathLength = (udword) (delimiter - pathAndFile);
        path = extractString(pathAndFile+1, pathLength);
        path[strlen(path)-1] = 0xff;
        fileName = extractString(delimiter+1, pathAndFileLength-pathLength-1);
        trace << "Path: " << path << endl;
        trace << "File: " << fileName << endl;
      }
      if(strncmp(type, "GET", 3) == 0 || strncmp(type, "HEAD", 4) == 0) {
        // GET
        trace << "GET/HEAD Received" << endl;
        udword fromBegining = (udword) (strstr(path, "private")-path);
        if(path == NULL){
          //IF ROOT CASE
          fromBegining = 1337;
        }

        char* header = NULL;
        if(fromBegining == 0){
          // This is a private realm
          trace << "YOU HAVE ENTERED MY PRIVATE REALM" << endl;

          if(strstr((char*) aData, "Authorization: Basic ") == NULL){
            // Call to private without auth
            trace << "BRO ARE YOU EVEN TRYING" << endl;
            header = new char[1000];
            buildHeader(header, strrchr(fileName, '.')+1, 97, true);
          } else {
            char* beginning = strstr((char*) aData, "Authorization: Basic ")+21;
            char* end = strstr(beginning, "\r\n");
            char* encodedCredentials = extractString(beginning, (udword) (end-beginning));
            char* decodedCredentials = decodeBase64(encodedCredentials);
            trace << "decodedCredentials: " << decodedCredentials << endl;
            if (strcmp(decodedCredentials, admin) == 0) {
              // YOU SHALL PASS
            } else {
              trace << "YOU SHALL NOT PASS" << endl;
              header = new char[1000];
              buildHeader(header, strrchr(fileName, '.')+1, 97, true);
            }
            delete[] encodedCredentials;
            delete[] decodedCredentials;
          }
        }
        //  See if file exists
        byte* file = FileSystem::instance().readFile(path, fileName, fileLength);
        char* response;
        udword responseLength = 0;
        if(file != 0 || header != NULL /* We are in /private */){
          // HEADER
          bool isPrivate = false;
          if(header == NULL) {
            header = new char[1000];
            buildHeader(header, strrchr(fileName, '.')+1, fileLength, false);
          } else {
            isPrivate = true;
            file = (byte*) "<html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>";
            fileLength = strlen((char*)file);
          }

          udword headerLength = strlen(header);
          responseLength = headerLength + fileLength;
          if(isHead){
            responseLength = headerLength;
          }
          response = new char[responseLength];
          memcpy(response, header, headerLength);
          if(!isHead){
            memcpy(response+headerLength, file, fileLength);
          }
          if(isPrivate){
            delete[] file;
          }
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


      }
      else if (strncmp((char*) aData, "POST", 4) == 0) {
        coreOut << "Core::POST begin " << ax_coreleft_total() << endl;
        // POST
        trace << "POST Received" << endl;
        udword lengthOfContent = contentLength((char*)aData, aLength);
        trace << "Content-Length: " << lengthOfContent << endl;

        // BEGIN ASSEMBY OF POST REQUEST
        char* aggregation = new char[aLength + 1];
        memcpy(aggregation, aData, aLength);
        udword aggregationLength = aLength;
        aggregation[aggregationLength] = '\0';
        while(strstr(aggregation, "\r\n\r\n") == NULL){
          // READ NEXT
          udword continuationLength = 0;
          byte* continuation = mySocket->Read(continuationLength);
          // cout << continuation << endl;

          // SAVE PREVIOUS AGGREGATION
          udword prevAggregationLength = aggregationLength;
          char* prevAggregation = aggregation;

          // COPY OLD AND NEW DATA INTO NEW, BIGGER, AGGREGATION
          aggregationLength += continuationLength;
          aggregation = new char[aggregationLength+1];
          memcpy(aggregation, prevAggregation, prevAggregationLength);
          memcpy(aggregation+prevAggregationLength, continuation, continuationLength);
          aggregation[aggregationLength]='\0';
          delete[] prevAggregation;
          delete[] continuation;
        }
        char* delimiter = strstr(aggregation, "\r\n\r\n")+4;
        udword currentContentLength = strlen(delimiter);
        if(currentContentLength < lengthOfContent){
          // ALL DATA NOT RECEIVED
          // cout << "currentContentLength: " << currentContentLength << endl;
          // cout << "requiredContentLenght: " << lengthOfContent << endl;
          // cout << "WE NEED MORE data" << endl;

          // READ NEXT
          udword continuationLength = 0;
          byte* continuation = mySocket->Read(continuationLength);
          // cout << continuation << endl;

          // SAVE PREVIOUS AGGREGATION
          udword prevAggregationLength = aggregationLength;
          char* prevAggregation = aggregation;

          // COPY OLD AND NEW DATA INTO NEW, BIGGER, AGGREGATION
          aggregationLength += continuationLength;
          aggregation = new char[aggregationLength+1];
          memcpy(aggregation, prevAggregation, prevAggregationLength);
          memcpy(aggregation+prevAggregationLength, continuation, continuationLength);
          aggregation[aggregationLength]='\0';
          delete[] prevAggregation;
          delete[] continuation;

          delimiter = strstr(aggregation, "\r\n\r\n")+4;
          currentContentLength = strlen(delimiter);
        }

        udword fromBegining = (udword) (strstr(path, "private")-path);
        coreOut << "Core::POSTandAuth before " << ax_coreleft_total() << endl;
        char* header = NULL;
        if(fromBegining == 0){
        // if(false){
          // This is a private realm
          trace << "YOU HAVE ENTERED MY PRIVATE REALM" << endl;

          if(strstr((char*)aData, "Authorization: Basic ") == NULL){
            // Call to private without auth
            trace << "BRO ARE YOU EVEN TRYING" << endl;
            header = new char[1000];
            buildHeader(header, strrchr(fileName, '.')+1, 97, true);
            coreOut << "Core::POSTandAuthNoAuth " << ax_coreleft_total() << endl;

          } else {
            coreOut << "Core::POSTandAuthAuth before " << ax_coreleft_total() << endl;
            char* beginning = strstr((char*) aData, "Authorization: Basic ")+21;
            char* end = strstr(beginning, "\r\n");
            char* encodedCredentials = extractString(beginning, (udword) (end-beginning));
            char* decodedCredentials = decodeBase64(encodedCredentials);
            trace << "decodedCredentials: " << decodedCredentials << endl;
            if (strcmp(decodedCredentials, admin) == 0) {
              // YOU SHALL PASS
            } else {
              trace << "YOU SHALL NOT PASS" << endl;
              header = new char[1000];
              buildHeader(header, strrchr(fileName, '.')+1, 97, true);
            }
            delete[] encodedCredentials;
            delete[] decodedCredentials;
            coreOut << "Core::POSTandAuthAuth after " << ax_coreleft_total() << endl;
          }
        }
        coreOut << "Core::POSTandAuth after " << ax_coreleft_total() << endl;

        coreOut << "Core::POSTandSend before " << ax_coreleft_total() << endl;
        char* response = NULL;
        udword responseLength = 0;
        if (header == NULL) {
          coreOut << "Core::POSTandSendSave before " << ax_coreleft_total() << endl;
          char* encodedContent = extractString(strstr(aggregation, "\r\n\r\n")+4, lengthOfContent);
          char* decodedContent = decodeForm(encodedContent);
          coreOut << "Core::POSTandSendSaveWrite before " << ax_coreleft_total() << endl;
          FileSystem::instance().writeFile(NULL, NULL, (byte*) decodedContent, strlen(decodedContent));
          coreOut << "Core::POSTandSendSaveWrite after " << ax_coreleft_total() << endl;
          delete[] encodedContent;
          delete[] decodedContent;
          coreOut << "Core::POSTandSendSave after " << ax_coreleft_total() << endl;

          // RESPOND TO MAKE CHROME HAPPY
          response = "HTTP/1.0 200 OK\r\nContent-type: text/html\r\nContent-Length: 117\r\n\r\n<html><head><title>Accepted</title></head><body><h1>The file dynamic.htm was updated successfully.</h1></body></html>";
          responseLength = strlen(response);
        } else {
          char* unauthMessage = "<html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>";
          udword unauthMessageLength = strlen(unauthMessage);

          udword headerLength = strlen(header);
          responseLength = headerLength + unauthMessageLength;
          response = new char[responseLength];
          memcpy(response, header, headerLength);
          memcpy(response+headerLength, unauthMessage, unauthMessageLength);

          delete[] unauthMessage;
          delete[] header;
        }
        coreOut << "Core::POSTandSend after " << ax_coreleft_total() << endl;
        mySocket->Write((byte*) response, responseLength);
        delete[] aggregation;
        delete[] response;
        coreOut << "Core::POST end " << ax_coreleft_total() << endl;
      }
      // BLACK MAGIC, no touchie
      // done = true;
      delete[] type;
      delete[] path;
      delete[] fileName;
      delete[] pathAndFile;
    }

    delete[] aData;
    coreOut << "Core::doit end " << ax_coreleft_total() << endl;
  }
  coreOut << "Core::doit close " << ax_coreleft_total() << endl;
  mySocket->Close();
}

// MULLEMECK BYGGER HEADER
void
HTTPServer::buildHeader(char* header, char* suffixDelimiter, udword lengthOfContent, bool isPrivate){
  header[0] = '\0';
  //STATUS LINE
  if(!isPrivate){
    header = strcat(header, "HTTP/1.0 200 OK\r\n");
  } else {
    header = strcat(header, "HTTP/1.0 401 Unauthorized\r\n");
  }
  //CONTENT TYPE
  if(strcmp(suffixDelimiter, "gif") == 0){
    // GIF CASE
    header = strcat(header, "Content-Type: image/gif\r\n");
  } else if (strcmp(suffixDelimiter, "jpg") == 0) {
    // JPG CASE
    header = strcat(header, "Content-Type: image/jpg\r\n");
  } else if (strcmp(suffixDelimiter, "htm") == 0 ) {
    // HTM CASE
    header = strcat(header, "Content-Type: text/html\r\n");
  }
  //CONTENT LENGTH
  if(lengthOfContent > 0 && !isPrivate) {
    header = strcat(header, "Content-Length: ");
    sprintf(header+strlen(header),"%d",lengthOfContent);
    header = strcat(header, "\r\n");
  }
  if(isPrivate) {
    header = header = strcat(header, "WWW-Authenticate: Basic realm=\"private\"\r\n");
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
