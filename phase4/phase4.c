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

#define TRACE 1
#define DEBUG 1

#define READ 0
#define WRITE 1
#define SIZE 2

#define MAX_TERM_BUFFERS 10

typedef struct sleep_list_node {
	int pid;
	long wake_up_time;
	struct sleep_list_node* next;
}sleep_list_node;

typedef struct disk_list_node{
	int pid;
	int started;
	int mailbox_num;
	char* buffer;
	int track;
	int sectors;
	int sectors_done;
	int start_block;
	int operation;
	int response_status;
	struct disk_list_node* next;
}disk_list_node;

typedef struct term_data {
	int mailbox_num;
	char buffers[MAX_TERM_BUFFERS][MAXLINE+1];
	int buffer_idx;
	int msgs_waiting;
} term_data;

term_data terminals[USLOSS_MAX_UNITS];

long time_counter;
int curr_track;
int track_count0;
int track_count1;
sleep_list_node* sleep_list;
disk_list_node* disk_list;

int sleep_daemon(char*);
int disk_daemon(char*);
int term_read_daemon(char*);

int disk_mutex_mailbox_num;
void disk_lock();
void disk_unlock();
void add_to_buffer(char c, int termNum);

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
	curr_track = 0;
	track_count0 = -1;
	track_count1 = -1;

	systemCallVec[SYS_SLEEP] = Sleep_handler;
	systemCallVec[SYS_TERMREAD] = TermRead_handler;
	systemCallVec[SYS_TERMWRITE] = TermWrite_handler;
	systemCallVec[SYS_DISKSIZE] = DiskSize_handler;
	systemCallVec[SYS_DISKREAD] = DiskRead_handler;
	systemCallVec[SYS_DISKWRITE] = DiskWrite_handler;

	disk0_mutex_mailbox_num = MboxCreate(1,0);
	disk1_mutex_mailbox_num = MboxCreate(1,0);

	for (int i = 0; i < USLOSS_MAX_UNITS; i++) {
		term_data td;
		td.mailbox_num = MboxCreate(MAX_TERM_BUFFERS,MAXLINE+1);
		for (int j = 0; j < MAX_TERM_BUFFERS; j++) {
			memset(terminals[i].buffers[j], 0, MAXLINE+1);
		}
		td.buffer_idx = 0;
		td.msgs_waiting = 0;	
		terminals[i] = td; 
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
	int unit0 = 0;
	int unit1 = 0;

	fork1("get_tracks", get_tracks, "", USLOSS_MIN_STACK, 1);
	fork1("sleep_daemon", sleep_daemon, "", USLOSS_MIN_STACK, 1);
	fork1("disk_daemon", disk_daemon, "", USLOSS_MIN_STACK, 1);
	fork1("term_read_daemon_0", term_read_daemon, "0", USLOSS_MIN_STACK, 1);
	fork1("term_read_daemon_1", term_read_daemon, "1", USLOSS_MIN_STACK, 1);
	fork1("term_read_daemon_2", term_read_daemon, "2", USLOSS_MIN_STACK, 1);
	fork1("term_read_daemon_3", term_read_daemon, "3", USLOSS_MIN_STACK, 1);
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
	if (term_ptr->msgs_waiting > 0) {
		char* temp_buffer;
		MboxCondRecv(term_ptr->mailbox_num, &temp_buffer, bufferSize);
		term_ptr->msgs_waiting--;
		args->arg2 = (void*)(long) strlen(temp_buffer);
	}
	else {
		args->arg2 = (void*)(long) strlen(buffer);
	}
	
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
 * 	arg2: number of characters read
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void TermWrite_handler(USLOSS_Sysargs *args) {
	 
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
void DiskSize_handler(USLOSS_Sysargs *args) {
	if(DEBUG)
		USLOSS_Console("in disk size hanlder\n");	
	int unit = (int)(long) args->arg1;
	args->arg1 = (void*)(long)512;
	args->arg2 = (void*)(long)16;
	if(unit==0){
		track_count_lock0();
		int count = track_count0;
		track_count_unlock0();
		if(count>-1){
			if(DEBUG)
				USLOSS_Console("count available");
			args->arg3 = (void*)(long)count;
			return;
		}
	}
	else{
		track_count_lock1();
		int count = track_count1;
		track_count_unlock1();
		if(count>-1){
			args->arg3 = (void*)(long)count;
			return;
		}
	}
		

	int my_mailbox_num = MboxCreate(1,0);
	
	track_list_node new_node;
	new_node.mailbox_num = my_mailbox_num;
	new_node.next = NULL;

	if(unit==0){
	
		track_list_lock0();

		if(track_list0==NULL){
			track_list0 = &new_node;
		}
		else{
			track_list_node* head = track_list0;
			
			new_node.next = head->next;
			head->next = &new_node;
		}

		track_list_unlock0();
	}
	else{
		track_list_lock1();

		if(track_list1==NULL){
			track_list1 = &new_node;
		}
		else{
			track_list_node* head = track_list1;
			new_node.next = head->next;
			head->next = &new_node;
		}

		track_list_unlock1();
	}


	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	if(DEBUG)
		USLOSS_Console("Disk size blockig\n");
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	MboxRelease(my_mailbox_num);
	if(DEBUG)
		USLOSS_Console("DISk size unblocked\n");
	int tracks = new_node.response;
	args->arg3 = (void*)(long)tracks;	
}

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
	int unit = (int)(long) args->arg5;
	
	disk_list_node new_node;
	new_node.pid = getpid();
	new_node.started = 0;
	new_node.track = track;
	new_node.buffer = args->arg1;
	new_node.sectors = (int)(long)args->arg2;
	new_node.sectors_done = 0;
	new_node.start_block = (int)(long)args->arg4;
	new_node.mailbox_num = my_mailbox_num;
	new_node.operation = READ;
	new_node.response_status = 0;
	new_node.next = NULL;
	
	if(unit==0){
	
		disk_lock0();

		if(disk_list0==NULL){

			// Send dummy operation
			track_count_lock0();
			int count = track_count0;
			track_count_unlock0();
			if(count<0){

				if(DEBUG)
					USLOSS_Console("in write, track isn;t done yet\n");
				int mailbox_num = MboxCreate(1,0);
		
				track_list_node new_node;
				new_node.mailbox_num = mailbox_num;
				new_node.next = NULL;

	
				track_list_lock0();
	
				if(track_list0==NULL){
					track_list0 = &new_node;
				}
				else{
					track_list_node* head = track_list1;
			
					new_node.next = head->next;
					head->next = &new_node;
				}

				track_list_unlock0();
				
				void* empty_message = "";
				MboxRecv(mailbox_num, empty_message, 0);	
				MboxRelease(mailbox_num);
			}
			disk_list0 = &new_node;
			if(DEBUG)
				USLOSS_Console("Disk queue empty, sending dummy op track count is %d\n", track_count0);
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
		else{
			disk_list_node* curr = disk_list0;
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

		disk_unlock0();
	}
	else{
		disk_lock1();

		if(disk_list1==NULL){

			// Send dummy operation
			
			track_count_lock1();
			int count = track_count1;
			track_count_unlock1();
			if(count<0){

				if(DEBUG)
					USLOSS_Console("in write, track isn;t done yet\n");
				int mailbox_num = MboxCreate(1,0);
		
				track_list_node new_node;
				new_node.mailbox_num = mailbox_num;
				new_node.next = NULL;

	
				track_list_lock1();
	
				if(track_list1==NULL){
					track_list1 = &new_node;
				}
				else{
					track_list_node* head = track_list1;
			
					new_node.next = head->next;
					head->next = &new_node;
				}

				track_list_unlock1();
				
				void* empty_message = "";
				MboxRecv(mailbox_num, empty_message, 0);	
				MboxRelease(mailbox_num);
			}
			disk_list1 = &new_node;
			if(DEBUG)
				USLOSS_Console("Disk queue empty, sending dummy op track count is %d\n", track_count0);
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);

		}
		else{
			disk_list_node* curr = disk_list1;
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

		disk_unlock1();
	}


	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	MboxRelease(my_mailbox_num);
	//Operation is complete
	args->arg1 = (void*)(long)0;
	args->arg4 = (void*)(long)new_node.response_status;;
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
	if(DEBUG)
		USLOSS_Console("In write handler\n");
	int track = (int)(long) args->arg3;	
	int my_mailbox_num = MboxCreate(1,0);
	int unit = (int)(long) args->arg5;

	disk_list_node new_node;
	new_node.pid = getpid();
	new_node.started = 0;
	new_node.track = track;
	new_node.mailbox_num = my_mailbox_num;
	new_node.buffer = args->arg1;
	new_node.sectors = (int)(long)args->arg2;
	new_node.sectors_done = 0;
	new_node.start_block = (int)(long)args->arg4;
	new_node.operation = WRITE;
	new_node.response_status = 0;
	new_node.next = NULL;

	if(unit==0){
	
		disk_lock0();

		if(disk_list0==NULL){
			// Send dummy operation
			
			track_count_lock0();
			int count = track_count0;
			track_count_unlock0();
			if(count<0){

				if(DEBUG)
					USLOSS_Console("in write, track isn;t done yet\n");
				int mailbox_num = MboxCreate(1,0);
		
				track_list_node new_node;
				new_node.mailbox_num = mailbox_num;
				new_node.next = NULL;

	
				track_list_lock0();
	
				if(track_list0==NULL){
					track_list0 = &new_node;
				}
				else{
					track_list_node* head = track_list0;
			
					new_node.next = head->next;
					head->next = &new_node;
				}

				track_list_unlock0();
				
				void* empty_message = "";
				MboxRecv(mailbox_num, empty_message, 0);	
				MboxRelease(mailbox_num);
			}
			disk_list0 = &new_node;
			if(DEBUG)
				USLOSS_Console("Disk queue empty, sending dummy op track count is %d\n", track_count0);
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);

		/*	USLOSS_Sysargs args;
 			args.arg1 = (void *) ( (long) unit);
			DiskSize_handler(&args);*/
		}
		else{
			disk_list_node* curr = disk_list0;
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

		disk_unlock0();
	}
	else{
		disk_lock1();

		if(disk_list1==NULL){

			// Send dummy operation
			
			track_count_lock1();
			int count = track_count1;
			track_count_unlock1();
			if(count<0){

				if(DEBUG)
					USLOSS_Console("in write, track isn;t done yet\n");
				int mailbox_num = MboxCreate(1,0);
		
				track_list_node new_node;
				new_node.mailbox_num = mailbox_num;
				new_node.next = NULL;

	
				track_list_lock1();
	
				if(track_list1==NULL){
					track_list1 = &new_node;
				}
				else{
					track_list_node* head = track_list1;
			
					new_node.next = head->next;
					head->next = &new_node;
				}

				track_list_unlock1();
				
				void* empty_message = "";
				MboxRecv(mailbox_num, empty_message, 0);	
				MboxRelease(mailbox_num);
			}
			disk_list1 = &new_node;
			if(DEBUG)
				USLOSS_Console("Disk queue empty, sending dummy op track count is %d\n", track_count0);
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
		else{
			disk_list_node* curr = disk_list1;
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

		disk_unlock1();
	}


	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	MboxRelease(my_mailbox_num);
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

int term_read_daemon(char* arg) {
	if (TRACE)
		USLOSS_Console("TRACE: In term read daemon\n");

	int status;
	int termNum = atoi(arg);
	while (1) {
		waitDevice(USLOSS_TERM_DEV, termNum, &status);
		if (USLOSS_TERM_STAT_RECV(status)) {
			char c = USLOSS_TERM_STAT_CHAR(status);
			add_to_buffer(c, termNum);
		}
	}
	return 0;
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

int get_tracks(char* args){
	get_track_count(0);
	get_track_count(1);
	USLOSS_Sysargs sysArgs0;
 	sysArgs0.arg1 = (void *) ( (long) 0);
	DiskSize_handler(&sysArgs0);
	USLOSS_Sysargs sysArgs1;
 	sysArgs1.arg1 = (void *) ( (long) 1);
	DiskSize_handler(&sysArgs1);
}

void get_track_count(int unit){
	USLOSS_DeviceRequest req;
	int num;
	int status;
	req.opr = USLOSS_DISK_TRACKS;
	req.reg1 = &num;
	if(DEBUG)
		USLOSS_Console("Sending request in get track %d\n", unit);
	USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
	waitDevice(USLOSS_DISK_DEV, unit, &status);
	if(DEBUG)
		USLOSS_Console("After wait dev in get track %d\n", unit);
	if(unit==0){
		track_count_lock0();
		track_count0 = num;
		track_count_unlock0();
		track_list_lock0();
		track_list_node* curr = track_list0;
		
		while(curr!=NULL){
			curr->response = track_count0;
			
			void* empty_message = "";
			MboxSend(curr->mailbox_num, empty_message, 0);
			curr = curr->next;
		}
	
		track_list_unlock0();
	}
	else{
		track_count_lock1();
		track_count1 = num;
		track_count_unlock1();
		track_list_lock1();
		track_list_node* curr = track_list1;
		while(curr!=NULL){
			curr->response = track_count1;
			
			void* empty_message = "";
			MboxSend(curr->mailbox_num, empty_message, 0);
			curr = curr->next;
		}
		track_list_unlock1();
	}
}

int disk_daemon(char* arg){
	int status;
	int unit = (int)(long)arg;
	disk_list_node* curr;
	USLOSS_DeviceRequest req;
	while(1){
		if(DEBUG)
			USLOSS_Console("Before waitDevice in Disk\n");
		waitDevice(USLOSS_DISK_DEV, unit, &status);
		if(DEBUG)
			USLOSS_Console("After waitDevice in disk\n");
		// grab next proc off queue
		if(unit==0){
			disk_lock0();
			curr = disk_list0;
			disk_unlock0();
		}
		else{
			disk_lock1();
			curr = disk_list1;
			disk_unlock1();
		}
		if(curr!=NULL){
		if(status == USLOSS_DEV_ERROR){
			if(unit==0){
				disk_lock0();
				disk_list0 = disk_list0->next;
				disk_unlock0();
			}
			else{
				disk_lock1();
				disk_list1 = disk_list1->next;
				disk_unlock1();
			}
			// Wake up the process for this operation
			curr->response_status = status;
			void* empty_message = "";
			MboxSend(curr->mailbox_num, empty_message, 0);
		}
		else{
			if(curr->started){
				if(DEBUG)
					USLOSS_Console("Continuing an op\n");
				/*if(curr->operation==SIZE){
					// If operation is done remove from queue
					// grab next proc off queue
					if(unit==0){
						disk_lock0();
						curr = disk_list0;
						disk_unlock0();
					}
					else{
						disk_lock1();
						curr = disk_list1;
						disk_unlock1();
					}
					// Wake up the process for this operation
					void* empty_message = "";
					MboxSend(curr->mailbox_num, empty_message, 0);
				}*/
				// At correct track, ready to read/write
				//else if(curr->sectors == 0){
				if(curr->sectors_done == curr->sectors){
					// If operation is done remove from queue
					// grab next proc off queue
					if(DEBUG)
						USLOSS_Console("done w op\n");
					if(unit==0){
						disk_lock0();
						curr = disk_list0;
						disk_unlock0();
					}
					else{
						disk_lock1();
						curr = disk_list1;
						disk_unlock1();
					}
					// Wake up the process for this operation
					curr->response_status = status;
					void* empty_message = "";
					MboxSend(curr->mailbox_num, empty_message, 0);
				}
				else{
					// TODO: Check if crossed into next sector
					// if so, need to seek again.
					
					int block = curr->start_block;
					if(block==16){
						// Cross to next sector
						curr->start_block = 0;
						curr->track++;
						
						req.opr = USLOSS_DISK_SEEK;
						req.reg1 = curr->track;
					}
					else{
					int buff_offset = curr->sectors_done;
					curr->sectors_done++;
					curr->start_block++;
					
					int block_index = block;
					char *buf = (curr->buffer)+(buff_offset*512);
					if(DEBUG)
						USLOSS_Console("Gonna do write/read block %d blcok index %d track %d of buff %p\n", block, block_index, curr->track, buf);
					req.reg1 = (void*)(long)block_index;
					req.reg2 = buf;
					if(curr->operation==READ){
						req.opr = USLOSS_DISK_READ;
					}
					else{
						req.opr = USLOSS_DISK_WRITE;
					}
					}
				}
			}
			else{
				if(DEBUG)
					USLOSS_Console("Strating new op\n");
				curr->started = 1;
				/*if(curr->operation==SIZE){
					req.opr = USLOSS_DISK_TRACKS;
					req.reg1 = curr->response_status;
				}
				else{*/
					req.opr = USLOSS_DISK_SEEK;
					req.reg1 = curr->track;
				//}
			}
			if(DEBUG)
				USLOSS_Console("Sending a request\n");
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
		}
		
	}
	return 0;
}

// Helper Functions
/////////////////////////////////////////////////////////////////////////////////
/**
* Acquire lock for disk0 queue
*/
void disk_lock0(){
	void* empty_message = "";
	MboxSend(disk0_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for disk0 queue
*/
void disk_unlock0(){	
	void* empty_message = "";
	MboxRecv(disk0_mutex_mailbox_num, empty_message, 0);
}

/**
* Acquire lock for disk1 queue
*/
void disk_lock1(){
	void* empty_message = "";
	MboxSend(disk1_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for disk1 queue
*/
void disk_unlock1(){	
	void* empty_message = "";
	MboxRecv(disk1_mutex_mailbox_num, empty_message, 0);
}


/**
* Acquire lock for track_count0
*/
void track_count_lock0(){
	void* empty_message = "";
	MboxSend(track_count0_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for track_count0
*/
void track_count_unlock0(){	
	void* empty_message = "";
	MboxRecv(track_count0_mutex_mailbox_num, empty_message, 0);
}


/**
* Acquire lock for track_count1
*/
void track_count_lock1(){
	void* empty_message = "";
	MboxSend(track_count1_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for track_count1
*/
void track_count_unlock1(){	
	void* empty_message = "";
	MboxRecv(track_count1_mutex_mailbox_num, empty_message, 0);
}


/**
* Acquire lock for track_list0
*/
void track_list_lock0(){
	void* empty_message = "";
	MboxSend(track_list0_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for track_list0
*/
void track_list_unlock0(){	
	void* empty_message = "";
	MboxRecv(track_list0_mutex_mailbox_num, empty_message, 0);
}


/**
* Acquire lock for track_list1
*/
void track_list_lock1(){
	void* empty_message = "";
	MboxSend(track_list1_mutex_mailbox_num, empty_message, 0);
}

/**
* Release lock for track_list1
*/
void track_list_unlock1(){	
	void* empty_message = "";
	MboxRecv(track_list1_mutex_mailbox_num, empty_message, 0);
}

/** 
* Adds a character to the terminal's buffer.
* Handles rotating as well as flushing through MboxCondSend
*/
void add_to_buffer(char c, int termNum) {
	term_data* term_ptr = &terminals[termNum];
	// If this is the first character to come in, there's now at least one msg_waiting
	if (term_ptr->msgs_waiting == 0)
		term_ptr->msgs_waiting++; 

	// If the buffer is at capacity, send it through MboxCondSend with the buffer's address, then clear out the next one and place the next character there.
	if (sizeof(term_ptr->buffers[term_ptr->buffer_idx]) > MAXLINE) {
		MboxCondSend(term_ptr->mailbox_num, term_ptr->buffers[term_ptr->buffer_idx], MAXLINE);
		term_ptr->buffer_idx = (term_ptr->buffer_idx+1) % MAX_TERM_BUFFERS;
		memset(term_ptr->buffers[term_ptr->buffer_idx], 0, MAXLINE);	 
		term_ptr->msgs_waiting++;
	}
	
	size_t len = strlen(term_ptr->buffers[term_ptr->buffer_idx]);
	term_ptr->buffers[term_ptr->buffer_idx][len] = c;
}

