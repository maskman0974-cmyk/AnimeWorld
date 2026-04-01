#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <stdint.h>
#include <stdbool.h>

// Toetsen definities voor de hele game
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

// Struct voor aanvallen
typedef struct {
    char naam[16];
    int kracht;
    int minLvl;
    int type; // 0:FY, 1:KI, 2:CH, 3:HA, 4:DF
} Move;

// Struct voor je helden en vijanden (volledige versie)
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

// Prototypes voor globaal gebruik
void initKarakter(Karakter* k, int id, int lvl);
void checkEvolutie(Karakter* k);
void initTeam();
int getBaseType(int char_id);

#endif
