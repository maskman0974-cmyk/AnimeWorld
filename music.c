#include "music.h"
#include <stdint.h>

#define REG_SOUND2CNT_L *(volatile uint16_t*)0x04000068
#define REG_SOUND2CNT_H *(volatile uint16_t*)0x0400006C

const uint16_t bgm_onepiece[8] = {1651, 0, 1751, 1783, 1798, 1783, 1751, 0};
const uint16_t bgm_naruto[8]   = {1751, 1798, 1602, 0, 1602, 1651, 1798, 1751};
const uint16_t bgm_dbz[8]      = {1714, 1714, 1673, 1714, 1798, 1714, 1673, 1651};

int bgmTimer = 0; int noteIndex = 0; int currentRegion = -1;

void updateBGM(int region) {
    if (currentRegion != region) { currentRegion = region; noteIndex = 0; bgmTimer = 0; }
    bgmTimer--;
    if (bgmTimer <= 0) {
        bgmTimer = 15; uint16_t note = 0;
        if (region == 0) note = bgm_onepiece[noteIndex];
        else if (region == 1) note = bgm_naruto[noteIndex];
        else if (region == 2) note = bgm_dbz[noteIndex];
        if (note != 0) { REG_SOUND2CNT_L = 0x8277; REG_SOUND2CNT_H = 0x8000 | note; }
        noteIndex = (noteIndex + 1) % 8;
    }
}
void stopBGM() { REG_SOUND2CNT_L = 0; currentRegion = -1; }