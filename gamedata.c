#include "gamedata.h"
#include <string.h>
#include <stdlib.h> 

#include "luffy_front.h"
#include "luffy_back.h"
#include "zoro_front.h"
#include "zoro_back.h"
#include "kaido_front.h"
#include "kaido_back.h"
#include "goku_front.h"
#include "goku_back.h"
#include "naruto_front.h"
#include "naruto_back.h"
#include "vegeta_front.h"
#include "vegeta_back.h"
#include "shanks_front.h"
#include "shanks_back.h"
#include "madara_front.h"
#include "madara_back.h"
#include "obito_front.h"
#include "obito_back.h"
#include "pain_front.h"
#include "pain_back.h"
#include "itachi_front.h"
#include "itachi_back.h"

char* itemNamen[5] = {"MEAT BONE", "TOAD OIL", "SENZU BEAN", "CAPTURE NET", "CHOPPER MED"};
int itemHeal[5]    = {50, 100, 999, 0, 0}; 
int itemAantal[5]  = {5, 3, 1, 5, 0};      

Karakter team[6];
Karakter vijand_team[6];
Karakter pcBox[30]; 

int activeEnemyIdx = 0;
int activeIdx = 0;
int regio = 0; 

// AANGEPAST: 'chance' is nu het Element Type (0=Fysiek, 1=Ki, 2=Chakra, 3=Haki, 4=Devil Fruit)
void setMove(Move* m, const char* naam, int kracht, int minLvl, int effect, int chance) {
    strcpy(m->naam, naam); 
    m->kracht = kracht; 
    m->minLvl = minLvl; 
    m->effect = effect; 
    m->effectChance = chance; 
}

int getEmptyPartySlot(void) {
    for(int i = 0; i < 6; i++) { if(!team[i].isGevuld) return i; } 
    return -1;
}

int getEmptyPcSlot(void) {
    for(int i = 0; i < 30; i++) { if(!pcBox[i].isGevuld) return i; } 
    return -1;
}

void healWholeTeam(void) {
    for(int i=0; i<6; i++) { if(team[i].isGevuld) { team[i].hp = team[i].max_hp; } }
}

bool checkAliveTeam() {
    for(int i = 0; i < 6; i++) { if (team[i].isGevuld && team[i].hp > 0) return true; }
    return false;
}

void checkNewMoves(Karakter* k) {
    if (k->char_id == 0) { // LUFFY (Type 4: Devil Fruit)
        if (k->lvl >= 15) setMove(&k->moves[1], "JET GATLING", 60, 15, 0, 4);
        if (k->lvl >= 30) setMove(&k->moves[2], "KONG GUN", 120, 30, 0, 4);
        if (k->lvl >= 50) setMove(&k->moves[3], "BAJRANG GUN", 250, 50, 2, 4);
    }
    else if (k->char_id == 1) { // ZORO (Type 3: Haki)
        if (k->lvl >= 15) setMove(&k->moves[1], "TATSUMAKI", 65, 15, 1, 3);
        if (k->lvl >= 30) setMove(&k->moves[2], "ASHURA", 130, 30, 0, 3);
        if (k->lvl >= 50) setMove(&k->moves[3], "103 MERCIES", 240, 50, 0, 3);
    }
    else if (k->char_id == 3) { // GOKU (Type 1: Ki)
        if (k->lvl >= 15) setMove(&k->moves[1], "SUPER KAME", 75, 15, 1, 1);
        if (k->lvl >= 30) setMove(&k->moves[2], "DIVINE KAME", 140, 30, 1, 1);
        if (k->lvl >= 50) setMove(&k->moves[3], "MASTERED UI", 260, 50, 2, 1);
    }
    else if (k->char_id == 4) { // NARUTO (Type 2: Chakra)
        if (k->lvl >= 15) setMove(&k->moves[1], "GIANT RASEN", 75, 15, 2, 2);
        if (k->lvl >= 30) setMove(&k->moves[2], "BIJUU BOMB", 135, 30, 2, 2);
        if (k->lvl >= 50) setMove(&k->moves[3], "BARYON SMASH", 250, 50, 2, 2);
    }
    else if (k->char_id == 5) { // VEGETA (Type 1: Ki)
        if (k->lvl >= 15) setMove(&k->moves[1], "BIG BANG", 75, 15, 2, 1);
        if (k->lvl >= 30) setMove(&k->moves[2], "PROMINENCE", 140, 30, 2, 1);
        if (k->lvl >= 50) setMove(&k->moves[3], "FINAL HAKAI", 255, 50, 1, 1);
    }
}

