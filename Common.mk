###############################################################################
#
# Common.mk
#
# author: Henryk Richter <henryk.richter@gmx.net>
#
# note: when switching between different hc99ardware targets, don`t forget
#       "make clean" in between
#
# tools required:
#  GNU Make, either GCC (m68k-amigaos-gcc, default) or VBCC for AmigaOS/68k
#  recent sana2.h , e.g. from RoadShow SDK
#
# porting:
#  you might need to adapt the paths (prefix for Compiler/Includes)
#  Some installations have ADE: instead of GG: on Amigaos
#  Recent work on AmigaOS cross toolchains suggest /opt/m68k-amigaos/sys-include
#  instead of the traditional os-include
#
###############################################################################
# debug = 1 will include string debugging for terminal/sushi/sashimi
debug = 0
# compiler_vcc = 1 will trigger VBCC, else GCC (default)
compiler_vcc ?= 0

###############################################################################
# prefix for system includes (ASM)
# native AmigaOS compilation: set PREFX=GG: or PREFX=ADE:, depending on toolchain
###############################################################################
ifeq ($(compiler_vcc),1)
PREFX  = $(VBCC)/targets/m68k-amigaos
#PREFX = gg:
SYSINC ?= "-I$(PREFX)/include" 
SYSLIB ?= "-L$(PREFX)/lib"
endif

###############################################################################
#
# compiler executables (choice of gcc or vbcc)
#
###############################################################################
ifeq ($(compiler_vcc),1)

# VBCC (use explicit vlink line for LINK= if complaints about R_PC happen)
CCX  = vc +aos68k $(SYSINC)
LINK = vlink -bamigahunk -x -s -mrel -Cvbcc -Bstatic -nostdlib -lamiga $(SYSLIB) #-Rshort
#LINKEXE = vlink -bamigahunk -x -s -mrel -Cvbcc -Bstatic
LINKEXE = vc +aos68k
#LINK = $(CCX) -nostdlib
TOOLLIBS = -lamiga
CFLAGS  = -O2 -+ -sc -cpu=$(CPU)
CFLAGS2 = -Os -+ -sc -c99 -cpu=$(CPU2)
ifeq ($(debug),1)
CFLAGS  = 
CFLAGS2 = 
endif


else

# GCC (tested with m68k-amigaos-gcc 16.2)
# No startup code: the device is entered through its ROMTag. libnix only
# supplies freestanding string helpers, libgcc the 68000 mul/div helpers.
# Library bases live in the device base, so no global SysBase is needed.
CCX  = m68k-amigaos-gcc
LINK = $(CCX) -m$(CPU) -noixemul -nostartfiles -s
LINKEXE = $(CCX) -s -noixemul
# Shell tools use the normal libnix startup code
TOOLLDFLAGS = -s
CFLAGS  = -O2 -m$(CPU) -noixemul -fomit-frame-pointer -Wall
CFLAGS2 = -O2 -m$(CPU2) -noixemul -fomit-frame-pointer -Wall
ifeq ($(debug),1)
CFLAGS  = -O0 -m$(CPU) -noixemul -Wall
CFLAGS2 = -O0 -m$(CPU2) -noixemul -Wall
endif

endif

VASM = vasmm68k_mot
VASMFORMAT  = -m$(CPU) -Fhunk -nowarn=2064 -quiet $(SYSINC)
VASMFORMAT2 = -m$(CPU2) -Fhunk -nowarn=2064 -quiet $(SYSINC)

# unused here
#HOST = $(shell uname)

###############################################################################
#
# paths to the local includes
#
###############################################################################

# GCC ships its own NDK headers; VBCC needs them pointed out
ifeq ($(compiler_vcc),1)
IPATH ?= -I/opt/amiga/NDK3.2/Include_H
endif

###############################################################################
#
# compile-level feature definitions
#
###############################################################################
ifeq ($(compiler_vcc),1)
# skip quotes with VCC, the AmigaOS native version doesn't like them
DEFINES += -DDEVICENAME="$(DEVICEID)"
DEFINES += -DHAVE_VERSION_H=1

DEFINES2 += -DDEVICENAME="$(DEVICEID2)"
DEFINES2 += -DHAVE_VERSION_H=1

else

DEFINES += -D"DEVICENAME="$(DEVICEID)""
DEFINES += -DHAVE_VERSION_H=1
DEFINES += -DNEWSTYLE

DEFINES2 += -D"DEVICENAME="$(DEVICEID2)""
DEFINES2 += -DHAVE_VERSION_H=1
DEFINES2 += -DNEWSTYLE
#ASMDEFS2 += -DDEVICEVERSION=$(DEVICEVERSION) -DDEVICEREVISION=$(DEVICEREVISION)

endif


###############################################################################
#
# debug
#
###############################################################################

ifeq ($(debug),1)
CFLAGS  += -DDEBUG -g
CFLAGS2 += -DDEBUG -g
LINKLIBS = -L$(PREFX)/lib -ldebug
endif

###############################################################################
#
# compiler flags and optimization levels
#
###############################################################################

CFLAGS  += -I. $(if $(SUBDIR),-I$(SUBDIR))
CFLAGS2 += -I. $(if $(SUBDIR),-I$(SUBDIR))
LDFLAGS =

###############################################################################
#
# objects to build
#
###############################################################################
# ASM based alternative to deviceheader.o would be romtag.o

OBJECTS = deviceheader.o deviceinit.o device.o scsiwifi.o
ifneq ($(compiler_vcc),1)
OBJECTS += gccsupport.o
endif
OBJECTS += $(ASMOBJECTS)

# used for secondary build
OBJECTS2 = $(patsubst %.o,%.2o,$(OBJECTS))

###############################################################################
#
# rules and commands
#
###############################################################################

all:	$(DEVICEID) $(DEVICEID2) $(TESTTOOL) $(TESTTOOL2) $(TOOLS)

clean:
	rm -f $(OBJECTS) $(OBJECTS2)
	rm -f $(DEVICEID) $(DEVICEID2) $(TOOLS) $(EXTRACLEAN)

# not for cross compile :-)
install: $(DEVICEID) $(DEVICEID2)
	copy $(DEVICEID) $(DEVICEID2) DEVS:


#sdnet:	 $(DEVICEID)
#expnet: $(DEVICEID2)

$(DEVICEID) : $(OBJECTS)
	$(LINK) $(LDFLAGS) -o $@ $(OBJECTS) $(LINKLIBS) $(LINKOPTS)


# separate ruleset for each subdirectory, ./src overrides all other paths for priority
# of platform-optimized routines

%.o : %.c
	$(CCX) -c $(CFLAGS) $(DEFINES) $(IPATH) -o $@ $<

# Shell tool sharing the driver's SCSI code
daynawifi: daynawifi.c scsiwifi.c scsiwifi.h
	$(CCX) $(CFLAGS) $(IPATH) $(TOOLLDFLAGS) -o $@ daynawifi.c scsiwifi.c $(TOOLLIBS)


# secondary ruleset (used for expnet, will be ignored if DEVICEID2 is empty)
$(DEVICEID2) : $(OBJECTS2)
	$(LINK) $(LDFLAGS) -o $@ $(OBJECTS2) $(LINKLIBS)

%.2o : %.c
	$(CCX) -c $(CFLAGS2) $(DEFINES2) $(IPATH) $< -o $@

