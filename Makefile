VERSION			=	0.0.1

CC				=	$(CROSS_COMPILE)gcc

MAIN_INCLUDES	=	-I. -Iinclude -Inet-lib \
					-I$(CROSS_COMPILE_ROOT)/usr/include/glib-2.0 \
					-I$(CROSS_COMPILE_ROOT)/usr/lib/glib-2.0/include \
					-I$(CROSS_COMPILE_ROOT)/usr/include/lunaservice \
					-I$(CROSS_COMPILE_ROOT)/usr/include/mjson \
					-Ilibircclient/include
					
ifeq ($(CS_TOOLCHAIN_ROOT),)				
	INCLUDES	=	$(MAIN_INCLUDES)
else
	INCLUDES	=	-L$(CS_TOOLCHAIN_ROOT)/arm-none-linux-gnueabi/libc/usr/lib \
					-Xlinker -rpath-link=$(CROSS_COMPILE_ROOT)/usr/lib \
					-L$(CROSS_COMPILE_ROOT)/usr/lib \
					$(MAIN_INCLUDES)
endif

ifeq ($(DEVICE),pre)
	MARCH_TUNE	=	-march=armv7-a -mtune=cortex-a8
	SUFFIX		=	-armv7
else
ifeq ($(DEVICE),pixi)
	MARCH_TUNE	=	-march=armv6j -mtune=arm1136j-s
	SUFFIX		=	-armv6
else
ifeq ($(DEVICE),emu)
	SUFFIX		=	-i686
endif
endif
endif

CFLAGS			=	-Os -g $(MARCH_TUNE) -DVERSION=\"$(VERSION)\"
					
LIBS			= 	-lglib-2.0 -llunaservice

PROGRAM_BASE	=	us.ryanhope.freetetherd

PROGRAM			= 	$(PROGRAM_BASE)$(SUFFIX)

OBJECTS			= 	main.o

.PHONY			: 	clean-objects clean


all: $(PROGRAM)

fresh: clean all

$(PROGRAM): libnet-tools.a $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) net-lib/libnet-tools.a -o $(PROGRAM) $(INCLUDES) $(LIBS)
	
libnet-tools.a:
	@$(MAKE) -C net-lib/

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $<  -o $@ -I. $(INCLUDES) $(LIBS)
	
subdirs:
	
clean-objects:
	rm -rf $(OBJECTS)
	
clean-net-tools:
	@$(MAKE) -C net-lib/ clean
	
clean: clean-objects clean-net-tools
	rm -rf $(PROGRAM_BASE)*;
