#include "gamedata.h"
#include "sound.h"
#include <stdlib.h>
#include <string.h>

#include "battle_bg.h"

#define REG_KEYINPUT *(volatile uint16_t*)0x04000130
#define KEY_A 0x0001
#define KEY_B 0x0002
#define KEY_RIGHT 0x0010
#define KEY_LEFT 0x0020
#define KEY_UP 0x0040
#define KEY_DOWN 0x0080

#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0x7FFF
#define COLOR_GOLD   0x03FF
#define COLOR_RED    0x001F
#define UI_BG        0x4A69 
#define UI_BORDER    0x7FFF 
#define BAR_GREEN    0x03E0 
#define BAR_YELLOW   0x03FF
#define BAR_RED      0x001F
#define BAR_BLUE     0x7C00

extern void initGraphicsMode3(void);
extern void drawPixel(int x, int y, uint16_t color);
extern void drawText(char* text, int x, int y, uint16_t color);
extern void drawNumber(int num, int x, int y, uint16_t color);
extern void drawTextBox(char* line1, char* line2);

extern bool isKeyJustPressed(uint16_t key);
extern bool isKeyHeld(uint16_t key);

extern int interactie_npc; 
extern int regio;
extern int regio_stage[]; 

extern Karakter vijand_team[6];
extern int activeEnemyIdx;
extern char* itemNamen[];
extern int itemHeal[];
extern int itemAantal[];

int bState = 0; 
int prevState = -1; 
int bTimer = 0; 
bool bBoss = false;
int bMenu = 0;       
int bMoveSelect = 0; 
int teamSelect = 0; 
int bBagCursor = 0; 

int pOffX = 0; 
int eOffX = 0;
bool pVis = true;
bool eVis = true;

void drawRect(int x, int y, int w, int h, uint16_t color) {
    for(int r = 0; r < h; r++) {
        for(int c = 0; c < w; c++) {
            drawPixel(x + c, y + r, color);
        }
    }
}

void drawBitmapSprite(int x, int y, int width, int height, const uint16_t* data) {
    uint16_t transColor = data[0]; 
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            uint16_t color = data[r * width + c];
            if (color != transColor && color != 0x7C1F && color != 0x0000) { 
                drawPixel(x + c, y + r, color);
            }
        }
    }
}

void drawUIBox(int x, int y, int w, int h) {
    for(int r = 0; r < h; r++) {
        for(int c = 0; c < w; c++) {
            uint16_t color = UI_BG;
            if (r < 2 || r > h-3 || c < 2 || c > w-3) color = UI_BORDER; 
            else if (r == 2 || r == h-3 || c == 2 || c == w-3) color = COLOR_BLACK; 
            drawPixel(x + c, y + r, color);
        }
    }
}

void drawBar(int x, int y, int w, int h, int val, int max, uint16_t color) {
    drawRect(x, y, w, h, COLOR_BLACK);
    if (max <= 0 || val <= 0) return;
    int fill = (val * w) / max;
    if (fill > w) fill = w;
    drawRect(x, y, fill, h, color);
}

void drawBattleUI() {
    drawUIBox(4, 8, 118, 34); 
    drawText(vijand_team[activeEnemyIdx].naam, 10, 12, COLOR_WHITE);
    drawText("LVL", 92, 12, COLOR_GOLD); 
    drawNumber(vijand_team[activeEnemyIdx].lvl, 110, 12, COLOR_GOLD);
    
    uint16_t vColor = BAR_GREEN;
    if (vijand_team[activeEnemyIdx].hp < vijand_team[activeEnemyIdx].max_hp / 2) vColor = BAR_YELLOW;
    if (vijand_team[activeEnemyIdx].hp < vijand_team[activeEnemyIdx].max_hp / 5) vColor = BAR_RED;
    drawBar(10, 25, 106, 5, vijand_team[activeEnemyIdx].hp, vijand_team[activeEnemyIdx].max_hp, vColor);
    
    drawUIBox(118, 68, 118, 42); 
    drawText(team[activeIdx].naam, 124, 72, COLOR_WHITE);
    drawText("LVL", 195, 72, COLOR_GOLD); 
    drawNumber(team[activeIdx].lvl, 215, 72, COLOR_GOLD);
    
    uint16_t pColor = BAR_GREEN;
    if (team[activeIdx].hp < team[activeIdx].max_hp / 2) pColor = BAR_YELLOW;
    if (team[activeIdx].hp < team[activeIdx].max_hp / 5) pColor = BAR_RED;
    drawBar(124, 85, 105, 5, team[activeIdx].hp, team[activeIdx].max_hp, pColor);
    drawBar(124, 94, 105, 2, team[activeIdx].xp, team[activeIdx].xp_nodig, BAR_BLUE);
}

