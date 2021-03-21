MCU=attiny85
F_CPU=1000000
FUSE_L=0x42
FUSE_H=0xDD
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I.
TARGET=dices
SRCS=dices.c

help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ....... to build main.hex"
	@echo "make fuse ...... to flash the fuses"
	@echo "make flash ..... to flash the firmware (use this on metaboard)"
	@echo "make clean ..... to delete objects and hex file"

hex:
		${CC} ${CFLAGS} -o ${TARGET}.bin ${SRCS}
		${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.bin ${TARGET}.hex

fuse:
		avrdude -p ${MCU} -c avrisp2 -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m -P usb

flash:
		avrdude -p ${MCU} -c avrisp2 -U flash:w:${TARGET}.hex:i -P usb

clean:
		rm -f *.bin *.hex
