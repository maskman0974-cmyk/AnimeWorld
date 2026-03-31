#include "sound.h"
#include <stdint.h>

#define REG_SOUNDCNT_L *(volatile uint16_t*)0x04000080
#define REG_SOUNDCNT_H *(volatile uint16_t*)0x04000082
#define REG_SOUNDCNT_X *(volatile uint16_t*)0x04000084
#define REG_SOUND1CNT_L *(volatile uint16_t*)0x04000060
#define REG_SOUND1CNT_H *(volatile uint16_t*)0x04000062
#define REG_SOUND1CNT_X *(volatile uint16_t*)0x04000064
#define REG_SOUND2CNT_L *(volatile uint16_t*)0x04000068
#define REG_SOUND2CNT_H *(volatile uint16_t*)0x0400006C

void initSound() {
    REG_SOUNDCNT_X = 0x0080; REG_SOUNDCNT_L = 0x7777; REG_SOUNDCNT_H = 0x020F;
}
void playSoundAttack()  { REG_SOUND1CNT_L=0x0027; REG_SOUND1CNT_H=0x81FA; REG_SOUND1CNT_X=0x8500; }
void playSoundDamage()  { REG_SOUND1CNT_L=0x0000; REG_SOUND1CNT_H=0x8477; REG_SOUND1CNT_X=0x8200; }
void playSoundVictory() { REG_SOUND2CNT_L=0x81FA; REG_SOUND2CNT_H=0x8600; }