## Makefile for device tree overlay compilation

# files
TARGET = ROCKETLOGGER-00A0.dtbo
SRC = ROCKETLOGGER-v1.1.dts
TMP = $(SRC).tmp
INC_DIRS = /usr/src/linux-headers-`uname -r`/include

# install
INSTALL_DIR = /lib/firmware

# tools
CPP = cpp  # cpp; gcc -E; or clang -E
DTC = dtc
RM = rm -rf
CP = cp -f


# make rules
$(TARGET): $(SRC)
	$(CPP) -nostdinc -undef -I $(INC_DIRS) -x assembler-with-cpp -o $(TMP) $^
	$(DTC) --out-format dtb --out $@ $(TMP)
	$(RM) $(TMP)

all: $(TARGET)

install: all
	$(CP) $(TARGET) $(INSTALL_DIR)
	@echo "Binary installed, configure cape manager manually to load overlay at boot."

.PHONY: clean uninstall
clean:
	$(RM) $(TMP) $(TARGET)

uninstall:
	$(RM) $(INSTALL_DIR)/$(TARGET)

