###############################################################################
#
# Copyright (C) 2014 Nikolay Nizov <nizovn@gmail.com>
#
# Portions copyright © 1996-2003 PalmSource, Inc. or its subsidiaries.
# All rights reserved.
#
###############################################################################

CC = m68k-palmos-gcc
CFLAGS = -O2 -Wall

# This Makefile would be a little tidier if we put .rcp files in the same
# directory as the source code.
R = ../Rsc

2048.prc: 2048App tver0001.bin tFRM03e8.bin Talt03e9.bin
	build-prc 2048.prc "2048" 2048 2048App *.bin

2048App: 2048App.o
2048App.o: 2048App.c 2048Rsc.h

tver0001.bin: $(R)/2048App.rcp \
	      2048Rsc.h
	pilrc -I $(R) $(R)/2048App.rcp

tFRM03e8.bin: $(R)/2048Main.rcp 2048Rsc.h
	pilrc -I $(R) $(R)/2048Main.rcp

Talt03e9.bin: $(R)/2048Misc.rcp 2048Rsc.h
	pilrc -I $(R) $(R)/2048Misc.rcp

clean:
	rm -f 2048App *.o *.bin
