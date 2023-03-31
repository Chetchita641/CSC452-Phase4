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

int Child(char *arg);



extern int testcase_timeout;   // defined in the testcase common code

int start4(char *arg)
{
    int  kidpid, status;
    int  i;
    char buf[4][12];

    testcase_timeout = 60;

    USLOSS_Console("start4(): Spawn 4 children. Each child writes one line to each terminal\n");
    for (i = 0; i < 4; i++) {
        sprintf(buf[i], "%d", i);
        Spawn("Child", Child, buf[i], 2 * USLOSS_MIN_STACK, 4, &kidpid);
    }

    USLOSS_Console("start4(): calling Wait four times\n");
    for (i = 0; i < 4; i++)
        Wait(&kidpid, &status);

    USLOSS_Console("start4(): calling Terminate\n");
    Terminate(0);

    USLOSS_Console("start4(): should not see this message!\n");
    return 0;    // so that gcc won't complain
}



int Child(char *arg)
{
    char buffer[MAXLINE];
    int  result, size;
    int  unit = atoi(arg);
    int  i;

    USLOSS_Console("Child(%d): started\n", unit);

    sprintf(buffer, "Child %d: A Something interesting to print here...", unit);
    switch(unit) {
    case 0:
        strcat(buffer, "zero\n");
        break;
    case 1:
        strcat(buffer, "one\n");
        break;
    case 2:
        strcat(buffer, "second\n");
        break;
    case 3:
        strcat(buffer, "three\n");
        break;
    }

    for (i = 0; i < 4; i++)
    {
        result = TermWrite(buffer, strlen(buffer), i, &size);
        if ( result < 0 || size != strlen(buffer) )
        {
            USLOSS_Console("\n ***** Child(%d): ", unit);
            USLOSS_Console("got bad result or bad size! *****\n\n");
        }
        USLOSS_Console("Child(%d): done with write #%d\n", unit, i);
    }

    USLOSS_Console("Child(%d): terminating\n", unit);
    Terminate(1);

    USLOSS_Console("Child(%d): should not see this message!\n", unit);
    return 0;    // so that gcc won't complain
}

