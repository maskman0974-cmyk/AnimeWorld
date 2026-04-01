#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdint.h>
#include <stdbool.h>

// Struct voor aanvallen
typedef struct {
    char naam[16];
    int kracht;
    int minLvl;
    int type; // 0:FY, 1:KI, 2:CH, 3:HA, 4:DF
} Move;

// Struct voor je helden en vijanden
typedef struct {
    char naam[16];
    int char_id;
    int lvl;
    int hp;
    int max_hp;
    int xp;
    int xp_nodig;
    int status; // 0:Base, 1:Evo1, 2:Evo2
    bool isGevuld;
    Move moves[4];
} Karakter;

// Global variabelen die overal gebruikt worden
extern Karakter team[6];
extern Karakter vijand_team[6];
extern int activeIdx;
extern int activeEnemyIdx;
extern int regio;
extern int stage;
extern char* typeNamen[5];

// Prototypes
void initKarakter(Karakter* k, int id, int lvl);
void checkEvolutie(Karakter* k);

#endif
