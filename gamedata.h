#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdbool.h>
#include <stdint.h>

// Toetsen (Verwijder ze dus uit main.c!)
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

typedef struct {
    char naam[16];
    int kracht;
    int minLvl;
    int effect;
    int effectChance; 
} Move;

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

extern char* itemNamen[5];
extern int itemHeal[5];
extern int itemAantal[5];

extern Karakter team[6];
extern Karakter vijand_team[6];
extern Karakter pcBox[30];

extern int activeEnemyIdx;
extern int activeIdx;
extern int regio;

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
