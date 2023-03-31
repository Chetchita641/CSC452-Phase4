/*
 * Test Invalid disk and Terminal.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <usloss.h>
#include <usyscall.h>

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase3_usermode.h>
#include <phase4.h>
#include <phase4_usermode.h>



int start4(char *arg)
{
    int  status;
    char buffer[80];

    USLOSS_Console("start4(): Attempting some invalid writes.  If they are all rejected (as expected), then you will not see anything more before 'done'\n");

    if (DiskWrite(buffer, 3, 1, 1, 1, &status) != -1)
        USLOSS_Console("start4(): Disk : An invalid call was made, but DiskWrite() returned something other than -1   (unit 3 is invalid)\n");

    if (DiskWrite(buffer, 0, 1, 17, 1, &status) != -1)
        USLOSS_Console("start4(): Disk : An invalid call was made, but DiskWrite() returned something other than -1   (startSector is invalid)\n");

    /* unlike the others (invalid arguments), this should have an OK return value
     * (the I/O was attempted), but a bad status (it didn't work)
     */
    if (DiskWrite(buffer, 0, 1776, 1, 1, &status) != 0)
        USLOSS_Console("start4(): Disk : An invalid call was made, but DiskWrite() returned something other than 0   (track seems plausible, but is too high)\n");
    if (status != USLOSS_DEV_ERROR)
        USLOSS_Console("start4(): Disk : An invalid call was made, but DiskWrite() the status was something other than USLOSS_DEV_ERR   (track seems plausible, but is too high)\n");

    USLOSS_Console("start4(): done\n");
    Terminate(0);
}

