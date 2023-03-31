/* TERMTEST
 * Read exactly 13 bytes from term 1. Display the bytes to stdout.
 */

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

char buf[256];



int start4(char *arg)
{
    int j, length;
    char dataBuffer[256];
    int result;
  
    USLOSS_Console("start4(): Read from terminal 1, but ask for fewer chars than are present on the first line.\n");

    length = 0;
    memset(dataBuffer, 'a', 256);  // Fill dataBuffer with a's
    dataBuffer[254] = '\n';
    dataBuffer[255] = '\0';

    result = TermRead(dataBuffer, 13, 1, &length);
    if (result < 0)
    {
        USLOSS_Console("start4(): ERROR from Readterm, result = %d\n", result);
        Terminate(1);
    }	

    USLOSS_Console("start4(): term1 read %d bytes, first 13 bytes: '", length);
    USLOSS_Console(buf);
    for (j = 0; j < 13; j++)
        USLOSS_Console("%c", dataBuffer[j]);	    
    USLOSS_Console("'\n");
  
    USLOSS_Console("start4(): simple terminal test is done.\n");
    Terminate(0);
}

