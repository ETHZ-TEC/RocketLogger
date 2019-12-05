## Makefile for RocketLogger Binaries
##
## Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
## All rights reserved.
## 
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
## 
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
## 
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## 
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

#####  DEFINITIONS  #####

## code revision
PROJECT_NAME = RocketLogger
PROJECT_VERSION = 1.99

## directories
BIN_DIR = bin
LIB_DIR = lib
OBJ_DIR = obj
DEP_DIR = $(OBJ_DIR)/.dep
SRC_DIR = .

## install directories
INSTALL_BIN_DIR = /lib/firmware

## binary names
BINARY_NAME = rocketlogger.bin

## code revision
GIT_DESCRIPTION = `git describe --tags --dirty`
GIT_DATE = `git show -s --format="%ci" HEAD`
COMPILE_DATE = `date --rfc-3339=seconds`

## source files
SRC = $(wildcard  $(SRC_DIR)/*.asm)

## object files
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(addsuffix .obj, $(basename $(SRC)))))

## linker command file definition
LD_CMD = AM335x_PRU.cmd

## target binary
TARGET_BINARY = $(BIN_DIR)/$(BINARY_NAME)


#####  PROGRAMMS AND TOOLS  #####

## program definitions
TOOLCHAIN_PREFIX = /opt/ti/ccs9/ccs/tools/compiler/ti-cgt-pru_2.3.2/bin/
# PASM = pasm
PRUCC = $(TOOLCHAIN_PREFIX)clpru
AS = $(PRUCC)
CC = $(PRUCC)
CXX = $(PRUCC)
LD = $(PRUCC) --run_linker
DISAS = $(TOOLCHAIN_PREFIX)dispru -1 -e
MAKEDIR = mkdir -p
INSTALL = install
REMOVE = rm -f


#####  COMPILER OPTIONS  #####

## language standards
CSTD = -std=gnu99
CXXSTD = -std=c++11

## libraries
LIBS = 

## defines
DEFS = 

## warnings
WARNINGS = -Werror -Wall -Wextra -fmessage-length=0

## optimization (at max -O1 when when checking for memory leaks)
OPT = -O2 # -flto

## debug (at least -g when checking for memory leaks)
DEBUG = #-g3 -ggdb

## misc C flags
COTHER = #
#COTHER += 

## misc C++ flags
CXXOTHER = -ffunction-sections -fdata-sections
CXXOTHER += -finline-functions -finline-functions-called-once

## C compiler flags
CFLAGS = $(WARNINGS) $(GENDEP) $(OPT) $(DEBUG) $(CSTD) $(DEFS) $(COTHER) $(INCS)

## C++ compiler flags
CXXFLAGS = $(WARNINGS) $(GENDEP) $(OPT) $(DEBUG) $(CXXSTD) $(DEFS) $(CXXOTHER) $(INCS)


## Linker options
LDFLAGS = $(LIBS)


#####  TARGETS  #####

## build targets
all:  $(BIN_DIR) $(TARGET_BINARY)

install: all
	$(INSTALL) $(TARGET_BINARY) $(INSTALL_BIN_DIR)

uninstall:
	$(UNINSTALL) $(INSTALL_BIN_DIR)/$(BINARY_NAME)

$(OBJ_DIR)/%.obj: $(SRC_DIR)/%.asm
	@$(MAKEDIR) $(OBJ_DIR)
	$(AS) -v3 --asm_listing --output_file=$@ $<
	$(DISAS) $@ $(@:%.obj=%.lst)

$(TARGET_BINARY): $(LD_CMD) $(OBJ)
	@$(MAKEDIR) $(@D)
	$(LD) --output_file=$@ --map_file=$(@:%.bin=%.map) $^
	$(DISAS) $@ $(@:%.bin=%.lst)

$(SRC_DIR)/version.c: $(filter-out $(OBJ_DIR)/version.obj, $(OBJ))
	echo "generating version.c"
	@echo "#include \"version.h\"" > $@
	@echo "char const *const PROJECT_VERSION = \"$(PROJECT_VERSION)\";" >> $@
	@echo "char const *const GIT_DESCRIPTION = \"$(GIT_DESCRIPTION)\";" >> $@
	@echo "char const *const GIT_DATE = \"$(GIT_DATE)\";" >> $@
	@echo "char const *const COMPILE_DATE = \"$(COMPILE_DATE)\";" >> $@

## directories
$(BIN_DIR):
	$(MAKEDIR) $(BIN_DIR)


## phony targets
.PHONY: clean

clean:
	$(REMOVE) $(SRC_DIR)/*.lst $(OBJ_DIR)/*.obj $(OBJ_DIR)/*.lst $(TARGET_BINARY) $(TARGET_BINARY:.bin=.lst)
