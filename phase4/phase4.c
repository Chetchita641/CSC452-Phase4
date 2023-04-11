#include <stdlib.h>
#include <string.h>
#include <usloss.h>
#include <usyscall.h>
#include "phase1.h"
#include "phase2.h"
#include "phase3.h"
#include "phase4.h"

// Sys call handler declaration
void Sleep_handler(USLOSS_Sysargs *args);
void TermRead_handler(USLOSS_Sysargs *args);
void TermWrite_handler(USLOSS_Sysargs *args);
void DiskSize_handler(USLOSS_Sysargs *args);
void DiskRead_handler(USLOSS_Sysargs *args);
void DiskWrite_handler(USLOSS_Sysargs *args);

#define TRACE 0
#define DEBUG 0

#define READ 0
#define WRITE 1

#define MAX_TERM_BUFFERS 10

typedef struct sleep_list_node {
	int pid;
	long wake_up_time;
	struct sleep_list_node* next;
}sleep_list_node;

typedef struct disk_list_node{
	int pid;
	int mailbox_num;
	int track;
	USLOSS_Sysargs *args;
	int operation;
	int response_status;
	struct disk_list_node* next;
}disk_list_node;

typedef struct term_data {
	int read_mb;
	int write_mb;
	char buffer[MAXLINE+1];
} term_data;

term_data terminals[USLOSS_MAX_UNITS];

long time_counter;
sleep_list_node* sleep_list;
disk_list_node* disk_list;

int sleep_daemon(char*);
int disk_daemon(char*);
int term_daemon(char*);

int disk_mutex_mailbox_num;
void disk_lock();
void disk_unlock();

void add_to_buffer(char c, int termNum);
void flush_term_buffer(int termNum);

int terminal_locks[USLOSS_MAX_UNITS];
void terminal_lock(int termNum);
void terminal_unlock(int termNum);

void termWriting(int termNum, char* to_write);
void termReading(int termNum);

// Core Functions
/////////////////////////////////////////////////////////////////////////////////
/**
* Called by the testcase during bootstrap. Initializes the data structures
* needed for this phase.
* 
* May Block: no
* May Context Swtich: no
*/ 
void phase4_init(void) {
	time_counter = 0;

	systemCallVec[SYS_SLEEP] = Sleep_handler;
	systemCallVec[SYS_TERMREAD] = TermRead_handler;
	systemCallVec[SYS_TERMWRITE] = TermWrite_handler;
	systemCallVec[SYS_DISKSIZE] = DiskSize_handler;
	systemCallVec[SYS_DISKREAD] = DiskRead_handler;
	systemCallVec[SYS_DISKWRITE] = DiskWrite_handler;

	disk_mutex_mailbox_num = MboxCreate(1,0);

	for (int i = 0; i < USLOSS_MAX_UNITS; i++) {
		term_data td;
		td.read_mb = MboxCreate(MAX_TERM_BUFFERS,MAXLINE+1);
		td.write_mb = MboxCreate(1,MAXLINE+1);
		memset(td.buffer,0,MAXLINE+1);
		terminals[i] = td; 

		// Activating locks for terminal locks
		terminal_locks[i] = MboxCreate(1,0);
	}
}

/**
* Implements any service processes needed for this phase. Called once 
* processes are running, but before the testcase begins
* 
* May Block: no
* May Context Switch: no 
*/
void phase4_start_service_processes(void) {
	fork1("sleep_daemon", sleep_daemon, "", USLOSS_MIN_STACK, 1);
	fork1("disk_daemon", disk_daemon, "", USLOSS_MIN_STACK, 1);
	fork1("term_daemon_0", term_daemon, "0", USLOSS_MIN_STACK, 1);
	fork1("term_daemon_1", term_daemon, "1", USLOSS_MIN_STACK, 1);
	fork1("term_daemon_2", term_daemon, "2", USLOSS_MIN_STACK, 1);
	fork1("term_daemon_3", term_daemon, "3", USLOSS_MIN_STACK, 1);
}

