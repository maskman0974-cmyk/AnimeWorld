#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char naam[16];
    int kracht;
    int minLvl;
    int effect;       // 0: Normal, 1: Fire/Ki, 2: Special
    int effectChance; // Wordt nu gebruikt voor Type ID in UI
} Move;

typedef struct {
    char naam[16];
    int char_id;
    int lvl;
    int hp;
    int max_hp;
    int xp;
    int xp_nodig;
    int status; // 0: Base, 1: G2, 2: G4, 3: G5 etc.
    bool isGevuld;
    Move moves[4];
    const uint16_t* battle_front_bitmap;
    const uint16_t* battle_back_bitmap;
} Karakter;

// Declaraties voor extern gebruik
extern Karakter team[6];
extern Karakter vijand_team[6];
extern int activeIdx;
extern int activeEnemyIdx;
extern char* typeNamen[5];

int getBaseType(int char_id);

#endif
