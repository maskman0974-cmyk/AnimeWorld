#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdint.h>
#include <stdbool.h>

#define BITMAP_FRONT_SIZE 64*64 
#define BITMAP_BACK_SIZE  32*32 

typedef struct {
    char naam[20];
    int kracht;
    int minLvl;
    int effect;       
    int effectChance; 
} Move;

typedef struct {
    int char_id;
    char naam[20];
    int lvl;
    int hp;
    int max_hp;
    int xp;
    int xp_nodig;
    bool isGevuld;
    int status;       
    const uint16_t* battle_front_bitmap;
    const uint16_t* battle_back_bitmap;
    Move moves[4];
} Karakter;

extern Karakter team[6];
extern Karakter vijand;
extern int activeIdx;
extern int regio;
extern int berries; 
extern int itemAantal[5];
extern char* itemNamen[5];
extern int itemHeal[5];

void initKarakter(Karakter* k, int id, int lvl);
void initTeam(void);
bool checkNewMove(int char_id, int lvl, Move* newMove);
void setMove(Move* m, const char* naam, int kracht, int minLvl, int effect, int chance);
void healWholeTeam(void);
int getEmptyPartySlot(void);
bool checkAliveTeam(void);
void checkEvolutie(Karakter* k); // <-- Hier is de functie die de compiler zocht!

#endif