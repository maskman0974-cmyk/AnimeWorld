# --- PROJECT NAAM ---
ROM = AnimeBattle.gba
ELF = AnimeBattle.elf

# --- COMPILER INSTELLINGEN ---
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mthumb -mthumb-interwork -O2 -Wall
LDFLAGS = -specs=gba.specs

# --- BRONBESTANDEN (Hier staan alle karakters en mappen!) ---
# --- BRONBESTANDEN ---
SOURCES = main.c graphics.c gamedata.c battle.c sound.c music.c \
          bg.c battle_bg.c marineford.c alabasta.c konoha.c shibuya.c \
          luffy.c luffy_front.c luffy_back.c \
          zoro_front.c zoro_back.c \
          kaido_front.c kaido_back.c \
          goku_front.c goku_back.c \
          madara_front.c madara_back.c \
          naruto_front.c naruto_back.c \
          obito_front.c obito_back.c \
          pain_front.c pain_back.c \
          shanks_front.c shanks_back.c \
          vegeta_front.c vegeta_back.c \
          itachi_front.c itachi_back.c

# --- OBJECT BESTANDEN BEREKENEN ---
OBJECTS = $(SOURCES:.c=.o)

# --- BOUW REGELS ---
all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(ELF): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Hoe hij een .c bestand omzet in een .o bestand
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Commando om de map op te ruimen
clean:
	rm -f *.o *.elf *.gba