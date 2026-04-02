# --- ANIME WORLD MAKEFILE V2.6 ---
# Deze Makefile pakt automatisch ALLE .c bestanden in je map.

# Vertel de Makefile waar de GBA tools staan (nooit meer handmatig exporteren!)
export PATH := /opt/devkitpro/devkitARM/bin:$(PATH)

TARGET  := AnimeBattle
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ARCH    := -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall -std=gnu99
LDFLAGS := $(ARCH) -specs=gba.specs

# Zoek automatisch alle .c bestanden (inclusief je Trading Cards!)
CFILES  := $(wildcard *.c)
OBJS    := $(CFILES:.c=.o)

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -v -O binary $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.gba