/** 
 * Performs a read of one of the terminals; an entire line will be read. This line will either end with a newline, or be exactly MAXLINE characters long (will need to do MAXLINE+1 for buffer). If the syscall asks for a shorter line than is ready in the buffer, only part of the buffer will be copied and the rest discarded.
 * System Call: SYS_TERMREAD
 * System Call Arguments:
 *	arg1: buffer pointer
 * 	arg2: length of the buffer
 * 	arg3: which terminal to read
 * System Call Outputs:
 * 	arg2: number of characters read
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/

void TermRead_handler(USLOSS_Sysargs *args) {
	if (TRACE)
		USLOSS_Console("TRACE: In TermRead handler\n");

	char* buffer = (char*)(long) args->arg1;
	int bufferSize = (int)(long) args->arg2;
	int termNum = (int)(long) args->arg3;

	
	if (bufferSize <= 0 || bufferSize > MAXLINE) {
		args->arg2 = 0;
		args->arg4 = (void*)(long) -1;
		return;
	}
	if (termNum < 0 || termNum > USLOSS_MAX_UNITS) {
		args->arg2 = 0;
		args->arg4 = (void*)(long) -1;
		return;
	}

	term_data* term_ptr = &terminals[termNum];
	if (DEBUG) {
		USLOSS_Console("DEBUG: Right before receiving message\n");
		dumpProcesses();
	}

	MboxRecv(term_ptr->read_mb, buffer, bufferSize);

	int charsRead = strlen(buffer);
	args->arg2 = (void*)(long) charsRead;
	args->arg4 = 0;
}

/** 
 * Writes characters from a buffer to a terminal. All of the character of the buffer will be written atomically; no other process can write to the terminal until they have flushed.
 * System Call: SYS_TERMWRITE
 * System Call Arguments:
 *	arg1: buffer pointer
 * 	arg2: length of the buffer
 * 	arg3: which terminal to write to
 * System Call Outputs:
 * 	arg2: number of characters written
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void TermWrite_handler(USLOSS_Sysargs *args) {
	if (TRACE)
		USLOSS_Console("TRACE: In TermWrite handler\n");

	char* buffer = (char*)(long) args->arg1;
	int bufferSize = (int)(long) args->arg2;
	int termNum = (int)(long) args->arg3;

	if (bufferSize < 0 || bufferSize > MAXLINE) {
		args->arg2 = 0;
		args->arg4 = (void*)(long) -1;
		return;
	}

	term_data* term_ptr = &terminals[termNum];
	
	MboxSend(term_ptr->write_mb, buffer, bufferSize);

	args->arg2 = (void*)(long) bufferSize;
	args->arg4 = 0;
}

/** 
 * Queries the size of a given disk. It returns three values, all as out-parameters.
 * System Call: SYS_DISKSIZE
 * System Call Arguments:
 *	arg1: the disk to query
 * System Call Outputs:
 * 	arg1: size of a block, in bytes
 * 	arg2: number of blocks in track
 * 	arg3: number of tracks in the disk
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void DiskSize_handler(USLOSS_Sysargs *args) {}

/** 
 * Reads a certain number of blocks from disk, sequentially. Once begun, the entire read is atomic.
 * System Call: SYS_DISKREAD
 * System Call Arguments:
 *	arg1: buffer pointer
 * 	arg2: number of sectors to read
 * 	arg3: starting track number
 *	arg4: starting block number
 *	arg5: which disk to access
 * System Call Outputs:
 * 	arg1: 0 if transfer was successful; the disk status register otherwise
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void DiskRead_handler(USLOSS_Sysargs *args) {
	int track = (int)(long) args->arg3;	
	int my_mailbox_num = MboxCreate(1,0);
	disk_list_node new_node;
	new_node.pid = getpid();
	new_node.track = track;
	new_node.args = args;
	new_node.mailbox_num = my_mailbox_num;;
	new_node.operation = READ;
	new_node.response_status = 0;
	new_node.next = NULL;
	
	disk_lock();

	if(disk_list==NULL){
		disk_list = &new_node;
	}
	else{
		disk_list_node* curr = disk_list;
		if(new_node.track < curr->track){
			// Inserting value that is less than first value, put at end
			// Itterate to end of front part of list
			while(curr->next!=NULL && curr->track <= curr->next->track){
				curr = curr->next;
			}
			// Now can just insert like normal
		}

		while(curr->next!=NULL && (curr->next->track < new_node.track)){
			curr = curr->next;
		}
		new_node.next = curr->next;
		curr->next = &new_node;
	}

	disk_unlock();
	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	
	// Operation is finished
	args->arg1 = (void*)(long)0;
	args->arg4 = (void*)(long)new_node.response_status;
}

/** 
 * Writes a certain number of blocks to disk, sequentially. Once begun, the entire write is atomic.
 * System Call: SYS_DISKWRITE
 * System Call Arguments:
 *	arg1: buffer pointer
 * 	arg2: number of sectors to write
 * 	arg3: starting track number
 *	arg4: starting block number
 *	arg5: which disk to access
 * System Call Outputs:
 * 	arg1: 0 if transfer was successful; the disk status register otherwise
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void DiskWrite_handler(USLOSS_Sysargs *args) {
	int track = (int)(long) args->arg3;	
	int my_mailbox_num = MboxCreate(1,0);
	disk_list_node new_node;
	new_node.pid = getpid();
	new_node.track = track;
	new_node.mailbox_num = my_mailbox_num;
	new_node.args = args;
	new_node.operation = WRITE;
	new_node.response_status = 0;
	new_node.next = NULL;
	
	disk_lock();

	if(disk_list==NULL){
		disk_list = &new_node;
	}
	else{
		disk_list_node* curr = disk_list;
		if(new_node.track < curr->track){
			// Inserting value that is less than first value, put at end
			// Itterate to end of front part of list
			while(curr->next!=NULL && curr->track <= curr->next->track){
				curr = curr->next;
			}
			// Now can just insert like normal
		}

		while(curr->next!=NULL && (curr->next->track < new_node.track)){
			curr = curr->next;
		}
		new_node.next = curr->next;
		curr->next = &new_node;
	}

	disk_unlock();
	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	
	//Operation is complete
	args->arg1 = (void*)(long)0;
	args->arg4 = (void*)(long)new_node.response_status;;
}

/** 
 * Pauses the current process for a specified number of seconds (The delay is approximate.)
 * System Call: SYS_SLEEP
 * System Call Arguments:
 *	arg1: seconds
 * System Call Outputs:
 *	arg4: -1 if illegal values were given as input; 0 otherwise
 */
