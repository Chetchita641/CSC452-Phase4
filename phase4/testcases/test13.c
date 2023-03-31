/* DISKTEST
 * - note that the test script should clean out the disk file
 * each time before running this test.
 * Write three sectors to the disk and then read them back.
 * Write starting at track 4, sector 15 -- should wrap around to track 5
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <usloss.h>
#include <usyscall.h>

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase3_usermode.h>
#include <phase4.h>
#include <phase4_usermode.h>



static char sectors[3 * 512];
static char copy[3 * 512];

int start4(char *arg)
{
    int result;
    int status;

    USLOSS_Console("start4(): Writing data to 3 disk sectors, wrapping to next track\n");
    USLOSS_Console("\n");

    USLOSS_Console("start4(): Disk 0:\n");
    strcpy(&sectors[0 * 512], "This is a test");
    strcpy(&sectors[1 * 512], "Does it work?");
    strcpy(&sectors[2 * 512], "One last chance");
    result = DiskWrite(sectors, 0, 4, 15, 3, &status);
    assert(result == 0);
    assert(status == 0);

    result = DiskWrite(sectors, 1, 4, 15, 1, &status);
    assert(result == 0);
    assert(status == 0);

    result = DiskWrite(sectors + 512, 1, 5, 0, 1, &status);
    assert(result == 0);
    assert(status == 0);

    result = DiskWrite(sectors + 1024, 1, 5, 1, 1, &status);
    assert(result == 0);
    assert(status == 0);

    result = DiskRead(copy, 0, 4, 15, 3, &status);
    assert(result == 0);
    assert(status == 0);

    USLOSS_Console("start4(): Read from disk: %s\n", &copy[0*512]);
    USLOSS_Console("start4(): Read from disk: %s\n", &copy[1*512]);
    USLOSS_Console("start4(): Read from disk: %s\n", &copy[2*512]);
    USLOSS_Console("\n");

    USLOSS_Console("start4(): Disk 1:\n");
    strcpy(&sectors[0 * 512], "This is a test -- something different for this disk");
    strcpy(&sectors[1 * 512], "Does it work?  -- another change");
    strcpy(&sectors[2 * 512], "One last chance -- and one last change, too!");
    result = DiskWrite((char *) sectors, 1, 4, 15, 3, &status);
    assert(result == 0);
    assert(status == 0);

    result = DiskRead((char *) copy, 1, 4, 15, 3, &status);
    assert(result == 0);
    assert(status == 0);

    USLOSS_Console("start4(): Read from disk: %s\n", &copy[0*512]);
    USLOSS_Console("start4(): Read from disk: %s\n", &copy[1*512]);
    USLOSS_Console("start4(): Read from disk: %s\n", &copy[2*512]);
    USLOSS_Console("\n");

    Terminate(0);
}

