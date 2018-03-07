/*!***************************************************************************
*!
*! FILE NAME  : fs.hh
*!
*! DESCRIPTION: simple filesystem
*!
*!***************************************************************************/

#ifndef http_hh
#define http_hh

/****************** INCLUDE FILES SECTION ***********************************/

/****************** CLASS DEFINITION SECTION ********************************/

/*****************************************************************************
*%
*% CLASS NAME   : HTTPServer
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   : Class
*%
*% DESCRIPTION  : simple web server
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class HTTPServer : public Job
{
 public:
  HTTPServer(TCPSocket* theSocket);

  char* extractString(char* thePosition, udword theLength);

  udword contentLength(char* theData, udword theLength);

  char* decodeBase64(char* theEncodedString);

  char* decodeForm(char* theEncodedForm);

  void doit();
  // Gets called when the application thread begins execution.
  // The HTTPServer job is scheduled by TCP when a connection is
  // established.

 private:
  // MULLEMECK BYGGER HEADER
   void buildHeader(char* header,
     char* suffixDelimiter,
     udword lengthOfContent,
     bool isPrivate);
   TCPSocket* mySocket;
   char* admin;
};

#endif

/****************** END OF FILE fs.hh *************************************/
