PREFIX = ../..

CC = gcc

CSRCS = $(wildcard *.c)
COBJS = $(CSRCS:.c=.o)

LIBS = -lusloss4.7 -lphase1 -lphase2 -lphase3

LIB_DIR     = ${PREFIX}/lib
INCLUDE_DIR = ${PREFIX}/include

CFLAGS = -Wall -g -I${INCLUDE_DIR} -I.
LDFLAGS = -Wl,--start-group -L${LIB_DIR} -L. ${LIBS} -Wl,--end-group



VPATH = testcases
TESTS = test00 test01 test02 test03 test04 test05 test06 test07 test08 test09 \
        test10 test11 test12 test13 test14 test15 test16 test17 test18 test19 \
        test20 test21 test22 test23 test24



all: ${TESTS}

${TESTS}: phase4_common_testcase_code.o $(COBJS) libphase1.a libphase2.a libphase3.a

ARCH=$(shell uname | tr '[:upper:]' '[:lower:]')-$(shell uname -p | sed -e "s/aarch/arm/g")

phase4_no_debug_symbols-${ARCH}.o: phase4.c
	gcc -I${INCLUDE_DIR} -I. -c phase4.c -o phase4_no_debug_symbols-${ARCH}.o

phase4_clock_no_debug_symbols-${ARCH}.o: phase4_clock.c
	gcc -I${INCLUDE_DIR} -I. -c phase4_clock.c -o phase4_clock_no_debug_symbols-${ARCH}.o

phase4_term_no_debug_symbols-${ARCH}.o: phase4_term.c
	gcc -I${INCLUDE_DIR} -I. -c phase4_term.c -o phase4_term_no_debug_symbols-${ARCH}.o

phase4_disk_no_debug_symbols-${ARCH}.o: phase4_disk.c
	gcc -I${INCLUDE_DIR} -I. -c phase4_disk.c -o phase4_disk_no_debug_symbols-${ARCH}.o

phase4_usermode_no_debug_symbols-${ARCH}.o: phase4_usermode.c
	gcc -I${INCLUDE_DIR} -I. -c phase4_usermode.c -o phase4_usermode_no_debug_symbols-${ARCH}.o

libphase4-${ARCH}.a: phase4_no_debug_symbols-${ARCH}.o phase4_clock_no_debug_symbols-${ARCH}.o phase4_term_no_debug_symbols-${ARCH}.o phase4_disk_no_debug_symbols-${ARCH}.o phase4_usermode_no_debug_symbols-${ARCH}.o
	-rm $@
	ar -r $@ $^

clean:
	-rm *.o ${TESTS} term[0-3].out