bool checkEvolutie(Karakter* k) {
    bool evolueerde = false;
    if (k->char_id == 0) { 
        if (k->lvl >= 50 && k->status < 3) { k->status = 3; strcpy(k->naam, "GEAR 5 LUFFY"); k->max_hp += 50; evolueerde = true; } 
        else if (k->lvl >= 30 && k->status < 2) { k->status = 2; strcpy(k->naam, "GEAR 4 LUFFY"); k->max_hp += 30; evolueerde = true; } 
        else if (k->lvl >= 15 && k->status < 1) { k->status = 1; strcpy(k->naam, "GEAR 2 LUFFY"); k->max_hp += 20; evolueerde = true; }
    }
    else if (k->char_id == 1) { 
        if (k->lvl >= 50 && k->status < 3) { k->status = 3; strcpy(k->naam, "KING OF HELL"); k->max_hp += 40; evolueerde = true; } 
        else if (k->lvl >= 30 && k->status < 2) { k->status = 2; strcpy(k->naam, "ASHURA ZORO"); k->max_hp += 25; evolueerde = true; } 
        else if (k->lvl >= 15 && k->status < 1) { k->status = 1; strcpy(k->naam, "SANTORYU"); k->max_hp += 15; evolueerde = true; }
    }
    else if (k->char_id == 3) { 
        if (k->lvl >= 50 && k->status < 3) { k->status = 3; strcpy(k->naam, "UI GOKU"); k->max_hp += 60; evolueerde = true; } 
        else if (k->lvl >= 30 && k->status < 2) { k->status = 2; strcpy(k->naam, "SSJ BLUE GOKU"); k->max_hp += 35; evolueerde = true; } 
        else if (k->lvl >= 15 && k->status < 1) { k->status = 1; strcpy(k->naam, "SSJ GOKU"); k->max_hp += 20; evolueerde = true; }
    }
    else if (k->char_id == 4) { 
        if (k->lvl >= 50 && k->status < 3) { k->status = 3; strcpy(k->naam, "BARYON MODE"); k->max_hp += 55; evolueerde = true; } 
        else if (k->lvl >= 30 && k->status < 2) { k->status = 2; strcpy(k->naam, "KCM NARUTO"); k->max_hp += 30; evolueerde = true; } 
        else if (k->lvl >= 15 && k->status < 1) { k->status = 1; strcpy(k->naam, "SAGE NARUTO"); k->max_hp += 20; evolueerde = true; }
    }
    else if (k->char_id == 5) { 
        if (k->lvl >= 50 && k->status < 3) { k->status = 3; strcpy(k->naam, "ULTRA EGO"); k->max_hp += 50; evolueerde = true; } 
        else if (k->lvl >= 30 && k->status < 2) { k->status = 2; strcpy(k->naam, "SSJ BLUE VEG"); k->max_hp += 30; evolueerde = true; } 
        else if (k->lvl >= 15 && k->status < 1) { k->status = 1; strcpy(k->naam, "SSJ VEGETA"); k->max_hp += 20; evolueerde = true; }
    }
    return evolueerde;
}

