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
    char buf[MAXLINE] = "";
    int read_length;
    int i, result, size;

    USLOSS_Console("Child%d(): start\n", term);

    for (i = 0; i< 5; i++)
    {
        int retval = TermRead(buf, MAXLINE, term, &read_length);
        if (retval < 0)
        {
            USLOSS_Console("ERROR: ReadTerm\n");
            return -1;
        }
        buf[read_length] = '\0';

        USLOSS_Console("buffer written '%s'\n", buf);

        result = TermWrite(buf, strlen(buf), term, &size);
        if (result < 0 || size != strlen(buf))
        {
            USLOSS_Console("\n ***** Child(%d): got bad result or ", term);
            USLOSS_Console("bad size! *****\n\n");
        }
    }

    USLOSS_Console("Child%d(): done\n", term);
    Terminate(0);
}



extern int testcase_timeout;   // defined in the testcase common code

int start4(char *arg)
{
    int  pid, status, i;
    char buf[4][12];
    char child_buf[12];

    testcase_timeout = 60;

    USLOSS_Console("start4(): Spawn four children.\n");

    for (i=0; i<4; i++)
    {
        sprintf(buf[i], "%d", i);
        sprintf(child_buf, "Child%d", i);
        status = Spawn(child_buf, Child1, buf[i], USLOSS_MIN_STACK,2, &pid);
        assert(status == 0);
    }

    for (i=0; i<4; i++)
    {
        Wait(&pid, &status);
        assert(status == 0);
    }

    USLOSS_Console("start4(): done.\n");
    Terminate(0);
}

