/* TERMTEST
 * Send a negative length to TermRead. Send an invalid terminal
 * number to TermWrite. The routines should return -1.
 */

#include <assert.h>

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
    int len, result;
    char aBuffer[14];
    char bBuffer[14] = "abcdefghijklm";
  
    USLOSS_Console("start4(): 3 tests of invalid requests from terminals\n");

    USLOSS_Console("start4(): Read negative number of chars from term 1\n");
    result = TermRead(aBuffer, -13, 1, &len);
    assert(result == -1);

    USLOSS_Console("start4(): Read from invalid term 5\n");
    result = TermRead(bBuffer, 14, 5, &len);
    assert(result == -1);

    USLOSS_Console("start4(): Read zero chars from term 0\n");
    result = TermRead(bBuffer, 0, 0, &len);
    assert(result == -1);

    USLOSS_Console("start4(): After reading 0 characters from terminal 0, ");
    USLOSS_Console("bBuffer contains '%s'\n", bBuffer);

    USLOSS_Console("start4(): Done with test of illegal terminal parameters\n");
    Terminate(0);
}

