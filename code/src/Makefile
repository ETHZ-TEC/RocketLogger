##
## Makefile for RocketLogger
##

#####  DEFINITIONS  #####

## directories
BIN_DIR = bin
LIB_DIR = lib
OBJ_DIR = obj
DEP_DIR = $(OBJ_DIR)/.dep
SRC_DIR = .

## install directories
INSTALL_BIN_DIR = /bin
INSTALL_FIRMWARE_DIR = /lib/firmware

## binary names
BINARY_NAME = rocketlogger
DEAMON_NAME = rocketloggerd
FIRMWARE_NAME = rocketlogger_spi.bin

## source files
SRC = $(wildcard  $(SRC_DIR)/*.c)
SRC += $(wildcard  $(SRC_DIR)/*.cpp)
FIRMWARE_SRC = SPI.p

## object files
OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(addsuffix .o, $(basename $(SRC)))))

## target binary
TARGET_BINARY = $(BIN_DIR)/$(BINARY_NAME)
TARGET_DEAMON = $(BIN_DIR)/$(DEAMON_NAME)
TARGET_FIRMWARE = $(BIN_DIR)/$(FIRMWARE_NAME)


#####  PROGRAMMS AND TOOLS  #####

## program definitions
TOOLCHAIN_PREFIX = #
PASM = pasm
CC = $(TOOLCHAIN_PREFIX)clang
CXX = $(TOOLCHAIN_PREFIX)clang++
MAKEDIR = mkdir -p
COPY = cp -f
REMOVE = rm -f


#####  COMPILER OPTIONS  #####

## language standards
CSTD = -std=gnu99
CXXSTD = -std=c++11

## libraries
LIBS = -pthread
LIBS += -lprussdrv
LIBS += -lncurses
LIBS += -lrt

## warnings
WARNINGS = -Wall -Wextra -fmessage-length=0

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
all:  $(BIN_DIR) $(TARGET_BINARY) $(TARGET_DEAMON) $(TARGET_FIRMWARE)

install: all
	$(COPY) $(TARGET_BINARY) $(INSTALL_BIN_DIR)
	$(COPY) $(TARGET_DEAMON) $(INSTALL_BIN_DIR)
	$(COPY) $(TARGET_FIRMWARE) $(INSTALL_FIRMWARE_DIR)
	chmod 4755 $(INSTALL_BIN_DIR)/$(BINARY_NAME) $(INSTALL_BIN_DIR)/$(DEAMON_NAME)
	echo "#!/bin/bash" > /etc/init.d/rocketlogger
	echo "$(INSTALL_BIN_DIR)/$(DEAMON_NAME) &" >> /etc/init.d/rocketlogger
	echo "echo 31 > /sys/class/gpio/export" >> /etc/init.d/rocketlogger
	echo "echo out > /sys/class/gpio/gpio31/direction" >> /etc/init.d/rocketlogger
	echo "echo 1 > /sys/class/gpio/gpio31/value" >> /etc/init.d/rocketlogger
	chmod +x /etc/init.d/rocketlogger
	update-rc.d rocketlogger defaults
	echo "options uio_pruss extram_pool_sz=0x300000" > /etc/modprobe.d/rl_pru.conf

uninstall:
	$(REMOVE) $(INSTALL_BIN_DIR)/$(BINARY_NAME)
	$(REMOVE) $(INSTALL_BIN_DIR)/$(DEAMON_NAME)
	$(REMOVE) $(INSTALL_FIRMWARE_DIR)/$(FIRMWARE_NAME)
	$(REMOVE) /etc/init.d/rocketlogger
	update-rc.d rocketlogger remove
	$(REMOVE) /etc/modprobe.d/rl_pru.conf

$(TARGET_BINARY): $(filter-out $(OBJ_DIR)/rl_deamon.o, $(OBJ))
	@$(MAKEDIR) $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^
	
$(TARGET_DEAMON): $(filter-out $(OBJ_DIR)/rocketlogger.o, $(OBJ))
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


## directories
$(OBJ_DIR):
	$(MAKEDIR) $(OBJ_DIR)

$(DEP_DIR):
	$(MAKEDIR) $(DEP_DIR)

$(BIN_DIR):
	$(MAKEDIR) $(BIN_DIR)


## phony targets
.PHONY: clean

clean:
	$(REMOVE) $(OBJ_DIR)/*.o $(DEP_DIR)/*.d


#####  INCLUDE DEPENDENCIES  #####
-include $(wildcard  $(DEP_DIR)/*.d)