void initKarakter(Karakter* k, int id, int lvl) {
    k->char_id = id; k->lvl = lvl; k->xp = 0; k->xp_nodig = 50 + (lvl * 15); 
    k->isGevuld = true; k->status = 0; 
    
    // Type ID = 0 (Normal), 1 (Ki), 2 (Chakra), 3 (Haki), 4 (Devil Fruit)
    if (id == 0) { 
        strcpy(k->naam, "LUFFY"); k->max_hp = 100 + (lvl * 15); 
        setMove(&k->moves[0], "PISTOL", 20, 1, 0, 4); 
        setMove(&k->moves[1], "GATLING", 40, 10, 0, 4); 
        setMove(&k->moves[2], "RED HAWK", 85, 20, 1, 4); 
        setMove(&k->moves[3], "-", 0, 99, 0, 0); 
        k->battle_front_bitmap = (const uint16_t*)luffy_frontBitmap; k->battle_back_bitmap = (const uint16_t*)luffy_backBitmap;
    }
    else if (id == 1) { 
        strcpy(k->naam, "ZORO"); k->max_hp = 90 + (lvl * 14); 
        setMove(&k->moves[0], "ONI GIRI", 22, 1, 0, 3); 
        setMove(&k->moves[1], "1080 PHOENIX", 48, 10, 0, 3); 
        setMove(&k->moves[2], "SILVER MIST", 90, 20, 0, 3); 
        setMove(&k->moves[3], "-", 0, 99, 0, 0); 
        k->battle_front_bitmap = (const uint16_t*)zoro_frontBitmap; k->battle_back_bitmap = (const uint16_t*)zoro_backBitmap;
    }
    else if (id == 2) { 
        strcpy(k->naam, "KAIDO"); k->max_hp = 150 + (lvl * 18); 
        setMove(&k->moves[0], "THUNDER BAGUA", 50, 1, 0, 3); 
        setMove(&k->moves[1], "BLAST BREATH", 90, 15, 1, 4); 
        setMove(&k->moves[2], "DRAGON TWISTER", 120, 25, 0, 4); 
        setMove(&k->moves[3], "FLAMING DRAGON", 200, 35, 2, 4); 
        k->battle_front_bitmap = (const uint16_t*)kaido_frontBitmap; k->battle_back_bitmap = (const uint16_t*)kaido_backBitmap;
    }
    else if (id == 3) { 
        strcpy(k->naam, "GOKU"); k->max_hp = 110 + (lvl * 16); 
        setMove(&k->moves[0], "PUNCH", 25, 1, 0, 0); 
        setMove(&k->moves[1], "KAMEHAMEHA", 65, 10, 1, 1); 
        setMove(&k->moves[2], "METEOR CRASH", 100, 20, 0, 1); 
        setMove(&k->moves[3], "-", 0, 99, 0, 0); 
        k->battle_front_bitmap = (const uint16_t*)goku_frontBitmap; k->battle_back_bitmap = (const uint16_t*)goku_backBitmap;
    }
    else if (id == 4) { 
        strcpy(k->naam, "NARUTO"); k->max_hp = 100 + (lvl * 15); 
        setMove(&k->moves[0], "KUNAI", 20, 1, 0, 0); 
        setMove(&k->moves[1], "RASENGAN", 65, 10, 2, 2); 
        setMove(&k->moves[2], "CLONE JUTSU", 40, 20, 0, 2); 
        setMove(&k->moves[3], "-", 0, 99, 0, 0); 
        k->battle_front_bitmap = (const uint16_t*)naruto_frontBitmap; k->battle_back_bitmap = (const uint16_t*)naruto_backBitmap;
    }
    else if (id == 5) { 
        strcpy(k->naam, "VEGETA"); k->max_hp = 105 + (lvl * 15);
        setMove(&k->moves[0], "PUNCH", 30, 1, 0, 0); 
        setMove(&k->moves[1], "GALICK GUN", 70, 10, 1, 1);
        setMove(&k->moves[2], "BIG BANG", 110, 20, 2, 1); 
        setMove(&k->moves[3], "-", 0, 99, 0, 0);
        k->battle_front_bitmap = (const uint16_t*)vegeta_frontBitmap; k->battle_back_bitmap = (const uint16_t*)vegeta_backBitmap;
    }
    else if (id == 6) { 
        strcpy(k->naam, "SHANKS"); k->max_hp = 120 + (lvl * 14);
        setMove(&k->moves[0], "SLASH", 35, 1, 0, 3); 
        setMove(&k->moves[1], "HAKI STRIKE", 75, 12, 1, 3);
        setMove(&k->moves[2], "GRIFFON DASH", 100, 22, 0, 3); 
        setMove(&k->moves[3], "DIVINE DEPART", 180, 32, 0, 3);
        k->battle_front_bitmap = (const uint16_t*)shanks_frontBitmap; k->battle_back_bitmap = (const uint16_t*)shanks_backBitmap;
    }
    else if (id == 7) { 
        strcpy(k->naam, "MADARA"); k->max_hp = 115 + (lvl * 17);
        setMove(&k->moves[0], "FIREBALL", 40, 1, 1, 2); 
        setMove(&k->moves[1], "SUSANOO RIB", 60, 15, 3, 2);
        setMove(&k->moves[2], "METEOR", 130, 25, 3, 2); 
        setMove(&k->moves[3], "PERFECT SUSANOO", 200, 35, 0, 2);
        k->battle_front_bitmap = (const uint16_t*)madara_frontBitmap; k->battle_back_bitmap = (const uint16_t*)madara_backBitmap;
    }
    else if (id == 8) { 
        strcpy(k->naam, "OBITO"); k->max_hp = 95 + (lvl * 14);
        setMove(&k->moves[0], "SHURIKEN", 25, 1, 0, 0); 
        setMove(&k->moves[1], "KAMUI", 50, 12, 3, 2); 
        setMove(&k->moves[2], "FIRE STYLE", 80, 22, 1, 2); 
        setMove(&k->moves[3], "WOOD DRAGON", 140, 32, 0, 2);
        k->battle_front_bitmap = (const uint16_t*)obito_frontBitmap; k->battle_back_bitmap = (const uint16_t*)obito_backBitmap;
    }
    else if (id == 9) { 
        strcpy(k->naam, "PAIN"); k->max_hp = 110 + (lvl * 15);
        setMove(&k->moves[0], "ROD STRIKE", 30, 1, 0, 0); 
        setMove(&k->moves[1], "BANSHO TENIN", 60, 12, 3, 2);
        setMove(&k->moves[2], "ALMIGHTY PUSH", 120, 22, 2, 2); 
        setMove(&k->moves[3], "PLANETARY DEV", 190, 32, 3, 2);
        k->battle_front_bitmap = (const uint16_t*)pain_frontBitmap; k->battle_back_bitmap = (const uint16_t*)pain_backBitmap;
    }
    else if (id == 10) { 
        strcpy(k->naam, "ITACHI"); k->max_hp = 100 + (lvl * 15);
        setMove(&k->moves[0], "KUNAI", 25, 1, 0, 0); 
        setMove(&k->moves[1], "FIREBALL", 55, 12, 1, 2);
        setMove(&k->moves[2], "AMATERASU", 120, 22, 1, 2); 
        setMove(&k->moves[3], "TSUKUYOMI", 160, 32, 3, 2);
        k->battle_front_bitmap = (const uint16_t*)itachi_frontBitmap; k->battle_back_bitmap = (const uint16_t*)itachi_backBitmap;
    }
    
    k->hp = k->max_hp; 
    
    checkEvolutie(k); 
    checkNewMoves(k);
}

void initTeam(void) { 
    activeIdx = 0; 
    for(int i = 0; i < 6; i++) { team[i].isGevuld = false; }
    for(int i = 0; i < 30; i++) { pcBox[i].isGevuld = false; } 
    initKarakter(&team[0], 0, 5); 
}
