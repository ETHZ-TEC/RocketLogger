## Makefile for ambient sensor readout test

#################   DEFINITIONS    #################

## Directories and files

# directories
BINDIR = bin
LIBDIR = lib
OBJDIR = obj
SRCDIR = .

# target binary

TARGET = ambient_sensor

# source files
SRC = main.c
#SRC += #


## Programs and Tools

# program definitions
TOOLCHAIN_PREFIX = #
CC = $(TOOLCHAIN_PREFIX)clang
CXX = $(TOOLCHAIN_PREFIX)clang++
MAKEDIR = mkdir -p
COPY = cp -rf
REMOVE = rm -f


################# COMPILER OPTIONS  #################

## language standards
CSTD = -std=gnu99
CXXSTD = -std=c++11

## warnings
WARNINGS = -Wall -Wextra -fmessage-length=0

## optimization (at max -O1 when when checking for memory leaks)
OPT = -O2 # -flto

## debug (at least -g when checking for memory leaks)
DEBUG = #-g3 -ggdb

## dependency file configurations
#GENDEP = -MMD -MP -MF $(OBJDIR)/dep/$(@F).d

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

LDFLAGS = # -rdynamic


.PHONY: clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(REMOVE) *.o ambient_sensor