void Sleep_handler(USLOSS_Sysargs *args) {
	// Add myself to queue
	// Block me
	if(TRACE){
		USLOSS_Console("In Sleep handler\n");
	}
	int pid = getpid();
	long seconds = (long)args->arg1;
	long wake_up_time = time_counter + seconds*10;
	
	sleep_list_node new_node;
	new_node.pid = pid;
	new_node.wake_up_time = wake_up_time;
	new_node.next = NULL;
	
	if(sleep_list==NULL){
		sleep_list = &new_node;
	}
	else{
		sleep_list_node* curr = sleep_list;
		while(curr->next!=NULL && (curr->next->wake_up_time < new_node.wake_up_time)){
			curr = curr->next;
		}
		new_node.next = curr->next;
		curr->next = &new_node;
	}
	blockMe(40);

	args->arg4 = 0;
}

int term_daemon(char* arg) {
	int status;
	int termNum = atoi(arg);
	term_data* term_ptr = &terminals[termNum];
	
	char to_write[MAXLINE+1];
	memset(to_write,0,MAXLINE+1);
	
	// Enabling recv and xmit interrupts for the terminals
	USLOSS_DeviceOutput(USLOSS_TERM_DEV, termNum, (void*) 6);

	while (1) {
		waitDevice(USLOSS_TERM_DEV, termNum, &status);
		//USLOSS_DeviceInput(USLOSS_TERM_DEV, termNum, &status);
		
		MboxCondRecv(term_ptr->write_mb, to_write, MAXLINE);
		if (USLOSS_TERM_STAT_XMIT(status) == USLOSS_DEV_READY && strlen(to_write) > 0) {
			terminal_lock(termNum);

			termWriting(termNum, to_write);
			memset(to_write,0,MAXLINE+1);

			terminal_unlock(termNum);
		}

		if (USLOSS_TERM_STAT_RECV(status) == USLOSS_DEV_BUSY) {
			terminal_lock(termNum);

			char c = USLOSS_TERM_STAT_CHAR(status);
			int len = (int) strlen(term_ptr->buffer);
			term_ptr->buffer[len] = c;

			if (c == '\0' || c == '\n' || len+1 >= MAXLINE) {
				termReading(termNum);				
			}

			terminal_unlock(termNum); 
		}
	}
}

