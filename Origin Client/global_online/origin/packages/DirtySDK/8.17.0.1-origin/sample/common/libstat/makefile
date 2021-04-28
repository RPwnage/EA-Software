#--------------------------------------------------------------------------
#
#	Description
#		Libstat sample makefile
#
#	Notes
#		This makefile is written to be used with gnumake.  By default, it
#		compiles against the 2.95.3 distribution; this can be overridden
#		by setting the DSOCK_OLDTC variable equal to one, either as an
#       environmental variable or as a command-line option.
#
#	Usage
#		make					; build
#		make clean				; clean build
#
#	Copyright
#		Copyright (c) Tiburon Entertainment / Electronic Arts 2002.
#			ALL RIGHTS RESERVED.
#
#   Version    1.0        08/08/03 (JLB) Default to 2.95.3, add 2.9-ee-991111 option.
#
#--------------------------------------------------------------------------

# make opus make barf and die
ifeq (1,2)
%abort "Opus Make not supported -- use GNUmake"
endif

# setup
TARG =		libstat
BASE =		../../..
SONY = 		/usr/local/sce

# to build with 2.9-33-991111, set the environmental variable DSOCK_OLDTC=1
ifeq ($(DSOCK_OLDTC),1)
 SYSVERS =	2.9-ee-991111
 CCNAME =	gcc
else
 SYSVERS =	2.95.3
 CCNAME =	gcc2953
endif

SONYLIB =	$(SONY)/ee/lib
SONYINC =	-I$(SONY)/ee/include \
			-I$(SONY)/ee/gcc/ee/include \
			-I$(SONY)/ee/gcc/lib/gcc-lib/ee/$(SYSVERS)/include

INCL =		-I$(BASE)/contrib/dirtydnas/include \
            -I$(BASE)/contrib/dirtyelf/include \
            -I$(BASE)/contrib/dirtystrings/include/ps2 \
            -I$(BASE)/contrib/lobbyglue/include \
            -I$(BASE)/contrib/ps2isp/include \
            -I$(BASE)/contrib/voip/include \
			-I$(BASE)/include \
			-I$(BASE)/include/ps2 \
			-I../demolib/include \
		 	-I../

UTIL =		$(BASE)/winutil

# paths to locate crt0.s
VPATH	=  	$(SONYLIB)/ProView \
			$(SONYLIB)

# paths to locate headers and libs
VPATH +=	$(BASE)/include \
			$(BASE)/include/ps2 \
			$(BASE)/contrib/dirtydnas/include \
			$(BASE)/contrib/dirtyelf/include \
			$(BASE)/contrib/dirtystrings/include/ps2 \
			$(BASE)/contrib/lobbyglue/include \
			$(BASE)/contrib/ps2isp/include \
			$(BASE)/contrib/voip/include \
			../demolib/include \
			../ \
			$(BASE)/library/ps2/debug \
			$(BASE)/contrib/dirtydnas/library/ps2/final \
			$(BASE)/contrib/dirtyelf/library/ps2/final \
			$(BASE)/contrib/dirtystrings/library/ps2/final \
			$(BASE)/contrib/lobbyglue/library/ps2/final \
			$(BASE)/contrib/ps2isp/library/ps2/final \
			$(BASE)/contrib/voip/library/ps2/final \
			../demolib/library/debug

# c compiler
EE_CC = 	ee-$(CCNAME)
EE_CF = 	-g2 -O2 -Wall -Werror -Wno-multichar -DCODE_DEBUG=1 -DDIRTYCODE_PLATFORM=5 $(SONYINC) $(INCL)

# linker 
EE_LL = 	$(SONYLIB)/libdev.a \
			$(SONYLIB)/libgraph.a \
	    	$(SONYLIB)/libdma.a \
	    	$(SONYLIB)/libcdvd.a \
	    	$(BASE)/contrib/dirtydnas/library/ps2/final/libdirtydnas.a \
	    	$(BASE)/contrib/dirtyelf/library/ps2/final/libdirtyelf.a \
	    	$(BASE)/contrib/dirtystrings/library/ps2/final/libdirtystrings.a \
	    	$(BASE)/contrib/lobbyglue/library/ps2/final/liblobbyglue.a \
	    	$(BASE)/contrib/ps2isp/library/ps2/final/libps2isp.a \
	    	$(BASE)/contrib/voip/library/ps2/final/libvoip.a \
			$(BASE)/library/ps2/final/libdirtysock.a \
			$(SONYLIB)/libscf.a \
			-Wl -nostartfiles -L$(SONYLIB) -lm
EE_LC =		-T libstat.cmd

# asm
EE_AS = 	ee-$(CCNAME)
EE_AF = 	-c -xassembler-with-cpp -Wa,-al

OBJS =		libstat.o crt0.o


# default
%.o: %.c
	@echo Compiling file $<
	@$(EE_CC) $(EE_CF) -c $<

%.o: %.s
	@echo Assembling file $<
	@$(EE_AS) $(EE_AF) -o $@ $< > $*.lst

# rules
all:	$(TARG).elf

clean:
	@echo Cleaning files
	@$(UTIL)/dsrm -f *.o *.elf *.map *.lst

strip:
	@echo Stripping $(TARG).elf
	@ee-strip $(TARG).elf

$(TARG).elf: $(OBJS)
	@echo Building $(TARG).elf
	@$(EE_CC) $(EE_LC) $(OBJS) $(EE_LL) -Xlinker -Map -Xlinker $(TARG).map -o $@

# dependencies
libstat.o: dirtydefs.h dirtysock.h rpc.h \
	lobbydisplist.h lobbyhasher.h lobbytagfield.h lobbyapi.h \
	dsdemo.h dsdemocons.h dsdemograph.h dsdemokey.h dsdemonet.h dsdemowin.h \
	democfg.h \
	libdirtysock.a libdirtydemo.a \
	makefile

