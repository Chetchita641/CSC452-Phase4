#include <stdio.h>
#include <string.h>

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
    int unit, sectorSize, trackSize, diskSize;
    char disk_buf_A[512];
    char disk_buf_B[512];
    int status;

    memset(disk_buf_A, 0, 512);
    memset(disk_buf_B, 0, 512);

    USLOSS_Console("start4(): started\n");

    unit = 1;
    DiskSize(unit, &sectorSize, &trackSize, &diskSize);

    USLOSS_Console("start4(): ");
    USLOSS_Console("unit %d, sector size %d, track size %d, disk size %d\n", unit, sectorSize, trackSize, diskSize);

    for ( int trackNum = 0; trackNum < diskSize; trackNum++ )
    {
        USLOSS_Console("\nstart4(): writing to disk %d, track %d, sector 0\n", unit, trackNum);
        sprintf(disk_buf_A, "track %d: A wonderful message to put on the disk...", trackNum);

        DiskWrite(disk_buf_A, unit, trackNum, 0, 1, &status);
        USLOSS_Console("start4(): DiskWrite returned status = %d\n", status);
    }   

    USLOSS_Console("start4(): calling Terminate\n");
    Terminate(0);
    USLOSS_Console("start4(): should not see this message!\n");
}

