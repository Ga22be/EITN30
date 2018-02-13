/*!**************************************************************************
*! FILE NAME  : init.cc
*!
*! DESCRIPTION: The implementation of initialisation for the
*!              CD server
*!
*!---------------------------------------------------------------------------
*! HISTORY
*!
*! DATE         NAME               CHANGES
*! ----         ----               -------
*! Jul 27 1995  Stefan S           Initial version
*! Sep 05 1995  Stefan S           PseudoDirectoryFilePointer->
*!                                   DirectoryFilePointer
*! Dec 08 1995  Patrik B           Creates  N_CBLOCKS_AND_THREADS threads.
*!                                 Changed stack size to 2000 per thread.
*! Feb 13 1996  Sven Ekstrom       Changed use of N_CBLOCKS_AND_THREADS to
*!                                 N_THREADS.
*! Apr  4 1996  Fredrik Norrman    Changed stack size to 2500
*! Apr 15 1996  Stefan S           cast enum to int in << expression
*!                                 (VC++ 'feature')
*! May  6 1996  Fredrik Norrman    Changed stack size to 4500
*! May  6 1996  Willy Sagefalk     Don't start SCSI for HD. (Temporary)
*! May 08 1996  Stefan S           New Job interface.
*! May 31 1996  Jens Johansson     SCSI for Daisy now included.
*! Jun 19 1996  Stefan Jonsson     Added OEM specific mib-file name.
*! Jul  3 1996  Per Flock          Added 'resourceType' to 'create(...)'
*! Jul 17 1996  Jens Johansson     Added 'scsi.ini' file for INA.
*! Jul 24 1996  Per Flock          Added HTML configuration
*! Aug  5 1996  Niklas Jonsson     Added images directory and cd/index.htm
*! Aug 13 1996  Niklas Jonsson     Images now use PseudoFileableData
*! Aug 16 1996  Niklas Jonsson     HtmlHomePageFile
*! Aug 28 1996  Per Flock          Added HtmlVolumeManageIndexFile
*! Sep  3 1996  Christian Matson   Added HTTP stuff.
*! Sep  5 1996  Christian Matson   Added HTTP stuff.
*! Sep 11 1996  Christian Matson   Added HTTP stuff.
*! Sep 13 1996  Christian Matson   Added HTTP stuff.
*! Sep 19 1996  Niklas Jonsson     And more HTTP changes.
*! Sep 24 1996  Niklas Jonsson     Made 'html' and 'images' as directories.
*! Sep 25 1996  Per Flock          Moved images and info.htm to a new direcory
*!                                 named /config/public/
*! Oct  1 1996  Niklas Jonsson     Added diagnostics HTML page
*! Oct  2 1996  Christian Matson   Removed creation of readme.txt file.
*! Oct 10 1996  Per Flock          Temporary removed diag.htm
*! Oct 18 1996  Per Flock          Added diag.htm again
*! Dec  5 1996  Patrik Bannura     Moved creation of volumes directory
*!                                 down a few lines.
*! Jan 08 1997  Stefan Jonsson     Added MediaAgent stuff.
*! Jan 14 1997  Fredrik Norrman    Added logfile, but not enabled
*!
*!---------------------------------------------------------------------------
*! (C) Copyright 1996, Axis Technologies AB, LUND, SWEDEN
*!**************************************************************************/
/* @(#) init.cc 1.33 01/14/97 */

#include <stddef.h>

extern "C" {
#include "compiler.h"
#include "system.h"
#include "osys.h"
#include "msg.h"
#include "reschedule.h"
}

#include "iostream.hh"
#include "threads.hh"
#include "job.hh"

#include "timer.hh"

#include "frontpanel.hh"
// #include "ethernet.hh"

#define D_INIT
#ifdef D_INIT
#define trace cout
#else
#define trace if(false) cout
#endif

//*#**************************************************************************
//*#

void* operator new(size_t mem_size)
{
  return ::ax_malloc(mem_size);
}

//*#**************************************************************************
//*#

void operator delete(void* ptr)
{
  ::ax_free(ptr);
}

//*#**************************************************************************
//*#

extern "C" void
ethernet_interrupt()
{
}

//*#**************************************************************************
//*#

enum { numberOfServers   = 10,
       serverStackSize   = 4500,
       jobQueueSoftLimit = 20 };

extern "C" void
initJob()
{
  // The enums are casted to ints, 'cos some compilers thinks that's cool.
  trace << "Create "
        << static_cast<int>(numberOfServers) << " Job Servers, stack "
        << static_cast<int>(serverStackSize) << " bytes" << endl;
  Job::createServers(numberOfServers, serverStackSize, jobQueueSoftLimit);
}

extern "C" void
thread_main(void)
{
  initJob();
  FrontPanel::instance();
  while (1)
  {
    mail_struct  msg;
    if (os_get(WAIT_FOREVER, &msg))
    {
      switch (msg.action_id)
      {
        case THREAD_TIMER_EXPIRED:
        {
          Timed* timerObj = (Timed*)msg.param_ptr;
          timerObj->timerExpired();
        }
        break;

        case THREAD_RESCHEDULE:
        {
          reschedule();
        }
         break;

        case THREAD_PACKET_RECEIVED:
        {
        }
        break;

       default:
         break;
      }
    }
  }
}
