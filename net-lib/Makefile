#
# lib/Makefile	Makefile for the net-lib function collection
#
# NET-LIB	A collection of functions used from the base set of the
#		NET-2 Networking Distribution for the LINUX operating
#		system.  (net-tools, net-drivers)
#
# Author:	Fred N. van Kempen, <waltje@uwalt.nl.mugnet.org>
#		Copyright 1993 MicroWalt Corporation
#
#		This program is free software; you can redistribute it
#		and/or  modify it under  the terms of  the GNU General
#		Public  License as  published  by  the  Free  Software
#		Foundation;  either  version 2 of the License, or  (at
#		your option) any later version.
#

CC	 = $(CROSS_COMPILE)gcc

HWOBJS	 = hw.o loopback.o ether.o ppp.o
AFOBJS	 = unix.o inet.o af.o 
AFGROBJS = inet_gr.o getroute.o 
AFSROBJS = inet_sr.o setroute.o 
ACTOBJS  = ppp_ac.o activate.o
VARIA	 = getargs.o masq_info.o proc.o util.o nstrcmp.o interface.o sockets.o

# Default Name
NET_LIB_NAME = net-tools

ifeq ($(HAVE_IP_TOOLS),1)
VARIA	+= util-ank.o
endif

OBJS	= $(sort $(VARIA) $(AFOBJS) $(HWOBJS) \
		$(AFGROBJS) $(AFSROBJS) $(ACTOBJS))


# This can be overwritten by the TOPLEVEL Makefile
TOPDIR=..
CFLAGS += -I$(TOPDIR) -idirafter $(TOPDIR)/include # -fPIC
SONAME=libnet-tools.so.0

.SUFFIXES: .a .so

all:	lib$(NET_LIB_NAME).a # lib$(NET_LIB_NAME).so

lib$(NET_LIB_NAME).a:	Makefile ../config.h $(OBJS)
			@echo Building $@
			@rm -f $@
			@$(AR) rcs $@ $(OBJS)

.a.so:;
	$(CC) -o $@ -shared -Wl,--whole-archive -Wl,--soname -Wl,$(SONAME) -nostdlib -nostartfiles $<

clean:
		rm -f *.o *~ *.orig lib$(NET_LIB_NAME).a lib$(NET_LIB_NAME).so

clobber:	clean

# End of lib/Makefile.