void drawVFX() {
    if (bState == 2 && bTimer <= 25 && bTimer >= 15) { 
        int fx = team[activeIdx].moves[bMoveSelect].effect;
        if (fx == 0) { drawRect(150, 30, 40, 5, COLOR_WHITE); drawRect(150, 40, 40, 5, COLOR_RED); } 
        else if (fx == 1) { drawRect(60, 40, 100, 12, 0x01FF); } 
        else if (fx == 2) { drawRect(140, 20, 30, 30, BAR_BLUE); } 
        else if (fx == 3) { drawRect(130, 10, 60, 60, 0x401F); } 
    }
    else if (bState == 3 && bTimer <= 25 && bTimer >= 15) { 
        int fx = vijand_team[activeEnemyIdx].moves[0].effect;
        if (fx == 0) { drawRect(40, 50, 40, 5, COLOR_WHITE); drawRect(40, 60, 40, 5, COLOR_RED); } 
        else if (fx == 1) { drawRect(80, 60, 100, 12, 0x01FF); } 
        else if (fx == 2) { drawRect(30, 50, 30, 30, BAR_BLUE); } 
        else if (fx == 3) { drawRect(20, 40, 60, 60, 0x401F); } 
    }
}

void redrawBattleScene() {
    *(volatile uint32_t*)0x040000D4 = (uint32_t)battle_bgBitmap;
    *(volatile uint32_t*)0x040000D8 = (uint32_t)0x06000000;
    *(volatile uint32_t*)0x040000DC = 0x80000000 | 0x04000000 | 19200; 

    if (pVis) drawBitmapSprite(24 + pOffX, 56, 64, 64, team[activeIdx].battle_back_bitmap);
    if (eVis) drawBitmapSprite(150 + eOffX, 16, 64, 64, vijand_team[activeEnemyIdx].battle_front_bitmap);

    drawVFX(); 

    if (bState != 10 && bState != 12 && bState != 14) {
        drawBattleUI();
    }
}

int calculateDamage(Karakter* attacker, Move* m) {
    return (m->kracht * attacker->lvl) / 5 + (attacker->lvl * 2);
}

void startBattle(bool isStory) {
    bBoss = isStory; bState = 0; prevState = -1; bTimer = 60; bMenu = 0; bMoveSelect = 0; teamSelect = 0; bBagCursor = 0;
    pOffX = 0; eOffX = 0; pVis = true; eVis = true;

    activeEnemyIdx = 0;
    for(int i=0; i<6; i++) vijand_team[i].isGevuld = false;

    if (isStory) {
        int stage = regio_stage[regio];
        
        // DE BALANS FIX: Lvl 7 in plaats van Lvl 18 voor baas 1.
        int baseLvl = 3 + (regio * 10) + (stage * 2); 
        
        for(int i = 0; i < stage; i++) {
            int random_minion_id = rand() % 11; 
            initKarakter(&vijand_team[i], random_minion_id, baseLvl + i); 
        }
        int lastSlot = stage - 1;
        if (lastSlot > 5) lastSlot = 5;
        initKarakter(&vijand_team[lastSlot], interactie_npc, baseLvl + 2); 
    } else {
        int wild_id = 1; 
        if (regio == 0)      wild_id = (rand() % 2 == 0) ? 0 : 1; 
        else if (regio == 1) wild_id = (rand() % 2 == 0) ? 0 : 6; 
        else if (regio == 2) wild_id = (rand() % 2 == 0) ? 3 : 5; 
        else if (regio == 3) wild_id = (rand() % 3 == 0) ? 4 : (rand()%2==0?8:10); 
        else if (regio == 4) wild_id = (rand() % 2 == 0) ? 9 : 7; 
        
        int lvl = team[activeIdx].lvl + ((rand() % 3) - 1);
        if (lvl < 1) lvl = 1;
        initKarakter(&vijand_team[0], wild_id, lvl); 
    }

    initGraphicsMode3();
    redrawBattleScene();
}

