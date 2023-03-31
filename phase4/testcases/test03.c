/* TERMTEST
 * Test reading from a terminal which doesn't have as many bytes as we ask for.
 */

#include <stdio.h>
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

int XXterm2(char *arg);



int start4(char *arg)
{
    int kidpid, pid, status;
    
    USLOSS_Console("start4(): started\n");
  
    status=Spawn("XXterm2", XXterm2, NULL, USLOSS_MIN_STACK, 4, &kidpid);
    assert(status==0);
  
    Wait(&pid, &status);
  
    USLOSS_Console("start4(): XXterm2 completed. kidpid = %d, pid = %d\n", kidpid, pid);
    Terminate(0);
}



int XXterm2(char *arg)
{
    int j, len;
    char data[256];
    int result;
  
    USLOSS_Console("XXterm2(): started\n");

    // line 0, does not have as many bytes as we asked for
    len = 0;
    bzero(data, 256);
    USLOSS_Console("XXterm2(): reading a line from terminal 2\n");
    result = TermRead(data, 80, 2, &len);  // ask for 80 bytes
    if (result < 0)
        USLOSS_Console("XXterm2(): ERROR: TermRead returned %d\n", result);

    USLOSS_Console("XXterm2(): after TermRead()\n");
    USLOSS_Console("XXterm2(): term2 read   %d bytes (hex): ", len);
    for (j = 0; j < len; j++)
        USLOSS_Console(" %02x", data[j]);
    USLOSS_Console("\n");
    USLOSS_Console("                 strlen %d buf  (text): '%s'\n", strlen(data), data);
  
    USLOSS_Console("XXterm2(): Terminating\n");
    Terminate(4);
}