void termWriting(int termNum, char* to_write) {
	int status;
	char* w = to_write;
	int ctrl;
	while (*w != '\0' && *w != '\n') {
		ctrl = USLOSS_TERM_CTRL_CHAR(7,*w);
		USLOSS_DeviceOutput(USLOSS_TERM_DEV, termNum, (void*) ctrl);
		waitDevice(USLOSS_TERM_DEV, termNum, &status);
		while (USLOSS_TERM_STAT_XMIT(status) == USLOSS_DEV_BUSY) {
			waitDevice(USLOSS_TERM_DEV, termNum, &status);
		}
		w++;	
	}
	
	ctrl = USLOSS_TERM_CTRL_CHAR(7,*w);
	USLOSS_DeviceOutput(USLOSS_TERM_DEV, termNum, (void*) ctrl);
}

void termReading(int termNum) {
	term_data* term_ptr = &terminals[termNum];
	char tempBuf[MAXLINE+1];
	
	strcpy(tempBuf, term_ptr->buffer);
	memset(term_ptr->buffer, 0, MAXLINE+1);

	MboxCondSend(term_ptr->read_mb, tempBuf, strlen(tempBuf));
}

int sleep_daemon(char* arg){
	int status;
	while(1){
		waitDevice(USLOSS_CLOCK_DEV, 0, &status);
		time_counter++;
		// check queue
		while(sleep_list!=NULL && sleep_list->wake_up_time<=time_counter){
			int pid = sleep_list->pid;
			sleep_list = sleep_list->next; 
			unblockProc(pid);	
		}
	}
	return 0;
}

int disk_daemon(char* arg){
	int status;
	while(1){
		waitDevice(USLOSS_DISK_DEV, 0, &status);
		disk_lock();
		// grab next proc off queue
		disk_list_node* curr = disk_list;
		disk_unlock();
		// TODO: decide what to do and do it
		
		// If operation is done remove from queue
		disk_lock();
		disk_list = disk_list->next;
		disk_unlock();
		// Wake up the process for this operation
		// TODO: Response from disk
		curr->response_status = status;
		void* empty_message = "";
		MboxSend(curr->mailbox_num, empty_message, 0);
	}
	return 0;
}

// Helper Functions
/////////////////////////////////////////////////////////////////////////////////
/**
* Acquire lock for disk queue
*/
void disk_lock(){
	void* empty_message = "";
	MboxSend(disk_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for disk queue
*/
void disk_unlock(){	
	void* empty_message = "";
	MboxRecv(disk_mutex_mailbox_num, empty_message, 0);
}

/** 
* Acquire lock for a given terminal
*/
void terminal_lock(int termNum) {
	void* empty_message = "";
	int term_mailbox = terminal_locks[termNum];
	MboxSend(term_mailbox, empty_message, 0);
}

/** 
* Release lock for a given terminal
*/
void terminal_unlock(int termNum) {
	void* empty_message = "";
	int term_mailbox = terminal_locks[termNum];
	MboxRecv(term_mailbox, empty_message, 0);
}

