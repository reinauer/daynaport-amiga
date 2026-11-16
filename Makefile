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
DEVICEVERSION=1
DEVICEREVISION=5
DEVICEDATE=2026-03-09

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
# import generic ruleset
# 
###############################################################################
include Common.mk



