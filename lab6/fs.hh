/*!***************************************************************************
*!
*! FILE NAME  : fs.hh
*!
*! DESCRIPTION: simple filesystem
*!
*!***************************************************************************/

#ifndef fs_hh
#define fs_hh

/****************** INCLUDE FILES SECTION ***********************************/


/****************** CLASS DEFINITION SECTION ********************************/

/*****************************************************************************
*%
*% CLASS NAME   : FileSystem
*%
*% BASE CLASSES : None
*%
*% CLASS TYPE   : Singleton
*%
*% DESCRIPTION  : simple file system
*%
*% SUBCLASSING  : None.
*%
*%***************************************************************************/
class FileSystem
{
 public:
  static FileSystem& instance();

  bool writeFile(char *path,char *name,byte *theData,udword theLength);
  // true if the write was done ie if enough memory is present

  byte *readFile(char *path,char *name,udword& theLength);
  // returns the ptr to and the length of a file..

  bool flushSaveFile();

 private:
  FileSystem();
  char* saveFile;
  static const byte myFileSystem[];
};

#endif

/****************** END OF FILE fs.hh *************************************/
