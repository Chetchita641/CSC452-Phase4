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
	int sectors_done;
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
void wait_get_tracks(int unit);

void disk_helper(USLOSS_Sysargs* args, int operation);
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
	int unit1 = 1;

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
	if(DEBUG)
		USLOSS_Console("in disk size hanlder\n");	
	int unit = (int)(long) args->arg1;
	// Size of block and blocks in a track are fixed
	args->arg1 = (void*)(long)512;
	args->arg2 = (void*)(long)16;

	// Check if the global count variable is already set
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
		

	// If not ready, put myself on a queue to be woken up when
	// the global track count variable is set
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


	
	// Recv on specified mailbox, so I'm woken up at the right time
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
	disk_helper(args, READ);
}

// Debug disk queue
void print_disk1(){
	disk_list_node* curr = disk_list1;
	while(curr!=NULL){
		USLOSS_Console("%d->",curr->track);	
		curr = curr->next;
	}
	USLOSS_Console("\n");
}

// Debug disk queue
void print_disk0(){
	disk_list_node* curr = disk_list0;
	while(curr!=NULL){
		USLOSS_Console("%d->",curr->track);	
		curr = curr->next;
	}
	USLOSS_Console("\n");
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
	disk_helper(args, WRITE);
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

// Not used?
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

void disk_helper(USLOSS_Sysargs* args, int operation){

	// Access arguments
	int sectors_num = (int)(long) args->arg2;
	int track = (int)(long) args->arg3;	
	int start_block = (int)(long) args->arg4;
	int my_mailbox_num = MboxCreate(1,0);
	int unit = (int)(long) args->arg5;
	
	// Validate args
	if((unit!=0&&unit!=1)|| start_block<0 || start_block>16){
		args->arg4 = -1;
		return;
	}

	// Create node for disk queue
	disk_list_node new_node;
	new_node.pid = getpid();
	new_node.started = 0;
	new_node.track = track;
	new_node.mailbox_num = my_mailbox_num;
	new_node.buffer = args->arg1;
	new_node.sectors = sectors_num;
	new_node.sectors_done = 0;
	new_node.start_block = start_block;
	new_node.operation = operation;
	new_node.response_status = 0;
	new_node.next = NULL;

	if(unit==0){
	
		disk_lock0();
		if(disk_list0==NULL){

			// No one on queue (disk daemon idle)
			
			disk_list0 = &new_node;
			disk_unlock0();
			
			// Need to wait until track count is done being set, so the
			// process getting track count doesn't "steal" this operation
			// we are sending
			wait_get_tracks(unit);
			
			// Send dummy operation to wake up idle daemon
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
		else{
			disk_list_node* curr = disk_list0;
			if(new_node.track < curr->track){
				// Inserting value that is less than head of list, put in second part of list
				// Iterate to end of front part of list
				while(curr->next!=NULL && curr->track <= curr->next->track){
					curr = curr->next;
				}
				// Now can just insert like normal
				while(curr->next!=NULL && (curr->next->track < new_node.track)){
					curr = curr->next;
				}
				new_node.next = curr->next;
				curr->next = &new_node;
			}
			else{
				// Insert value that isn't greater than head of list
				// Put in order in first part of list
				while(curr->next!=NULL && (curr->next->track < new_node.track) && (curr->track<curr->next->track)){
					curr = curr->next;
				}
				new_node.next = curr->next;
				curr->next = &new_node;
			}
			disk_unlock0();
		}
	}
	else{ // Same as above but for disk 1
		disk_lock1();
		if(disk_list1==NULL){

			
			disk_list1 = &new_node;
			disk_unlock1();
	
			wait_get_tracks(unit);
			
			int num=0;
			USLOSS_DeviceRequest req;
 			req.opr = USLOSS_DISK_TRACKS;
			req.reg1 = (void*)&num;
			USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);
		}
		else{
			disk_list_node* curr = disk_list1;
			if(new_node.track < curr->track){
				while(curr->next!=NULL && curr->track <= curr->next->track){
					curr = curr->next;
				}
				while(curr->next!=NULL && (curr->next->track < new_node.track)){
					curr = curr->next;
				}
				new_node.next = curr->next;
				curr->next = &new_node;
			}
			else{
				while(curr->next!=NULL && (curr->next->track < new_node.track) && (curr->track<curr->next->track)){
					curr = curr->next;
				}
				new_node.next = curr->next;
				curr->next = &new_node;
			}
			disk_unlock1();
		}

	}

	
	// Recv on specified mailbox, so daemon can wake me up at the right time
	void* empty_message = "";
	MboxRecv(my_mailbox_num, empty_message, 0);	
	
	//Operation is complete
	MboxRelease(my_mailbox_num);
	args->arg1 = (void*)(long)new_node.response_status;
	args->arg4 = (void*)(long)0;
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

/**
* Sends the global track_count variable for the specified disk unit
*/
void get_track_count(int unit){

	// Send a request to disk to get track info
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
		
		// Put result from device output into global variable
		track_count_lock0();
		track_count0 = num;
		track_count_unlock0();
		track_list_lock0();
		track_list_node* curr = track_list0;
		
		// Wake up all proccesses waiting for track count
		while(curr!=NULL){
			curr->response = track_count0;
			
			void* empty_message = "";
			MboxSend(curr->mailbox_num, empty_message, 0);
			curr = curr->next;
		}
	
		track_list_unlock0();
	}
	else{ // Same as above but for disk 1
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

void wait_get_tracks(int unit){
	if(unit==0){
		
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

	}
	else{
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
	}
}

int disk_daemon(char* arg){
	int status;
	int unit = (int)(long)arg;
	disk_list_node* curr;
	USLOSS_DeviceRequest req;
	wait_get_tracks(unit);
	while(1){
		if(DEBUG)
			USLOSS_Console("Before waitDevice in Disk %d\n", unit);
		waitDevice(USLOSS_DISK_DEV, unit, &status);
		if(DEBUG)
			USLOSS_Console("After waitDevice in disk %d\n", unit);
		// grab next proc off queue
		if(unit==0){
			disk_lock0();
			curr = disk_list0;
			disk_unlock0();
		}
		else{
			if(DEBUG)
				USLOSS_Console("get curr from unit1\n");
			disk_lock1();
			curr = disk_list1;
			disk_unlock1();
		}
		if(curr!=NULL){
			if(DEBUG)
				USLOSS_Console("curr is not null\n");
		if(status == USLOSS_DEV_ERROR){
			if(DEBUG)
				USLOSS_Console("Status is error\n");
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
			if(DEBUG)
				USLOSS_Console("Status isnt error\n");
			if(curr->started){
				if(DEBUG)
					USLOSS_Console("Continuing an op\n");
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
						disk_list0 = curr->next;
						disk_unlock0();
					}
					else{
						disk_lock1();
						curr = disk_list1;
						disk_list1 = curr->next;
						disk_unlock1();
					}
					// Wake up the process for this operation
					curr->response_status = status;
					void* empty_message = "";
					MboxSend(curr->mailbox_num, empty_message, 0);
				}
				else{
					if(DEBUG)
						USLOSS_Console("Keep continuing\n");
					
					int block = curr->start_block;
					if(block==16){
						if(DEBUG)
							USLOSS_Console("cross to next sector\n");
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
					req.opr = USLOSS_DISK_SEEK;
					req.reg1 = curr->track;
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
