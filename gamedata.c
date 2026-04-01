#include "gamedata.h"
#include <string.h>

Karakter team[6];
Karakter vijand_team[6];
int activeIdx = 0;
int activeEnemyIdx = 0;
int regio = 0;
int stage = 0;

char* typeNamen[5] = {"[FY]", "[KI]", "[CH]", "[HA]", "[DF]"};

void checkEvolutie(Karakter* k) {
    if (k->char_id == 0) { // Luffy
        if (k->lvl >= 50 && k->status < 3) { 
            k->status = 3; strcpy(k->naam, "G5 LUFFY"); k->max_hp += 50; 
        } else if (k->lvl >= 30 && k->status < 2) { 
            k->status = 2; strcpy(k->naam, "G4 LUFFY"); k->max_hp += 30; 
        } else if (k->lvl >= 15 && k->status < 1) { 
            k->status = 1; strcpy(k->naam, "G2 LUFFY"); k->max_hp += 20; 
        }
    }
}

void initKarakter(Karakter* k, int id, int lvl) {
    k->char_id = id;
    k->lvl = lvl;
    k->xp = 0;
    k->xp_nodig = 50 + (lvl * 10);
    k->status = 0;
    k->isGevuld = true;

    if (id == 0) { // Luffy
        strcpy(k->naam, "LUFFY");
        k->max_hp = 100 + (lvl * 5);
        // Moves
        strcpy(k->moves[0].naam, "PISTOL"); k->moves[0].kracht = 20; k->moves[0].type = 4;
        strcpy(k->moves[1].naam, "GATLING"); k->moves[1].kracht = 35; k->moves[1].type = 4;
    } 
    else if (id == 1) { // Zoro
        strcpy(k->naam, "ZORO");
        k->max_hp = 95 + (lvl * 6);
        strcpy(k->moves[0].naam, "ONI GIRI"); k->moves[0].kracht = 25; k->moves[0].type = 3;
    }
    k->hp = k->max_hp;
}
