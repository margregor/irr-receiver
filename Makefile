GOWIN_IDE   := C:\misc\gowin\edu\Gowin_V1.9.11.03_Education_x64\IDE
GOWIN_PROG  := C:\misc\gowin\edu\Gowin_V1.9.11.03_Education_x64\Programmer\bin\programmer_cli.exe
GW_SH       := $(GOWIN_IDE)/bin/gw_sh
BITSTREAM   := build/irreceiver/impl/pnr/irreceiver.fs
DEVICE      := GW2AR-18C

SOURCES := src/top.sdc src/constraints/top.cst src/top.sv

.PHONY: all timing flash-sram flash scan

all: $(BITSTREAM) timing

$(BITSTREAM): $(SOURCES)
	$(GW_SH) scripts/build.tcl

timing: $(BITSTREAM)
	python scripts/check_timing.py build/irreceiver/impl/pnr/irreceiver_tr_content.html 27

# Program SRAM only (volatile, fast — lost on power cycle)
flash-sram: $(BITSTREAM)
	$(GOWIN_PROG) --device $(DEVICE) --operation_index 2 --fsFile $(abspath $(BITSTREAM))

# Program external QSPI flash (persistent)
flash: $(BITSTREAM)
	$(GOWIN_PROG) --device $(DEVICE) --operation_index 36 \
		--spiaddr 0x000000 --fsFile $(abspath $(BITSTREAM))

scan:
	$(GOWIN_PROG) --scan
