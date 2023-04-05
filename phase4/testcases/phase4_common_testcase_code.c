
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>

#include "phase3_usermode.h"
#include "phase4_usermode.h"



/* a testcase can override this value in their start4() function.  But
 * start4() runs *AFTER* we create the timeout, so how does that work?
 * Simple!  We sleep the timeout proc for 1 second, then sleep for
 * this value minus 1.  :)
 */
int testcase_timeout = 10;



/* dummy MMU structure, used for debugging whether the student called thee
 * right functions.
 */
int dummy_mmu_pids[MAXPROC];

int mmu_init_count = 0;
int mmu_quit_count = 0;
int mmu_flush_count = 0;



void startup(int argc, char **argv)
{
    for (int i=0; i<MAXPROC; i++)
        dummy_mmu_pids[i] = -1;

    /* all student implementations should have PID 1 (init) */
    dummy_mmu_pids[1] = 1;

    phase1_init();
    phase2_init();
    phase3_init();
    phase4_init();
    startProcesses();
}



/* force the testcase driver to priority 1, instead of the
 * normal priority for testcase_main
 */
int start4(char*);
static int start4_trampoline(char*);

static int testcase_timeout_proc(char*);

int testcase_main()
{
    int pid_fork, pid_join;
    int status;

    //fork1("testcase_timeout", testcase_timeout_proc, "ignored", USLOSS_MIN_STACK, 5);

    pid_fork = fork1("start4", start4_trampoline, "start4", 4*USLOSS_MIN_STACK, 3);
    pid_join = join(&status);

    if (pid_join != pid_fork)
    {
        USLOSS_Console("*** TESTCASE FAILURE *** - the join() pid doesn't match the fork() pid.  %d/%d\n", pid_fork,pid_join);
        USLOSS_Halt(1);
    }

    return status;
}

static int testcase_timeout_proc(char *ignored)
{
    /* so that we can use the Sleep() syscall, I force this function into usermode.
     * But if we're going to do that, why not just make this a child of start4()?
     * Simply because then Terminate() would wait to join() on this process.
     */
    if (USLOSS_PsrSet(USLOSS_PSR_CURRENT_INT) != USLOSS_DEV_OK)
    {
        USLOSS_Console("ERROR: Could not disable kernel mode.\n");
        USLOSS_Halt(1);
    }

    Sleep(1);
    Sleep(testcase_timeout-1);

    USLOSS_Console("TESTCASE TIMED OUT!!!\n");
    USLOSS_Halt(1);

    return 0;
}

static int start4_trampoline(char *arg)
{
    if (USLOSS_PsrSet(USLOSS_PSR_CURRENT_INT) != USLOSS_DEV_OK)
    {
        USLOSS_Console("ERROR: Could not disable kernel mode.\n");
        USLOSS_Halt(1);
    }

    int rc = start4(arg);

    Terminate(rc);
    return 0;    // Terminate() should never return
}



void mmu_init_proc(int pid)
{
    mmu_init_count++;

    int slot = pid % MAXPROC;

    if (dummy_mmu_pids[slot] != -1)
    {
        USLOSS_Console("TESTCASE ERROR: mmu_init_proc(%d) called, when the slot was already allocated for process %d\n", pid, dummy_mmu_pids[slot]);
        USLOSS_Halt(1);
    }

    dummy_mmu_pids[slot] = pid;
}

void mmu_quit(int pid)
{
    mmu_quit_count++;

    int slot = pid % MAXPROC;

    if (dummy_mmu_pids[slot] != pid)
    {
        USLOSS_Console("TESTCASE ERROR: mmu_quit(%d) called, but the slot didn't contain the expected pid.  slot: %d\n", pid, dummy_mmu_pids[slot]);
        USLOSS_Halt(1);
    }

    dummy_mmu_pids[slot] = -1;
}

void mmu_flush(void)
{
    /* this function is sometimes called with current==NULL, and so we cannot
     * reasonably expect to know the current PID.  So this has to be a NOP in
     * this testcase, except for counting how many times it is called.
     */
    mmu_flush_count++;
}



void phase5_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}



void finish(int argc, char **argv)
{
    USLOSS_Console("%s(): The simulation is now terminating.\n", __func__);
}

void test_setup  (int argc, char **argv) {}
void test_cleanup(int argc, char **argv) {}

