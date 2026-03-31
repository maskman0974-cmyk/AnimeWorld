#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <stdint.h>

// --- GBA HARDWARE REGISTERS ---
#define REG_DISPCNT  *(volatile uint32_t*)0x04000000
#define REG_VCOUNT   *(volatile uint16_t*)0x04000006
#define REG_BG0CNT   *(volatile uint16_t*)0x04000008

// --- DE NIEUWE CAMERA REGISTERS ---
#define REG_BG0HOFS  *(volatile uint16_t*)0x04000010
#define REG_BG0VOFS  *(volatile uint16_t*)0x04000012

#define MODE_0       0x0000 
#define MODE_3       0x0003
#define BG0_ENABLE   0x0100 
#define BG2_ENABLE   0x0400
#define OBJ_ENABLE   0x1000 
#define OBJ_MAP_1D   0x0040

#define VRAM         ((volatile uint16_t*)0x06000000)
#define CHAR_BASE_BLOCK(n)   ((volatile uint16_t*)(0x06000000 + ((n) * 16384)))
#define SCREEN_BASE_BLOCK(n) ((volatile uint16_t*)(0x06000000 + ((n) * 2048)))
#define PALETTE_MEM  ((volatile uint16_t*)0x05000000)
#define OBJ_PALETTE  ((volatile uint16_t*)0x05000200)
#define OAM_MEM      ((volatile uint16_t*)0x07000000)

// --- DMA REGISTERS (Voor supersnel inladen) ---
#define REG_DMA3SAD  *(volatile uint32_t*)0x040000D4 
#define REG_DMA3DAD  *(volatile uint32_t*)0x040000D8 
#define REG_DMA3CNT  *(volatile uint32_t*)0x040000DC 

#define DMA_ENABLE   0x80000000
#define DMA_16_BIT   0x00000000
#define DMA_32_BIT   0x04000000
#define DMA_SOURCE_FIXED 0x01000000 

typedef struct { uint16_t attr0; uint16_t attr1; uint16_t attr2; uint16_t dummy; } OBJ_ATTR;

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0x7FFF
#define COLOR_RED     0x001F 
#define COLOR_BLUE    0x7C00 
#define COLOR_YELLOW  0x03FF
#define COLOR_CYAN    0x7FE0
#define COLOR_GOLD    0x01FF 

void waitForVSync();

// Mode 0 Functies (Map)
void initGraphicsMode0();
void loadMap();
void initOAM();
void setSpriteAttribute(int id, int x, int y, int tile_index, int palette_index, int shape, int size);
void hideAllSprites();

// Mode 3 Functies (Battles)
void initGraphicsMode3();
void fillScreen(uint16_t color);
void drawPixel(int x, int y, uint16_t color);
void drawText(const char* text, int x, int y, uint16_t color);
void drawNumber(int number, int x, int y, uint16_t color);
void drawTextBox(const char* line1, const char* line2);
void updateAnimations();
void drawAnimatedSprite(int x, int y, const uint16_t** animFrames);

#endif