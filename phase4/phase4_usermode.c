/*
 *  File:  libuser.c
 *
 *  Description:  This file contains the interface declarations
 *                to the OS kernel support package.
 */

#include <usloss.h>
#include <usyscall.h>

#include "phase4_usermode.h"

#define CHECKMODE { \
    if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { \
        USLOSS_Console("Trying to invoke syscall from kernel\n"); \
        USLOSS_Halt(1);  \
    } \
}



/*
 *  Routine:  Sleep
 *
 *  Description: This is the call entry point for timed delay.
 *
 *  Arguments:    int seconds -- number of seconds to sleep
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int Sleep(int seconds)
{
    USLOSS_Sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_SLEEP;
    sysArg.arg1 = (void *) ( (long) seconds);

    USLOSS_Syscall(&sysArg);

    return (long) sysArg.arg4;
} /* end of Sleep */


/*
 *  Routine:  TermRead
 *
 *  Description: This is the call entry point for terminal input.
 *
 *  Arguments:    char *buffer    -- pointer to the input buffer
 *                int   bufferSize   -- maximum size of the buffer
 *                int   unitID -- terminal unit number
 *                int  *numCharsRead      -- pointer to output value
 *                (output value: number of characters actually read)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int TermRead(char *buffer, int bufferSize, int unitID, int *numCharsRead)     
{
    USLOSS_Sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_TERMREAD;
    sysArg.arg1 = (void *) buffer;
    sysArg.arg2 = (void *) ( (long) bufferSize);
    sysArg.arg3 = (void *) ( (long) unitID);

    USLOSS_Syscall(&sysArg);

    *numCharsRead = (long) sysArg.arg2;
    return (long) sysArg.arg4;
} /* end of TermRead */


/*
 *  Routine:  TermWrite
 *
 *  Description: This is the call entry point for terminal output.
 *
 *  Arguments:    char *buffer    -- pointer to the output buffer
 *                int   bufferSize   -- number of characters to write
 *                int   unitID -- terminal unit number
 *                int  *numCharsWritten      -- pointer to output value
 *                (output value: number of characters actually written)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int TermWrite(char *buffer, int bufferSize, int unitID, int *numCharsWritten)    
{
    USLOSS_Sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_TERMWRITE;
    sysArg.arg1 = (void *) buffer;
    sysArg.arg2 = (void *) ( (long) bufferSize);
    sysArg.arg3 = (void *) ( (long) unitID);

    USLOSS_Syscall(&sysArg);

    *numCharsWritten = (long) sysArg.arg2;
    return (long) sysArg.arg4;
} /* end of TermWrite */


/*
 *  Routine:  DiskRead
 *
 *  Description: This is the call entry point for disk input.
 *
 *  Arguments:    void* diskBuffer  -- pointer to the input buffer
 *                int   unit -- which disk to read
 *                int   track  -- first track to read
 *                int   first -- first sector to read
 *                int   sectors -- number of sectors to read
 *                int   *status    -- pointer to output value
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int DiskRead(void *diskBuffer, int unit, int track, int first, int sectors,
    int *status)
{
    USLOSS_Sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_DISKREAD;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = (void *) ( (long) sectors);
    sysArg.arg3 = (void *) ( (long) track);
    sysArg.arg4 = (void *) ( (long) first);
    sysArg.arg5 = (void *) ( (long) unit);

    USLOSS_Syscall(&sysArg);

    *status = (long) sysArg.arg1;
    return (long) sysArg.arg4;
} /* end of DiskRead */


/*
 *  Routine:  DiskWrite
 *
 *  Description: This is the call entry point for disk output.
 *
 *  Arguments:    void *diskBuffer -- pointer to the output buffer
 *                int   unit       -- which disk to write
 *                int   track      -- first track to write
 *                int   first      -- first sector to write
 *                int   sectors    -- number of sectors to write
 *                int  *status     -- pointer to output value
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int DiskWrite(void *diskBuffer, int unit, int track, int first, int sectors, 
              int *status)
{
    USLOSS_Sysargs sysArg;

    CHECKMODE;
    sysArg.number = SYS_DISKWRITE;
    sysArg.arg1 = diskBuffer;
    sysArg.arg2 = (void *) ( (long) sectors);
    sysArg.arg3 = (void *) ( (long) track);
    sysArg.arg4 = (void *) ( (long) first);
    sysArg.arg5 = (void *) ( (long) unit);

    USLOSS_Syscall(&sysArg);

    *status = (long) sysArg.arg1;
    return (long) sysArg.arg4;
} /* end of DiskWrite */


/*
 *  Routine:  DiskSize
 *
 *  Description: This is the call entry point for getting the disk size.
 *
 *  Arguments:    int  unit   -- which disk
 *                int *sector -- # bytes in a sector
 *                int *track  -- # sectors in a track
 *                int *disk   -- # tracks in the disk
 *                (output value: completion status)
 *
 *  Return Value: 0 means success, -1 means error occurs
 */
int DiskSize(int unit, int *sector, int *track, int *disk)
{
    USLOSS_Sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_DISKSIZE;
    sysArg.arg1 = (void *) ( (long) unit);

    USLOSS_Syscall(&sysArg);

    *sector = (long) sysArg.arg1;
    *track  = (long) sysArg.arg2;
    *disk   = (long) sysArg.arg3;
    return (long) sysArg.arg4;
} /* end of DiskSize */

/* end libuser.c */
