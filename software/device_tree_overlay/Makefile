## Makefile for device tree overlay

# files
TARGET = ROCKETLOGGER-00A0.dtbo
SRC = ROCKETLOGGER.dts
TMP = $(SRC).tmp
# INC_DIRS = /usr/src/linux-headers-`uname -r`/include

# install
INSTALL_DIR = /lib/firmware

# tools
CPP = cpp  # cpp; gcc -E; or clang -E
DTC = dtc
RM = rm -rf
CP = cp -f


# make rules
$(TARGET): $(SRC)
	$(CPP) -nostdinc -undef -x assembler-with-cpp -o $(TMP) $^
	$(DTC) --out-format dtb --out $@ -@ $(TMP)
	$(RM) $(TMP)

all: $(TARGET)

install: all
	$(CP) $(TARGET) $(INSTALL_DIR)
	$(CP) capemgr /etc/default/capemgr
	@echo "Binary and cape manager config installed, overlay will be loaded automativally after reboot."

.PHONY: clean uninstall
clean:
	$(RM) $(TMP) $(TARGET)

uninstall:
	$(RM) $(INSTALL_DIR)/$(TARGET)
	$(RM) /etc/default/capemgr

