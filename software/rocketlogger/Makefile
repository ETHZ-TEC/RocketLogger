## Makefile for RocketLogger Binaries
##
## Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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
SENSOR_DIR = $(SRC_DIR)/sensor

## install directories
INSTALL_BIN_DIR = /bin
INSTALL_FIRMWARE_DIR = /lib/firmware
INSTALL_SERVICE_DIR = /etc/systemd/system

## binary names
BINARY_NAME = rocketlogger
DEAMON_NAME = rocketloggerd
SERVER_NAME = rocketloggers
FIRMWARE_NAME = rocketlogger.bin

## config file names
SERVICE_CONFIG = rocketlogger.service

## code revision
GIT_DESCRIPTION = `git describe --tags --dirty`
GIT_DATE = `git show -s --format="%ci" HEAD`
COMPILE_DATE = `date --rfc-3339=seconds`

## source files
SRC = $(wildcard  $(SRC_DIR)/*.c)
SRC += $(wildcard  $(SRC_DIR)/*.cpp)
SRC += $(wildcard  $(SENSOR_DIR)/*.c)
SRC += $(SRC_DIR)/version.c
FIRMWARE_SRC = rocketlogger.p

## object files
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(addsuffix .o, $(basename $(SRC)))))

## target binary
TARGET_BINARY = $(BIN_DIR)/$(BINARY_NAME)
TARGET_DAEMON = $(BIN_DIR)/$(DEAMON_NAME)
TARGET_SERVER = $(BIN_DIR)/$(SERVER_NAME)
TARGET_FIRMWARE = $(BIN_DIR)/$(FIRMWARE_NAME)


#####  PROGRAMMS AND TOOLS  #####

## program definitions
TOOLCHAIN_PREFIX = #
PASM = pasm
CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
MAKEDIR = mkdir -p
COPY = cp -f
REMOVE = rm -f


#####  COMPILER OPTIONS  #####

## language standards
CSTD = -std=gnu99
CXXSTD = -std=c++11

## libraries
LIBS = -lprussdrv
LIBS += -li2c
LIBS += -lncurses
LIBS += -lrt

## defines
DEFS = -D_LARGEFILE64_SOURCE

## warnings
WARNINGS = -Werror -Wall -Wextra -fmessage-length=0

## optimization (at max -O1 when when checking for memory leaks)
OPT = -O2 # -flto

## debug (at least -g when checking for memory leaks)
DEBUG = #-g3 -ggdb

## dependency file configurations
GENDEP = -MMD -MP -MF $(DEP_DIR)/$(@F).d

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
all:  $(BIN_DIR) $(TARGET_BINARY) $(TARGET_DAEMON) $(TARGET_SERVER) $(TARGET_FIRMWARE)

install: all
	$(COPY) $(TARGET_BINARY) $(INSTALL_BIN_DIR)
	$(COPY) $(TARGET_DAEMON) $(INSTALL_BIN_DIR)
	$(COPY) $(TARGET_SERVER) $(INSTALL_BIN_DIR)
	$(COPY) $(TARGET_FIRMWARE) $(INSTALL_FIRMWARE_DIR)
	chmod 4755 $(INSTALL_BIN_DIR)/$(BINARY_NAME) $(INSTALL_BIN_DIR)/$(DEAMON_NAME) $(INSTALL_BIN_DIR)/$(SERVER_NAME)
	$(COPY) $(SERVICE_CONFIG) $(INSTALL_SERVICE_DIR)
	systemctl enable rocketlogger
	systemctl restart rocketlogger

uninstall:
	$(REMOVE) $(INSTALL_BIN_DIR)/$(BINARY_NAME)
	$(REMOVE) $(INSTALL_BIN_DIR)/$(DEAMON_NAME)
	$(REMOVE) $(INSTALL_FIRMWARE_DIR)/$(FIRMWARE_NAME)
	systemctl disable rocketlogger
	$(REMOVE) $(INSTALL_SERVICE_DIR)/$(SERVICE_CONFIG)

$(TARGET_BINARY): $(filter-out $(OBJ_DIR)/rocketloggerd.o $(OBJ_DIR)/rl_server.o, $(OBJ))
	@$(MAKEDIR) $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^

$(TARGET_DAEMON): $(filter-out $(OBJ_DIR)/rocketlogger.o $(OBJ_DIR)/rl_server.o, $(OBJ))
	@$(MAKEDIR) $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^

$(TARGET_SERVER): $(filter-out $(OBJ_DIR)/rocketlogger.o $(OBJ_DIR)/rocketloggerd.o, $(OBJ))
	@$(MAKEDIR) $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^

$(TARGET_FIRMWARE): $(FIRMWARE_SRC)
	@$(MAKEDIR) $(@D)
	$(PASM) -b $< $(@:.bin=)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MAKEDIR) $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MAKEDIR) $(OBJ_DIR) $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SENSOR_DIR)/%.c
	@$(MAKEDIR) $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SENSOR_DIR)/%.cpp
	@$(MAKEDIR) $(OBJ_DIR) $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(SRC_DIR)/version.c: $(filter-out $(OBJ_DIR)/version.o, $(OBJ))
	echo "generating version.c"
	@echo "#include \"version.h\"" > $@
	@echo "char const *const PROJECT_VERSION = \"$(PROJECT_VERSION)\";" >> $@
	@echo "char const *const GIT_DESCRIPTION = \"$(GIT_DESCRIPTION)\";" >> $@
	@echo "char const *const GIT_DATE = \"$(GIT_DATE)\";" >> $@
	@echo "char const *const COMPILE_DATE = \"$(COMPILE_DATE)\";" >> $@

## directories
$(OBJ_DIR):
	$(MAKEDIR) $(OBJ_DIR)

$(DEP_DIR):
	$(MAKEDIR) $(DEP_DIR)

$(BIN_DIR):
	$(MAKEDIR) $(BIN_DIR)


## phony targets
.PHONY: clean code-format

clean:
	$(REMOVE) $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(TARGET_BINARY) $(TARGET_DAEMON) $(TARGET_FIRMWARE) $(TARGET_SERVER)

code-format:
	find . -type f \( -iname "*.c" -o -iname "*.h" -o -iname "*.cpp" \) -exec clang-format -style=file -i {} \;


#####  INCLUDE DEPENDENCIES  #####
-include $(wildcard  $(DEP_DIR)/*.d)
