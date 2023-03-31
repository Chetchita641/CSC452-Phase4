#include <stdio.h>
#include <stdlib.h>
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



int Child1(char *arg)
{
    int term = atoi(arg);
    char buf[MAXLINE + 1] = "";
    int result, read_length;
    int i, size;

    USLOSS_Console("Child%d(): start\n", term);

    for (i = 0; i< 10; i++)
    {
        memset(buf, 'x', sizeof(buf));

        int rc = TermRead(buf, MAXLINE, term, &read_length);
        if (rc < 0)
        {
            USLOSS_Console("ERROR: ReadTeam\n");
            return -1;
        }

        buf[read_length] = '\0';
        USLOSS_Console("Child%d(): buffer read from term%d   '%s'\n", term, term, buf);

        result = TermWrite(buf, strlen(buf), term, &size);
        if (result < 0 || size != strlen(buf))
        {
            USLOSS_Console("\n ***** Child(%d): got bad result = %d ", term, result);
            USLOSS_Console("or bad size = %d! *****\n\n ", size);
        }
    }

    USLOSS_Console("Child%d(): done\n", term);
    Terminate(0);
}



extern int testcase_timeout;   // defined in the testcase common code

int start4(char *arg)
{
    int  pid, status;
    char buf[12];
    char child_buf[12];

    testcase_timeout = 60;

    USLOSS_Console("start4(): Spawn one child.\n");

    sprintf(buf, "%d", 0);
    sprintf(child_buf, "Child%d", 0);
    status = Spawn(child_buf, Child1, buf, USLOSS_MIN_STACK,2, &pid);
    assert(status == 0);

    Wait(&pid, &status);
    assert(status == 0);

    USLOSS_Console("start4(): done.\n");
    Terminate(0);
}

