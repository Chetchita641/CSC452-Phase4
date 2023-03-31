/* TERMTEST
 * Spawn off 8 children. The children either read a line each from each
 * terminal, or write a line each to each terminal.
 */

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
    int i;

    USLOSS_Console("Child%d(): start\n", term);

    for (i = 0; i< 5; i++)
    {
       if (TermRead(buf, MAXLINE, term, &read_length) < 0)
       {
          USLOSS_Console("ERROR: ReadTeam\n");
          return -1;
       }
       else
       {
          buf[read_length] = '\0';
          USLOSS_Console("Child%d(): read %s", term, buf);
       }
    }

    USLOSS_Console("Child%d(): done\n", term);
    Terminate(0);
}



int Child2(char *arg)
{
    char buffer[MAXLINE];
    int  result, size;
    int  unit = atoi(arg);
    int  i;

    USLOSS_Console("Child_2%d(): start\n", unit);
    for (i = 0; i< 5; i++)
    {
       sprintf(buffer, "Child %d: A Something interesting to print here... line %d ", unit, i);
       switch(unit)
       {
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
       case 4:
          strcat(buffer, "quatro\n");
          break;
       }
       USLOSS_Console("Child_2%d(): writing to term%d\n", unit,unit);

       result = TermWrite(buffer, strlen(buffer), unit, &size);
       if (result < 0 || size != strlen(buffer))
          USLOSS_Console("\n ***** Child(%d): got bad result or bad size! *****\n\n", unit);
    }

    Terminate(0);
    USLOSS_Console("Child(%d): should not see this message!\n", unit);
}



extern int testcase_timeout;   // defined in the testcase common code

int start4(char *arg)
{
    int  pid, status, i;
    char buf[4][12];
    char child_buf[12];

    testcase_timeout = 60;

    USLOSS_Console("start4(): Spawn eight children.  \n");
    USLOSS_Console("          4 write 5 lines to a diff terminal.\n");
    USLOSS_Console("          4 read 5 lines to a diff terminal.\n");

    for (i = 0; i < 4; i++)
    {
       sprintf(buf[i], "%d", i);

       sprintf(child_buf, "Child%d", i);
       status = Spawn(child_buf, Child1, buf[i], USLOSS_MIN_STACK,2, &pid);
       assert(status == 0);

       sprintf(child_buf, "Child%d", i+4);
       status = Spawn(child_buf, Child2, buf[i], USLOSS_MIN_STACK,2, &pid);
       assert(status == 0);
    }

    for (i = 0; i < 8; i++)
    {
       Wait(&pid, &status);
       assert(status == 0);
    }

    USLOSS_Console("start4(): done.\n");
    Terminate(0);
}

