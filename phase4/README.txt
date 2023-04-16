test00 - 02: Slight in difference sleep time
test03, 16, 18: Diffence is in pid numbers. This difference is due to us having more 
	service processes, so the pids of the processes created in the testcases are
	higher a result.
test 07, 20, 22: Difference in the order of output for terminal operations, but all
	the correct operations are done. The difference is order is due to the fact
	that the processes have to race for a lock to be able to read/write on the
	terminal