bool updateBattle() {
    bool stateChanged = (bState != prevState);
    prevState = bState;

    if (bState == 0) { 
        if (stateChanged) drawTextBox(bBoss ? "ENEMY TEAM APPROACHES!" : "A WILD ENEMY", "APPEARED!");
        bTimer--;
        if (bTimer <= 0) bState = 1;
    }
    else if (bState == 1) { 
        if (stateChanged) drawTextBox(NULL, NULL); 
        drawText("FIGHT", 20, 130, (bMenu == 0) ? COLOR_RED : COLOR_WHITE);
        drawText("BAG",   120, 130, (bMenu == 1) ? COLOR_RED : COLOR_WHITE); 
        drawText("TEAM",  20, 145, (bMenu == 2) ? COLOR_RED : COLOR_WHITE);
        drawText("RUN",   120, 145, (bMenu == 3) ? COLOR_RED : COLOR_WHITE);

        if (isKeyJustPressed(KEY_RIGHT)) { bMenu++; if(bMenu > 3) bMenu = 0; }
        if (isKeyJustPressed(KEY_LEFT))  { bMenu--; if(bMenu < 0) bMenu = 3; }
        if (isKeyJustPressed(KEY_DOWN))  { bMenu += 2; if(bMenu > 3) bMenu -= 4; }
        if (isKeyJustPressed(KEY_UP))    { bMenu -= 2; if(bMenu < 0) bMenu += 4; }
        
        if (isKeyJustPressed(KEY_A)) {
            if (bMenu == 0) { bState = 5; bMoveSelect = 0; } 
            else if (bMenu == 1) { bState = 14; bBagCursor = 0; redrawBattleScene(); } 
            else if (bMenu == 2) { bState = 10; teamSelect = activeIdx; redrawBattleScene(); } 
            else if (bMenu == 3) { 
                if (bBoss) { bState = 13; bTimer = 60; }
                else { return true; } 
            }
        }
    }
    else if (bState == 5) { 
        if (stateChanged) drawTextBox("CHOOSE ATTACK:", ""); 
        for(int i = 0; i < 4; i++) {
            uint16_t color = (bMoveSelect == i) ? COLOR_RED : COLOR_WHITE;
            if (team[activeIdx].lvl >= team[activeIdx].moves[i].minLvl) {
                drawText(team[activeIdx].moves[i].naam, 20 + (i%2)*100, 130 + (i/2)*15, color);
            } else {
                drawText("LOCKED", 20 + (i%2)*100, 130 + (i/2)*15, 0x3DEF); 
            }
        }
        
        if (isKeyJustPressed(KEY_RIGHT)) { bMoveSelect++; if (bMoveSelect > 3) bMoveSelect = 0; }
        if (isKeyJustPressed(KEY_LEFT))  { bMoveSelect--; if (bMoveSelect < 0) bMoveSelect = 3; }
        if (isKeyJustPressed(KEY_DOWN))  { bMoveSelect += 2; if (bMoveSelect > 3) bMoveSelect -= 4; }
        if (isKeyJustPressed(KEY_UP))    { bMoveSelect -= 2; if (bMoveSelect < 0) bMoveSelect += 4; }

        if (isKeyJustPressed(KEY_A)) {
            if (team[activeIdx].lvl >= team[activeIdx].moves[bMoveSelect].minLvl && team[activeIdx].moves[bMoveSelect].kracht > 0) {
                bState = 2; bTimer = 40;
            }
        }
        if (isKeyJustPressed(KEY_B)) { bState = 1; } 
    }
    else if (bState == 14) { 
        if (stateChanged) {
            drawUIBox(10, 10, 220, 100);
            drawText("BATTLE BAG:", 15, 15, COLOR_GOLD);
        }
        for(int i = 0; i < 4; i++) { 
            uint16_t color = (bBagCursor == i) ? COLOR_RED : COLOR_WHITE;
            if (itemAantal[i] <= 0) color = 0x3DEF; 
            drawText(itemNamen[i], 30, 30 + (i * 15), color);
            drawText("x", 170, 30 + (i * 15), color);
            drawNumber(itemAantal[i], 180, 30 + (i * 15), color);
        }
        if (isKeyJustPressed(KEY_DOWN)) { bBagCursor = (bBagCursor + 1) % 4; redrawBattleScene(); }
        if (isKeyJustPressed(KEY_UP)) { bBagCursor = (bBagCursor + 3) % 4; redrawBattleScene(); }
        if (isKeyJustPressed(KEY_A)) {
            if (itemAantal[bBagCursor] > 0) {
                if (bBagCursor == 3) { 
                    if (getEmptyPartySlot() == -1) { bState = 6; bTimer = 60; } 
                    else { itemAantal[3]--; bState = 7; bTimer = 60; }
                } else if (itemHeal[bBagCursor] > 0) { 
                    itemAantal[bBagCursor]--; team[activeIdx].hp += itemHeal[bBagCursor];
                    if (team[activeIdx].hp > team[activeIdx].max_hp) team[activeIdx].hp = team[activeIdx].max_hp;
                    bState = 15; bTimer = 40; 
                }
            }
        }
        if (isKeyJustPressed(KEY_B)) { redrawBattleScene(); bState = 1; }
    }
    else if (bState == 15) { 
        if (stateChanged) { redrawBattleScene(); drawTextBox(team[activeIdx].naam, "RECOVERED HP!"); }
        bTimer--;
        if (bTimer <= 0) { bState = 3; bTimer = 60; } 
    }
    else if (bState == 7) { 
        if (stateChanged) { redrawBattleScene(); drawTextBox("YOU THREW A", "CAPTURE NET!"); }
        if (bTimer == 50) { eVis = false; redrawBattleScene(); }
        if (bTimer == 40) { eVis = true; redrawBattleScene(); }
        if (bTimer == 30) { eVis = false; redrawBattleScene(); }
        bTimer--;
        if (bTimer <= 0) {
            eVis = true; redrawBattleScene();
            if (bBoss) { bState = 9; bTimer = 60; } 
            else {
                int catch_chance = 30 + ((vijand_team[activeEnemyIdx].max_hp - vijand_team[activeEnemyIdx].hp) * 70 / vijand_team[activeEnemyIdx].max_hp);
                if ((rand() % 100) < catch_chance) { bState = 8; bTimer = 60; } 
                else { bState = 9; bTimer = 60; }
            }
        }
    }
    else if (bState == 8) { 
        if (stateChanged) drawTextBox("GOTCHA!", "CHARACTER CAUGHT!");
        bTimer--;
        if (bTimer <= 0) { team[getEmptyPartySlot()] = vijand_team[activeEnemyIdx]; return true; }
    }
    else if (bState == 9) { 
        if (stateChanged) drawTextBox(bBoss ? "YOU CAN'T CATCH" : "OH NO!", bBoss ? "A BOSS!" : "IT BROKE FREE!");
        bTimer--;
        if (bTimer <= 0) { bState = 3; bTimer = 40; } 
    }
    else if (bState == 2) { 
        if (stateChanged) drawTextBox(team[activeIdx].naam, team[activeIdx].moves[bMoveSelect].naam);
        if (bTimer == 35) { pOffX = 15; redrawBattleScene(); } 
        if (bTimer == 25) { pOffX = 0; redrawBattleScene(); } 
        if (bTimer == 15) { eVis = false; redrawBattleScene(); } 
        if (bTimer == 10) { eVis = true; redrawBattleScene(); }
        if (bTimer == 5)  { eVis = false; redrawBattleScene(); }
        
        bTimer--;
        if (bTimer <= 0) {
            eVis = true; 
            int damage = calculateDamage(&team[activeIdx], &team[activeIdx].moves[bMoveSelect]);
            vijand_team[activeEnemyIdx].hp -= damage;
            if (vijand_team[activeEnemyIdx].hp < 0) vijand_team[activeEnemyIdx].hp = 0;
            redrawBattleScene(); 
            bState = 3; bTimer = 60;
        }
    }
    else if (bState == 3) { 
        if (vijand_team[activeEnemyIdx].hp <= 0) {
            int nextIdx = -1;
            for(int i = activeEnemyIdx + 1; i < 6; i++) {
                if (vijand_team[i].isGevuld && vijand_team[i].hp > 0) { nextIdx = i; break; }
            }
            if (nextIdx != -1) {
                activeEnemyIdx = nextIdx;
                if (stateChanged) drawTextBox("ENEMY SENT OUT", vijand_team[activeEnemyIdx].naam);
                bTimer--;
                if (bTimer <= 0) { bState = 1; redrawBattleScene(); }
            } else {
                if (stateChanged) drawTextBox("ALL ENEMIES DEFEATED!", "YOU WIN!");
                bTimer--;
                if (bTimer <= 0) { bState = 11; bTimer = 60; } 
            }
        } else {
            if (stateChanged) drawTextBox(vijand_team[activeEnemyIdx].naam, vijand_team[activeEnemyIdx].moves[0].naam);
            if (bTimer == 45) { eOffX = -15; redrawBattleScene(); } 
            if (bTimer == 35) { eOffX = 0; redrawBattleScene(); }
            if (bTimer == 25) { pVis = false; redrawBattleScene(); } 
            if (bTimer == 15) { pVis = true; redrawBattleScene(); }
            if (bTimer == 5)  { pVis = false; redrawBattleScene(); }

            bTimer--;
            if (bTimer <= 0) {
                pVis = true;
                int damage = calculateDamage(&vijand_team[activeEnemyIdx], &vijand_team[activeEnemyIdx].moves[0]);
                team[activeIdx].hp -= damage;
                if (team[activeIdx].hp < 0) team[activeIdx].hp = 0;
                redrawBattleScene();
                bState = 4; bTimer = 60;
            }
        }
    }
    else if (bState == 4) { 
        if (team[activeIdx].hp <= 0) {
            if (stateChanged) { drawTextBox("YOU FAINTED...", ""); pVis = false; redrawBattleScene(); }
            bTimer--;
            if (bTimer <= 0) {
                if (checkAliveTeam()) { bState = 10; teamSelect = 0; } 
                else { drawTextBox("ALL HEROES FAINTED!", "GAME OVER..."); healWholeTeam(); return true; }
            }
        } else { bState = 1; }
    }
    else if (bState == 11) { 
        if (stateChanged) drawTextBox(team[activeIdx].naam, "GAINED XP & ITEMS!");
        bTimer--;
        if (bTimer <= 0) {
            team[activeIdx].xp += (vijand_team[activeEnemyIdx].lvl * 20);
            if (team[activeIdx].xp >= team[activeIdx].xp_nodig) {
                team[activeIdx].lvl++;
                team[activeIdx].xp = 0;
                team[activeIdx].xp_nodig = 50 + (team[activeIdx].lvl * 15);
                team[activeIdx].max_hp += 10;
                team[activeIdx].hp = team[activeIdx].max_hp;
                
                extern void checkEvolutie(Karakter* k);
                checkEvolutie(&team[activeIdx]);
            }
            return true; 
        }
    }
    else if (bState == 6) { 
        if (stateChanged) drawTextBox("YOUR PARTY", "IS FULL!");
        bTimer--;
        if (bTimer <= 0) { bState = 1; }
    }
    else if (bState == 10) { 
        if (stateChanged) {
            drawUIBox(10, 10, 220, 100);
            drawText("SELECT FIGHTER:", 15, 15, COLOR_GOLD);
        }
        for(int i = 0; i < 6; i++) {
            if(team[i].isGevuld) {
                uint16_t color = (teamSelect == i) ? COLOR_RED : COLOR_WHITE;
                if(team[i].hp <= 0) color = 0x3DEF; 
                drawText(team[i].naam, 30, 30 + (i * 12), color);
                drawNumber(team[i].hp, 150, 30 + (i * 12), color);
            }
        }
        if (isKeyJustPressed(KEY_DOWN)) do { teamSelect = (teamSelect + 1) % 6; } while(!team[teamSelect].isGevuld);
        if (isKeyJustPressed(KEY_UP)) do { teamSelect = (teamSelect - 1 + 6) % 6; } while(!team[teamSelect].isGevuld);
        if (isKeyJustPressed(KEY_A)) {
            if (team[teamSelect].hp > 0 && teamSelect != activeIdx) {
                activeIdx = teamSelect; pVis = true; redrawBattleScene(); bState = 12; bTimer = 40;
            }
        }
        if (isKeyJustPressed(KEY_B)) {
            if (team[activeIdx].hp > 0) { redrawBattleScene(); bState = 1; }
        }
    }
    else if (bState == 12) { 
        if (stateChanged) drawTextBox("GO!", team[activeIdx].naam);
        bTimer--;
        if (bTimer <= 0) bState = 1;
    }
    else if (bState == 13) {
        if (stateChanged) drawTextBox("YOU CAN'T RUN", "FROM A BOSS!");
        bTimer--;
        if (bTimer <= 0) { bState = 1; }
    }

    return false;
}