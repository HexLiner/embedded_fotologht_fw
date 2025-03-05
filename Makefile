PRG            = hot_fen_fw
OBJ            = main.o systimer.o gpio_driver.o cli_uart.o cli.o device_registers.o encoder_driver.o eeprom_driver.o error_handler.o char1602.o meas.o led_driver.o menu.o
MCU_TARGET     = atmega8
OPTIMIZE       = -Os

DEFS           = -DF_CPU=8000000UL -D__AVR_ATmega8__
LIBS           =

## Include Directories
INCLUDES = -I"/avr/include"

# You should not have to change anything below here.

CC                    = C:/Users/Gurage/Downloads/avr8-gnu-toolchain/avr8-gnu-toolchain-win32_x86_64/bin/avr-gcc.exe

CFLAGS           = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS         = -Wl,-Map,$(PRG).mapB

OBJCOPY        = C:/Users/Gurage/Downloads/avr8-gnu-toolchain/avr8-gnu-toolchain-win32_x86_64/bin/avr-objcopy.exe
OBJDUMP        = C:/Users/Gurage/Downloads/avr8-gnu-toolchain/avr8-gnu-toolchain-win32_x86_64/bin/avr-objdump.exe

#all: $(PRG).elf lst text eeprom
all: $(PRG).elf lst text

$(PRG).elf: $(OBJ)
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@
