#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdbool.h>
#include <stdint.h>

// Structuur voor een aanval
typedef struct {
    char naam[16];
    int kracht;
    int minLvl;
    int effect;
    int effectChance; 
} Move;

// Structuur voor een karakter
typedef struct {
    int char_id;
    char naam[20];
    int lvl;
    int xp;
    int xp_nodig;
    int hp;
    int max_hp;
    bool isGevuld;
    int status; 
    Move moves[4];
    const uint16_t* battle_front_bitmap;
    const uint16_t* battle_back_bitmap;
} Karakter;

// Externe variabelen voor Items
extern char* itemNamen[5];
extern int itemHeal[5];
extern int itemAantal[5];

// Externe variabelen voor Teams en PC
extern Karakter team[6];
extern Karakter vijand_team[6];
extern Karakter pcBox[30];

// Externe variabelen voor de game state
extern int activeEnemyIdx;
extern int activeIdx;
extern int regio;

// Functie declaraties
void setMove(Move* m, const char* naam, int kracht, int minLvl, int effect, int chance);
int getEmptyPartySlot(void);
int getEmptyPcSlot(void);
void healWholeTeam(void);
bool checkAliveTeam(void);
void checkNewMoves(Karakter* k);
bool checkEvolutie(Karakter* k);
void initKarakter(Karakter* k, int id, int lvl);
void initTeam(void);
int getBaseType(int char_id);

#endif
