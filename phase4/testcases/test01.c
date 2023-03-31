#include <stdlib.h>
#include <stdio.h>

#include <usloss.h>
#include <usyscall.h>

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase3_usermode.h>
#include <phase4.h>
#include <phase4_usermode.h>



#define ABS(a,b) (a-b > 0 ? a-b : -(a-b))



int Child(char *arg) 
{
    int begin, end, time;
    int me = atoi(arg);

    USLOSS_Console("Child%d(): Going to sleep for 10 seconds\n", me);
    GetTimeofDay(&begin);
    Sleep(10);
    GetTimeofDay(&end);

    time = end - begin;
    if (time < 10*1000*1000 || time > 13*1000*1000)
        USLOSS_Console("Child%d(): Sleep bad: %d\n", me, time);
    else
        USLOSS_Console("Child%d(): Sleep done at time %d\n", me, end);

    Terminate(1);
}



extern int testcase_timeout;   // defined in the testcase common code

int start4(char *arg)
{
    int pid, status;

    testcase_timeout = 15;
  
    USLOSS_Console("start4(): Start 5 children who all sleep for 10 seconds. Upon\n");
    USLOSS_Console("          waking up, each child checks if its sleep time was at\n");
    USLOSS_Console("          least 10 seconds.\n");

    Spawn("Child1", Child, "1", USLOSS_MIN_STACK, 4, &pid);
    Spawn("Child2", Child, "2", USLOSS_MIN_STACK, 4, &pid);
    Spawn("Child3", Child, "3", USLOSS_MIN_STACK, 4, &pid);
    Spawn("Child4", Child, "4", USLOSS_MIN_STACK, 4, &pid);
    Spawn("Child5", Child, "5", USLOSS_MIN_STACK, 4, &pid);

    Wait(&pid, &status);
    Wait(&pid, &status);
    Wait(&pid, &status);
    Wait(&pid, &status);
    Wait(&pid, &status);

    USLOSS_Console("start4(): Test sleep done.\n");
    Terminate(0);
}

