#include "gamedata.h"
#include <string.h>

// Headers van je grit conversies
#include "Zoro_card.h"
#include "LuffyG5_card.h"
#include "Vegeta_card.h"
#include "Shanks_card.h"

#define REG_DISPCNT  *(volatile uint32_t*)0x04000000
#define VRAM (uint16_t*)0x06000000

// Battle States
int bState = 0; // 0:Intro, 1:Menu, 2:PlayerAction, 3:EnemyAction, 10:Win, 11:XPAnim
int bTimer = 0;
int visual_xp = 0;

// Hulpfuncties voor graphics
void waitVBlank() {
    while(*(volatile uint16_t*)0x04000006 >= 160);
    while(*(volatile uint16_t*)0x04000006 < 160);
}

void drawRect(int x, int y, int w, int h, uint16_t color) {
    for(int i=0; i<h; i++) {
        for(int j=0; j<w; j++) {
            VRAM[(y+i)*240 + (x+j)] = color;
        }
    }
}

void drawHPBar(int x, int y, int hp, int max_hp, uint16_t color) {
    int breedte = (hp * 100) / max_hp;
    if (breedte < 0) breedte = 0;
    drawRect(x, y, 100, 4, 0x0000); // Achtergrond
    drawRect(x, y, breedte, 4, color);
}

void updateBattle() {
    waitVBlank();

    // 1. Teken de UI Basis (tegen flikkeren doen we dit elke frame)
    drawRect(0, 120, 240, 40, 0x1084); // Dialoogbox onder

    // 2. Battle Logica State Machine
    switch(bState) {
        case 0: // Intro
            drawText("EEN WILDE VIJAND VERSCHIJNT!", 10, 130, 0x7FFF);
            if(isKeyJustPressed(KEY_A)) bState = 1;
            break;

        case 1: // Menu
            drawText("A: VAL AAN", 10, 130, 0x7FFF);
            if(isKeyJustPressed(KEY_A)) { bState = 2; bTimer = 90; }
            break;

        case 2: // Player valt aan
            if(bTimer > 0) {
                char msg[32]; strcpy(msg, team[activeIdx].naam); strcat(msg, " GEBRUIKT AANVAL!");
                drawText(msg, 10, 130, 0x7FFF);
                bTimer--;
            } else {
                vijand_team[activeEnemyIdx].hp -= 30;
                bState = 3; bTimer = 90;
            }
            break;

        case 3: // Vijand valt aan
            if(vijand_team[activeEnemyIdx].hp <= 0) {
                bState = 11; visual_xp = team[activeIdx].xp;
            } else if(bTimer > 0) {
                drawText("TEGENSTANDER VALT AAN!", 10, 130, 0x7FFF);
                bTimer--;
            } else {
                team[activeIdx].hp -= 15;
                bState = 1;
            }
            break;

        case 11: // XP BALK ANIMATIE (V2.6)
            drawText("GEWONNEN! XP WORDT VERZAMELD...", 10, 130, 0x7FFF);
            // Teken de groeiende XP balk
            drawRect(120, 100, 100, 2, 0x0000);
            int xp_w = (visual_xp * 100) / team[activeIdx].xp_nodig;
            drawRect(120, 100, xp_w, 2, 0x7C00); // Blauwe balk

            if(visual_xp < (team[activeIdx].xp + 40)) {
                visual_xp++; // Hij loopt langzaam vol
                if(visual_xp >= team[activeIdx].xp_nodig) {
                    bState = 12; // Level up state
                }
            } else {
                if(isKeyJustPressed(KEY_A)) {
                    team[activeIdx].xp = visual_xp;
                    returnToOverworld();
                }
            }
            break;
            
        case 12: // Level Up Dialog
            drawText("LEVEL UP! JE BENT STERKER!", 10, 130, 0x03FF);
            if(isKeyJustPressed(KEY_A)) {
                team[activeIdx].lvl++;
                team[activeIdx].xp = 0;
                visual_xp = 0;
                bState = 11; 
            }
            break;
    }

    // Teken Namen en Types
    drawText(team[activeIdx].naam, 120, 80, 0x7FFF);
    drawText(typeNamen[team[activeIdx].moves[0].type], 200, 80, 0x03FF);
    drawHPBar(120, 90, team[activeIdx].hp, team[activeIdx].max_hp, 0x03E0);
}
