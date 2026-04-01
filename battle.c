#include "gamedata.h"
#include <string.h>

#define VRAM (uint16_t*)0x06000000

// Externe functies uit main.c
extern void drawRect(int x, int y, int w, int h, uint16_t color);
extern void drawText(const char* text, int x, int y, uint16_t color);
extern bool isKeyJustPressed(uint16_t key);
extern void waitVBlank();
extern void returnToOverworld();

// Battle Globals
int bState = 0; // 0:Intro, 1:Menu, 2:PlayerAct, 3:EnemyAct, 11:XP_Anim, 12:LevelUp
int bTimer = 0;
int visual_xp = 0;

void drawHPBar(int x, int y, int hp, int max_hp, uint16_t color) {
    int breedte = (hp * 100) / max_hp;
    if (breedte < 0) breedte = 0;
    if (breedte > 100) breedte = 100;
    drawRect(x, y, 100, 4, 0x0000); // Zwarte achtergrond
    drawRect(x, y, breedte, 4, color); // Levensbalk
}

void updateBattle() {
    waitVBlank();

    // UI Basis (Altijd tekenen om flikkeren te voorkomen)
    drawRect(0, 110, 240, 50, 0x1084); // Onderste dialoogbox
    drawRect(0, 110, 240, 2, 0x7FFF);  // Witte randlijn

    // Enemy Box (Boven)
    drawRect(10, 10, 110, 30, 0x0000);
    drawText(vijand_team[activeEnemyIdx].naam, 12, 12, 0x7FFF);
    drawText(typeNamen[getBaseType(vijand_team[activeEnemyIdx].char_id)], 85, 12, 0x03FF);
    drawHPBar(12, 25, vijand_team[activeEnemyIdx].hp, vijand_team[activeEnemyIdx].max_hp, 0x03E0);

    // Player Box (Midden-Rechts)
    drawRect(120, 60, 110, 40, 0x0000);
    drawText(team[activeIdx].naam, 122, 62, 0x7FFF);
    drawText(typeNamen[team[activeIdx].moves[0].type], 195, 62, 0x03FF);
    drawHPBar(122, 75, team[activeIdx].hp, team[activeIdx].max_hp, 0x03E0);

    // V2.6 XP Balk (Blauw)
    drawRect(122, 85, 100, 2, 0x2108); // Donkere achtergrond
    int xp_w = (visual_xp * 100) / team[activeIdx].xp_nodig;
    if (xp_w > 100) xp_w = 100;
    drawRect(122, 85, xp_w, 2, 0x7C00); // Blauwe vulling

    // Battle Logica & Timers
    switch(bState) {
        case 0: // Intro
            drawText("EEN VIJAND VALT AAN!", 10, 120, 0x7FFF);
            if(isKeyJustPressed(KEY_A)) bState = 1;
            break;

        case 1: // Menu
            drawText("A: VAL AAN", 10, 120, 0x7FFF);
            if(isKeyJustPressed(KEY_A)) { bState = 2; bTimer = 100; } // Vertraging ingebouwd
            break;

        case 2: // Player Attack
            if(bTimer > 0) {
                drawText("JE VALT AAN!", 10, 120, 0x7FFF);
                bTimer--;
            } else {
                vijand_team[activeEnemyIdx].hp -= 40; // Test damage
                bState = 3; bTimer = 100;
            }
            break;

        case 3: // Enemy Attack
            if(vijand_team[activeEnemyIdx].hp <= 0) {
                bState = 11; visual_xp = team[activeIdx].xp;
            } else if(bTimer > 0) {
                drawText("TEGENSTANDER SLAAT TERUG!", 10, 120, 0x7FFF);
                bTimer--;
            } else {
                team[activeIdx].hp -= 15;
                bState = 1;
            }
            break;

        case 11: // XP Animatie
            drawText("GEWONNEN! XP VERZAMELEN...", 10, 120, 0x7FFF);
            if(visual_xp < (team[activeIdx].xp + 50)) {
                visual_xp++; // Balk loopt vloeiend vol
                if(visual_xp >= team[activeIdx].xp_nodig) bState = 12;
            } else {
                if(isKeyJustPressed(KEY_A)) {
                    team[activeIdx].xp = visual_xp;
                    returnToOverworld();
                }
            }
            break;
            
        case 12: // Level Up
            drawText("LEVEL UP! JE BENT STERKER!", 10, 120, 0x03FF);
            if(isKeyJustPressed(KEY_A)) {
                team[activeIdx].lvl++;
                team[activeIdx].xp = 0;
                visual_xp = 0;
                checkEvolutie(&team[activeIdx]);
                bState = 11; 
            }
            break;
    }
}
