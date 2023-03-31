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

    USLOSS_Console("Child%d(): start\n", term);

    int retval = TermRead(buf, MAXLINE, term, &read_length);
    if (retval < 0)
    {
       USLOSS_Console("ERROR: ReadTerm\n");
       return -1;
    }

    buf[read_length] = '\0';
    USLOSS_Console("Child%d(): read '%s'\n", term, buf);
    return 0;
}


int Child2(char *arg)
{
    char buffer[MAXLINE];
    int  result, size;
    int  unit = atoi(arg);

    sprintf(buffer, "Child %d: A Something interesting to print here...\n", unit);

    result = TermWrite(buffer, strlen(buffer), unit, &size);
    if (result < 0 || size != strlen(buffer)) {
        USLOSS_Console("\n ***** Child(%d): got bad result or bad size! *****\n\n ", unit);
        USLOSS_Console("result = %d size = %d and bufferlength = %d\n", result, size, strlen(buffer));
    }

    Terminate(0);
    USLOSS_Console("Child(%d): should not see this message!\n", unit);
}



int start4(char *arg)
{
    int  pid, status, i;
    char buf[4][12];
    char child_buf[12];

    USLOSS_Console("start4(): Spawn four children.  Each child reads fm a different\n");
    USLOSS_Console("          terminal.  The child reading the shortest line will\n");
    USLOSS_Console("          finish first, etc.\n");
    USLOSS_Console("start4(): Spawn four children.  Each child writes to a different\n");
    USLOSS_Console("          terminal.\n");

    for (i = 0; i < 4; i++) {
        sprintf(buf[i], "%d", i);

        sprintf(child_buf, "Child%d", i);
        status = Spawn(child_buf, Child1, buf[i], USLOSS_MIN_STACK,2, &pid);
        assert(status == 0);

        sprintf(child_buf, "Child%d", i+4);
        status = Spawn(child_buf, Child2, buf[i], USLOSS_MIN_STACK,2, &pid);
        assert(status == 0);
    }

    for (i = 0; i < 8; i++) {
        Wait(&pid, &status);
        assert(status == 0);
    }

    USLOSS_Console("start4(): done.\n");
    Terminate(0);
}

