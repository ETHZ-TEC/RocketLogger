## Makefile for ambient sensor readout test
##
## Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
##

#####  DEFINITIONS  #####

## directories
BINDIR = bin
LIBDIR = lib
OBJDIR = obj
DEPDIR = $(OBJDIR)/.dep
SRCDIR = .

## target binary
TARGET = ambient_sensor

## source files
SRC = main.c
SRC += sensor/tsl256x.c

## object files
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(addsuffix .o, $(basename $(SRC)))))


#####  PROGRAMMS AND TOOLS  #####

## program definitions
TOOLCHAIN_PREFIX = #
CC = $(TOOLCHAIN_PREFIX)clang
CXX = $(TOOLCHAIN_PREFIX)clang++
MAKEDIR = mkdir -p
COPY = cp -rf
REMOVE = rm -f


#####  COMPILER OPTIONS  #####

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
GENDEP = -MMD -MP -MF $(DEPDIR)/$(@F).d

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


#####  TARGETS  #####

## build targets
all: $(TARGET)

$(OBJDIR)/%.o: %.c $(OBJDIR) $(DEPDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp $(OBJDIR) $(DEPDIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: sensor/%.c $(OBJDIR) $(DEPDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^


## intermediate directories
$(OBJDIR):
	$(MAKEDIR) $(OBJDIR)

$(DEPDIR):
	$(MAKEDIR) $(DEPDIR)
	

## phony targets
.PHONY: clean info

clean:
	$(REMOVE) $(OBJDIR)/*.o $(DEPDIR)/*.d $(TARGET)

