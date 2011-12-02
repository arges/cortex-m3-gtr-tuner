# This make file is based off the TI Example

#******************************************************************************
#
# Copyright (c) 2006-2011 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 6852 of the EK-LM3S8962 Firmware Package.
#
#******************************************************************************

NAME=gtr_tuner

BOARD=ek-lm3s8962
PART=LM3S8962
ROOT=/home/arges/Source/StellarisWare

VERBOSE=

include ${ROOT}/makedefs
VPATH=${ROOT}/boards/${BOARD}/drivers
VPATH+=${ROOT}/utils
IPATH=${ROOT}/boards/${BOARD}/
IPATH+=${ROOT}

all: ${COMPILER}
all: ${COMPILER}/gtr_tuner.axf

clean:
	@rm -rf ${COMPILER} ${wildcard *~}

${COMPILER}:
	@mkdir ${COMPILER}

#
# Rules for building the program.
#
${COMPILER}/gtr_tuner.axf: ${COMPILER}/graphics.o ${COMPILER}/queue.o
${COMPILER}/gtr_tuner.axf: ${COMPILER}/gtr_tuner.o ${COMPILER}/rit128x96x4.o
${COMPILER}/gtr_tuner.axf: ${COMPILER}/startup_${COMPILER}.o ${COMPILER}/ustdlib.o
${COMPILER}/gtr_tuner.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/gtr_tuner.axf: ${COMPILER}/Eg_FFT128Real_32b.o
${COMPILER}/gtr_tuner.axf: ${COMPILER}/Eg_Magnitude.o
${COMPILER}/gtr_tuner.axf: ${COMPILER}/Eg_Window16to32b_real.o

SCATTERgcc_gtr_tuner=gtr_tuner.ld
ENTRY_gtr_tuner=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif

