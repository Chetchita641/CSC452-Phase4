#include <stdlib.h>
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
#define SIZE 2

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
	int start_block;
	int operation;
	int response_status;
	struct disk_list_node* next;
}disk_list_node;

typedef struct track_list_node{
	int mailbox_num;
	int response;
	struct track_list_node* next;
}track_list_node;

long time_counter;
int curr_track;
int track_count0;
int track_count1;
sleep_list_node* sleep_list;
disk_list_node* disk_list0;
disk_list_node* disk_list1;

track_list_node* track_list0;
track_list_node* track_list1;

int sleep_daemon(char*);
int disk_daemon(char*);
void get_track_count(int unit);
int get_tracks(char* args);

void add_sleep_list(int pid, long wake_up_time);

int disk0_mutex_mailbox_num;
void disk_lock0();
void disk_unlock0();

int disk1_mutex_mailbox_num;
void disk_lock1();
void disk_unlock1();

int track_count0_mutex_mailbox_num;
void track_count_lock0();
void track_count_unlock0();

int track_count1_mutex_mailbox_num;
void track_count_lock1();
void track_count_unlock1();

int track_list0_mutex_mailbox_num;
void track_list_lock0();
void track_list_unlock0();

int track_list1_mutex_mailbox_num;
void track_list_lock1();
void track_list_unlock1();
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

	track_count0_mutex_mailbox_num = MboxCreate(1,0);	
	track_count1_mutex_mailbox_num = MboxCreate(1,0);	
	
	track_list0_mutex_mailbox_num = MboxCreate(1,0);	
	track_list1_mutex_mailbox_num = MboxCreate(1,0);	
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
	fork1("disk_daemon0", disk_daemon, (void*)(long)unit0, USLOSS_MIN_STACK, 1);
	fork1("disk_daemon1", disk_daemon, (void*)(long)unit1, USLOSS_MIN_STACK, 1);

}

/** 
 * Performs a raed of one of the terminals; an entire line will be read. This line will either end with a newline, or be exactly MAXLINE characters long (will need to do MAXLINE+1 for buffer). If the syscall asks for a shorter line than is ready in the buffer, only part of the buffer will be copied and the rest discarded.
 * System Call: SYS_TERMREAD
 * System Call Arguments:
 *	arg1: buffer pointer
 * 	arg2: length of the buffer
 * 	arg3: which terminal to read
 * System Call Outputs:
 * 	arg2: number of characters read
 * 	arg4: -1 if illegal values were given as input; 0 otherwise
*/
void TermRead_handler(USLOSS_Sysargs *args) {}


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
	
	int unit = (int)(long) args->arg1;
	args->arg1 = (void*)(long)512;
	USLOSS_Console("getting size of disk %d\n", unit);
	args->arg2 = (void*)(long)16;
	if(unit==0){
		track_count_lock0();
		int count = track_count0;
		track_count_unlock0();
		if(count>-1){
			if(DEBUG)
				USLOSS_Console("size of disk %d was already set \n", unit);
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
			if(DEBUG)
				USLOSS_Console("size of disk %d was already set \n", unit);
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
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	MboxRelease(my_mailbox_num);
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
	new_node.start_block = (int)(long)args->arg4;
	new_node.mailbox_num = my_mailbox_num;
	new_node.operation = READ;
	new_node.response_status = 0;
	new_node.next = NULL;
	
	if(unit==0){
	
		disk_lock0();

		if(disk_list0==NULL){
			disk_list0 = &new_node;
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
			disk_list1 = &new_node;
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
	new_node.start_block = (int)(long)args->arg4;
	new_node.operation = WRITE;
	new_node.response_status = 0;
	new_node.next = NULL;

	if(unit==0){
	
		disk_lock0();

		if(disk_list0==NULL){
			disk_list0 = &new_node;
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
			disk_list1 = &new_node;
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
	if(TRACE){
		USLOSS_Console("In Sleep handler\n");
		dumpProcesses();
	}
	int pid = getpid();
	long seconds = (long)args->arg1;
	long wake_up_time = time_counter + seconds*10;
	
	sleep_list_node new_node;
	new_node.pid = pid;
	new_node.wake_up_time = wake_up_time;
	new_node.next = NULL;
	
	if(sleep_list==NULL){
		if(DEBUG)
			USLOSS_Console("sleep_list was null\n");		
		sleep_list = &new_node;
	}
	else{
		if(DEBUG)
			USLOSS_Console("Adding not at head");
		sleep_list_node* curr = sleep_list;
		while(curr->next!=NULL && (curr->next->wake_up_time < new_node.wake_up_time)){
			curr = curr->next;
		}
		if(DEBUG)
			USLOSS_Console("After while loop");
		new_node.next = curr->next;
		curr->next = &new_node;
	}
	if(DEBUG)
		dumpProcesses();
	blockMe(40);

	args->arg4 = 0;
}

void add_sleep_list(int pid, long wake_up_time){
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
}

void get_track_count(int unit){
	if(DEBUG)
		USLOSS_Console("in get track count\n");
	USLOSS_DeviceRequest req;
	int num;
	int status;
	req.opr = USLOSS_DISK_TRACKS;
	req.reg1 = &num;
	USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
	if(DEBUG)
		USLOSS_Console("Before wiat decive\n");
	waitDevice(USLOSS_DISK_DEV, unit, &status);
	if(DEBUG)
		USLOSS_Console("After wait device\n");
	if(unit==0){
		track_count_lock0();
		track_count0 = num;
		track_count_unlock0();
		track_list_lock0();
		track_list_node* curr = track_list0;
		
		while(curr!=NULL){
			if(DEBUG)
				USLOSS_Console("unblcok waiting proc in get track\n");
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
			if(DEBUG)
				USLOSS_Console("unblcok waiting proc in get track\n");
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
		USLOSS_Console("Top of deamon loop\n");
		if(disk_list0==NULL){
			// Send dummy operation
			if(DEBUG)
				USLOSS_Console("Disk queue empty, sending dummy op\n");
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
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
		// TODO: decide what to do and do it
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
				else if(curr->sectors == 0){
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
					curr->response_status = status;
					void* empty_message = "";
					MboxSend(curr->mailbox_num, empty_message, 0);
				}
				else{
					// TODO: Check if crossed into next sector
					// if so, need to seek again.

					int block = curr->start_block;
					curr->sectors--;
					curr->start_block++;
					
					int block_index = block;
					char *buf = curr->buffer;
					
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
