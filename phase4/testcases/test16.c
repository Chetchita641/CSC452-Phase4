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

static char XXbuf[4][512];
int ubiq(int);



int k1(char *arg)
{
    ubiq(5);
    Terminate(2);
}

int k2(char *arg)
{
    ubiq(3);
    Terminate(3);
}

int k3(char *arg)
{
    ubiq(9);
    Terminate(4);
}

int k4(char *arg)
{
    ubiq(0);
    Terminate(5);
}

int k5(char *arg)
{
    ubiq(7);
    Terminate(6);
}

int k6(char *arg)
{
    ubiq(2);
    Terminate(7);
}

int k7(char *arg)
{
    ubiq(1);
    Terminate(8);
}

int k8(char *arg)
{
    ubiq(6);
    Terminate(9);
}

int cksum=0;

int ubiq(int t)
{
    int status = -1;
    int z = t % 4;

    USLOSS_Console("going to write track %d\n", t);
    int retval = DiskWrite(XXbuf[z], 1, t, 4, 1, &status);

    if (retval < 0 || status != 0)
        USLOSS_Console("ERROR: DiskWrite()   retval %d   status %d\n", retval,status);
    else
    {
        USLOSS_Console("after writing track %d\n", t);
        cksum+=t;
    }
    return 0;
}



int start4(char *arg)
{
    int status, pid;

    USLOSS_Console("start4(): disk scheduling test, 8 children\n");
    USLOSS_Console("          algorithm test           \n");
    USLOSS_Console("          different tracks\n");

    strcpy(XXbuf[0],"One flew East\n");
    strcpy(XXbuf[1],"One flew West\n");
    strcpy(XXbuf[2],"One flew over the coo-coo's nest\n");
    strcpy(XXbuf[3],"--did it work?\n");

    Spawn("k1", k1, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k2", k2, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k3", k3, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k4", k4, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k5", k5, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k6", k6, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k7", k7, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k8", k8, NULL, USLOSS_MIN_STACK, 1, &pid);

    for (int i=0; i<8; i++)
    {
        Wait(&pid, &status);
        USLOSS_Console("process %d quit with status %d\n", pid, status);
    }

    USLOSS_Console("start4(): done %d\n",cksum);
    Terminate(0);
}

