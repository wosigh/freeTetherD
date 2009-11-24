CC				=	$(CROSS_COMPILE)gcc

MAIN_INCLUDES	=	-I.
					
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
else
ifeq ($(DEVICE),pixi)
	MARCH_TUNE	=	-march=armv6j -mtune=arm1136j-s
else
ifeq ($(DEVICE),emu)
endif
endif
endif

CFLAGS			=	-Os -g $(MARCH_TUNE)

PROGRAM			= 	libip.so

OBJECTS			= 	ipaddress.o ipaddrlabel.o iproute.o iprule.o rtm_map.o \
					iptunnel.o ip6tunnel.o tunnel.o ipneigh.o ipntable.o \
					iplink.o ipmaddr.o ipmonitor.o ipmroute.o ipprefix.o \
    				ipxfrm.o xfrm_state.o xfrm_policy.o xfrm_monitor.o \
    				iplink_vlan.o link_veth.o link_gre.o

.PHONY			: 	clean-objects clean


all: $(PROGRAM)

fresh: clean all

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) -shared $(OBJECTS) -o $(PROGRAM) $(INCLUDES)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $<  -o $@ -I. $(INCLUDES)
	
clean-objects:
	rm -rf $(OBJECTS)
	
clean: clean-objects
	rm -rf $(PROGRAM)
