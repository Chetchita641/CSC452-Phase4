/* TERMTEST
 * Spawn off 4 children. 
 * Each child reads one line from a different terminal.
 * Child0 reads from term0.in
 * Child1 reads from term1.in
 * etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <usloss.h>
#include <usyscall.h>

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase3_usermode.h>
#include <phase4.h>
#include <phase4_usermode.h>



int Child(char *arg) 
{
    int term = atoi(arg);
    char buf[80] = "";
    int len, read_length;
  
    USLOSS_Console("Child%d(): start\n", term);

    len = sizeof(buf);
    int rc = TermRead(buf, len, term, &read_length);

    if (rc < 0)
    {
        USLOSS_Console("Child%d(): ERROR -- TermRead\n", term);
        return -1;
    }

    buf[read_length] = '\0';
    USLOSS_Console("Child%d(): read %s", term, buf);
  
    USLOSS_Console("Child%d(): done\n", term);
    Terminate(0);
}



int start4(char *arg)
{
    int  pid, status, i;
    char buf[4][12];
    char child_buf[12];

    USLOSS_Console("start4(): Spawn four children.  Each child reads from a different terminal.  The child reading the shortest line will finish first, etc.\n");

    for (i = 0; i < 4; i++) {
        sprintf(buf[i], "%d", i);
        sprintf(child_buf, "Child%d", i);
        status = Spawn(child_buf, Child, buf[i], USLOSS_MIN_STACK, 4, &pid);
        assert(status == 0);
    }

    for (i = 0; i < 4; i++) {
        Wait(&pid, &status);
        assert(status == 0);
    }

    USLOSS_Console("start4(): done.\n");
    Terminate(0);
}

