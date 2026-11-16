###############################################################################
#
# makefile for gcc (default) or vbcc
#
# original author: Henryk Richter <henryk.richter@gmx.net>
# 
# concept:
#
# tools required:
#  - m68k-amigaos-gcc (tested with 16.2), or
#  - vbcc + vlink: make compiler_vcc=1
#
# porting:
#
#  see Common.mk
#
###############################################################################

###############################################################################
# Date, version, extra objects to build
# 
###############################################################################
# version.h is the only place the version is set
DEVICEVERSION  := $(shell sed -n 's/^\#define DEVICEVERSION *//p' version.h)
DEVICEREVISION := $(shell sed -n 's/^\#define DEVICEREVISION *//p' version.h)
DEVICEDATE     := $(shell sed -n 's/^\#define DEVICEDATE *//p' version.h)

###############################################################################
# Devices to build (1 or 2, keep DEVICEID2 empty if only one build is desired)
#
###############################################################################

DEVICEID=scsidayna.device
DEFINES = #
ASMDEFS = #
CPU = 68000

DEVICEID2=

###############################################################################
# Shell tools, built with the same compiler (see Common.mk)
#
###############################################################################
TOOLS = daynawifi

###############################################################################
# import generic ruleset
# 
###############################################################################
include Common.mk
