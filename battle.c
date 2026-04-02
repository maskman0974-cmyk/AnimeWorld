#include "gamedata.h"
#include "sound.h"
#include <stdlib.h>
#include <stdio.h> // Voor sprintf
#include <string.h>

#include "battle_bg.h"
#include "Zoro_card.h"
#include "LuffyG5_card.h"
#include "Shanks_card.h"
#include "Mvegeta_card.h"

#define REG_KEYINPUT *(volatile uint16_t*)0x04000130

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

// V2.8 PRO FIX: Kleur Palette voor Type Guide
#define TYPE_COL_FYS 0x3DEF // Grijs/Fysiek
#define TYPE_COL_KI  0x001F // Rood/Ki
#define TYPE_COL_CHA 0x7C00 // Blauw/Chakra
#define TYPE_COL_HAK 0x401F // Paars/Haki
#define TYPE_COL_DEV 0x03FF // Goud/DevilFruit

extern void waitVBlank(); 
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
extern Karakter pcBox[30];
extern int activeEnemyIdx;
extern char* itemNamen[];
extern int itemHeal[];
extern int itemAantal[];

extern bool checkEvolutie(Karakter* k);
extern void checkNewMoves(Karakter* k);
extern int getEmptyPcSlot(void);
extern int getEmptyPartySlot(void);

int bState = 0; 
int prevState = -1; 
int bTimer = 0; 
bool bBoss = false;
int bMenu = 0;       
int bMoveSelect = 0; 
int teamSelect = 0; 
int bBagCursor = 0; 
bool justEvolved = false;

// V2.8 PRO FIX: Global structs voor pre-calculation van levels
typedef struct {
    int levels_to_gain;
    int max_hp_gain;
    int moves_learned_count;
    char moves_names[4][20];
    int moves_kracht[4];
    int moves_minLvl[4];
    int moves_effect[4];
    int moves_element[4];
    bool evolution_triggered;
    char evolution_newname[20];
    int evolution_hpgain;
    int final_xp_val;
    int final_xp_nodig;
} PendingStats;

PendingStats teamPending[6]; // Slaat uitkomsten op voor iedereen
int xp_pool_accumulated = 0; // Telt XP op tijdens het gevecht
int xp_to_give = 0; // Bevat shared_xp voor de actieve held's animatie
int target_xp = 0; // Voor de animatie

int pOffX = 0; 
int eOffX = 0;
bool pVis = true;
bool eVis = true;

int typeModifierText = 0; 
bool passiveTriggered = false;

// --- GFX: Kleuren Guide ---
char* typeAfk[5] = {"[FYS]", "[ KI]", "[CHA]", "[HAK]", "[DEV]"};

uint16_t getTypeColor(int typeID) {
    if(typeID == 1) return TYPE_COL_KI;
    else if(typeID == 2) return TYPE_COL_CHA;
    else if(typeID == 3) return TYPE_COL_HAK;
    else if(typeID == 4) return TYPE_COL_DEV;
    return TYPE_COL_FYS;
}

int getBaseType(int char_id) {
    if (char_id == 3 || char_id == 5) return 1; // Ki (Goku, Vegeta)
    if (char_id == 4 || char_id == 7 || char_id == 8 || char_id == 9 || char_id == 10) return 2; // Chakra
    if (char_id == 1 || char_id == 6) return 3; // Haki (Zoro, Shanks)
    if (char_id == 0 || char_id == 2) return 4; // DF (Luffy, Kaido)
    return 0; // Fysiek (Standaard)
}
// ----------------------------

void drawRect(int x, int y, int w, int h, uint16_t color) {
    for(int r = 0; r < h; r++) for(int c = 0; c < w; c++) drawPixel(x + c, y + r, color);
}

void drawBitmapSprite(int x, int y, int width, int height, const uint16_t* data) {
    uint16_t transColor = data[0]; 
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            uint16_t color = data[r * width + c];
            if (color != transColor && color != 0x7C1F && color != 0x0000) drawPixel(x + c, y + r, color);
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
    drawUIBox(6, 8, 118, 34); 
    drawText(vijand_team[activeEnemyIdx].naam, 10, 12, COLOR_WHITE);
    // V2.8 FIX: Typeguide is nu kleurrijk direct naast de naam!
    int eType = getBaseType(vijand_team[activeEnemyIdx].char_id);
    drawText(typeAfk[eType], 70, 12, getTypeColor(eType));
    drawText("LVL", 90, 12, COLOR_GOLD); 
    drawNumber(vijand_team[activeEnemyIdx].lvl, 108, 12, COLOR_GOLD);
    
    uint16_t vColor = BAR_GREEN;
    if (vijand_team[activeEnemyIdx].hp < vijand_team[activeEnemyIdx].max_hp / 2) vColor = BAR_YELLOW;
    if (vijand_team[activeEnemyIdx].hp < vijand_team[activeEnemyIdx].max_hp / 5) vColor = BAR_RED;
    drawBar(12, 25, 100, 5, vijand_team[activeEnemyIdx].hp, vijand_team[activeEnemyIdx].max_hp, vColor);
    
    drawUIBox(116, 68, 120, 42); 
    drawText(team[activeIdx].naam, 120, 72, COLOR_WHITE);
    // V2.8 FIX: Typeguide is nu kleurrijk direct naast de naam!
    int pType = getBaseType(team[activeIdx].char_id);
    drawText(typeAfk[pType], 165, 72, getTypeColor(pType));
    drawText("LVL", 195, 72, COLOR_GOLD); 
    drawNumber(team[activeIdx].lvl, 215, 72, COLOR_GOLD);
    
    uint16_t pColor = BAR_GREEN;
    if (team[activeIdx].hp < team[activeIdx].max_hp / 2) pColor = BAR_YELLOW;
    if (team[activeIdx].hp < team[activeIdx].max_hp / 5) pColor = BAR_RED;
    drawBar(120, 85, 105, 5, team[activeIdx].hp, team[activeIdx].max_hp, pColor);
    drawBar(120, 94, 105, 2, team[activeIdx].xp, team[activeIdx].xp_nodig, BAR_BLUE);
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
    waitVBlank(); 
    *(volatile uint32_t*)0x040000D4 = (uint32_t)battle_bgBitmap;
    *(volatile uint32_t*)0x040000D8 = (uint32_t)0x06000000;
    *(volatile uint32_t*)0x040000DC = 0x80000000 | 0x04000000 | 19200; 

    // V2.8 GFX: Tijdens gevecht tekenen we *alle sprites* onmiddellijk, nul flicker!
    if (pVis) {
        if (team[activeIdx].char_id == 0 && team[activeIdx].status == 3) {
            const uint16_t* g5_kaart = (const uint16_t*)LuffyG5_cardBitmap;
            for(int y = 0; y < 80; y++) {
                for(int x = 0; x < 64; x++) {
                    drawPixel(24 + pOffX + x, 40 + y, g5_kaart[y * 64 + x]);
                }
            }
        } else {
            drawBitmapSprite(24 + pOffX, 56, 64, 64, team[activeIdx].battle_back_bitmap);
        }
    }

    if (eVis) drawBitmapSprite(150 + eOffX, 16, 64, 64, vijand_team[activeEnemyIdx].battle_front_bitmap);

    drawVFX(); 

    if (bState != 10 && bState != 12 && bState != 14) {
        drawBattleUI();
    }

    if (bState == 12 && bTimer > 15) { 
        const uint16_t* summonCard = NULL;
        if (team[activeIdx].char_id == 1) summonCard = (const uint16_t*)Zoro_cardBitmap;
        
        if (summonCard != NULL && team[activeIdx].status < 3) {
            for(int y = 0; y < 80; y++) {
                for(int x = 0; x < 64; x++) {
                    drawPixel(88 + x, 15 + y, summonCard[y * 64 + x]);
                }
            }
        }
    }

    if (bState == 2 && bTimer > 15 && bTimer < 35) {
        const uint16_t* attackCard = NULL;
        if (team[activeIdx].char_id == 1) attackCard = (const uint16_t*)Zoro_cardBitmap;
        if (team[activeIdx].char_id == 0 && team[activeIdx].status == 3) attackCard = (const uint16_t*)LuffyG5_cardBitmap;

        if (attackCard != NULL) {
            for(int y = 0; y < 80; y++) {
                for(int x = 0; x < 64; x++) {
                    drawPixel(88 + x, 15 + y, attackCard[y * 64 + x]);
                }
            }
        }
    }
}

int calculateDamage(Karakter* attacker, Karakter* defender, Move* m) {
    int dmg = (m->kracht * attacker->lvl) / 5 + (attacker->lvl * 2);
    
    int atkType = m->effectChance; 
    int defType = getBaseType(defender->char_id);
    typeModifierText = 0;

    if (atkType > 0 && defType > 0) {
        if ((atkType == 1 && defType == 4) || (atkType == 4 && defType == 2) || 
            (atkType == 2 && defType == 3) || (atkType == 3 && defType == 1)) {
            dmg = (dmg * 15) / 10; 
            typeModifierText = 1; 
        }
        else if ((atkType == 4 && defType == 1) || (atkType == 2 && defType == 4) || 
                 (atkType == 3 && defType == 2) || (atkType == 1 && defType == 3)) {
            dmg = (dmg * 5) / 10; 
            typeModifierText = 2; 
        }
    }

    if ((attacker->char_id == 3 || attacker->char_id == 5) && attacker->hp < (attacker->max_hp / 3)) {
        dmg = (dmg * 15) / 10;
        passiveTriggered = true;
    }
    if ((attacker->char_id == 1 || attacker->char_id == 6) && (rand() % 100) < 20) {
        dmg = dmg * 2;
        passiveTriggered = true;
    }

    return dmg;
}

void startBattle(bool isStory) {
    bBoss = isStory; bState = 0; prevState = -1; bTimer = 60; bMenu = 0; bMoveSelect = 0; teamSelect = 0; bBagCursor = 0;
    pOffX = 0; eOffX = 0; pVis = true; eVis = true;
    justEvolved = false;

    // V2.8: Reset deelname voor verdeling XP
    for(int i=0; i<6; i++) { participated[i] = false; teamPending[i].levels_to_gain = 0; }
    participated[activeIdx] = true;
    xp_pool_accumulated = 0;

    activeEnemyIdx = 0;
    for(int i=0; i<6; i++) vijand_team[i].isGevuld = false;

    if (isStory) {
        int stage = regio_stage[regio];
        int baseLvl = 3 + (regio * 10) + (stage * 2); 
        for(int i = 0; i < stage; i++) {
            int random_minion_id = rand() % 11; 
            initKarakter(&vijand_team[i], random_minion_id, baseLvl + i); 
        }
        int lastSlot = stage - 1;
        if (lastSlot > 5) lastSlot = 5;
        initKarakter(&vijand_team[lastSlot], interactie_npc, baseLvl + 2); 
    } else {
        int baseLvl = 3 + (regio * 10) + (regio_stage[regio] * 2);
        int wild_id = 1; 
        if (regio == 0)      wild_id = (rand() % 2 == 0) ? 0 : 1; 
        else if (regio == 1) wild_id = (rand() % 2 == 0) ? 0 : 6; 
        else if (regio == 2) wild_id = (rand() % 2 == 0) ? 3 : 5; 
        else if (regio == 3) wild_id = (rand() % 3 == 0) ? 4 : (rand()%2==0?8:10); 
        else if (regio == 4) wild_id = (rand() % 2 == 0) ? 9 : 7; 
        
        int lvl = baseLvl + ((rand() % 3) - 1);
        if (lvl < 1) lvl = 1;
        initKarakter(&vijand_team[0], wild_id, lvl); 
    }

    initGraphicsMode3();
    redrawBattleScene();
}

// V2.8 GFX: Rollback data voor flicker-free HP bars
static int dynamic_old_vijand_hp = 0;
static int dynamic_target_vijand_hp = 0;
static int dynamic_old_active_hp = 0;
static int dynamic_target_active_hp = 0;
static int dynamic_anim_timer = 0;

// V2.8 globals voor pre-calculated victory
static int levelupQueueCharIdx = 0; // Tracks wie we displayen
static int levelupQueuePage = 0; // Tracks move/evolute page

bool updateBattle() {
    bool stateChanged = (bState != prevState);
    prevState = bState;

    if (bState == 0) { 
        if (stateChanged) drawTextBox(bBoss ? "ENEMY TEAM APPROACHES!" : "A WILD ENEMY", "APPEARED!");
        bTimer--;
        if (bTimer <= 0) bState = 1;
    }
    else if (bState == 1) { 
        if (stateChanged) { drawTextBox(NULL, NULL); } 
        drawText("FIGHT", 20, 130, (bMenu == 0) ? COLOR_RED : COLOR_WHITE);
        drawText("BAG",   120, 130, (bMenu == 1) ? COLOR_RED : COLOR_WHITE); 
        drawText("TEAM",  20, 145, (bMenu == 2) ? COLOR_RED : COLOR_WHITE);
        drawText("RUN",   120, 145, (bMenu == 3) ? COLOR_RED : COLOR_WHITE);

        if (isKeyJustPressed(KEY_RIGHT)) { bMenu++; if(bMenu > 3) bMenu = 0; drawTextBox(NULL, NULL); }
        if (isKeyJustPressed(KEY_LEFT))  { bMenu--; if(bMenu < 0) bMenu = 3; drawTextBox(NULL, NULL); }
        if (isKeyJustPressed(KEY_DOWN))  { bMenu += 2; if(bMenu > 3) bMenu -= 4; drawTextBox(NULL, NULL); }
        if (isKeyJustPressed(KEY_UP))    { bMenu -= 2; if(bMenu < 0) bMenu += 4; drawTextBox(NULL, NULL); }
        
        if (isKeyJustPressed(KEY_A)) {
            if (bMenu == 0) { bState = 5; bMoveSelect = 0; } 
            else if (bMenu == 1) { bState = 14; bBagCursor = 0; } 
            else if (bMenu == 2) { bState = 10; teamSelect = activeIdx; redrawBattleScene(); } 
            else if (bMenu == 3) { 
                if (bBoss) { bState = 13; bTimer = 60; redrawBattleScene(); }
                else { return true; } 
            }
        }
    }
    else if (bState == 5) { 
        if (stateChanged) drawTextBox("CHOOSE ATTACK:", ""); 
        for(int i = 0; i < 4; i++) {
            uint16_t color = (bMoveSelect == i) ? COLOR_RED : COLOR_WHITE;
            if (strcmp(team[activeIdx].moves[i].naam, "-") != 0) {
                drawText(team[activeIdx].moves[i].naam, 20 + (i%2)*100, 130 + (i/2)*15, color);
            } else {
                drawText("---", 20 + (i%2)*100, 130 + (i/2)*15, 0x3DEF); 
            }
        }
        if (isKeyJustPressed(KEY_RIGHT)) { bMoveSelect++; if (bMoveSelect > 3) bMoveSelect = 0; drawTextBox("CHOOSE ATTACK:", ""); }
        if (isKeyJustPressed(KEY_LEFT))  { bMoveSelect--; if (bMoveSelect < 0) bMoveSelect = 3; drawTextBox("CHOOSE ATTACK:", ""); }
        if (isKeyJustPressed(KEY_DOWN))  { bMoveSelect += 2; if (bMoveSelect > 3) bMoveSelect -= 4; drawTextBox("CHOOSE ATTACK:", ""); }
        if (isKeyJustPressed(KEY_UP))    { bMoveSelect -= 2; if (bMoveSelect < 0) bMoveSelect += 4; drawTextBox("CHOOSE ATTACK:", ""); }
        if (isKeyJustPressed(KEY_A)) {
            if (strcmp(team[activeIdx].moves[bMoveSelect].naam, "-") != 0) { bState = 2; bTimer = 40; }
        }
        if (isKeyJustPressed(KEY_B)) { drawTextBox(NULL,NULL); bState = 1; } 
    }
    else if (bState == 14) { 
        if (stateChanged) { drawUIBox(10, 10, 220, 100); drawText("BATTLE BAG:", 15, 15, COLOR_GOLD); }
        for(int i = 0; i < 4; i++) { 
            uint16_t color = (bBagCursor == i) ? COLOR_RED : COLOR_WHITE;
            if (itemAantal[i] <= 0) color = 0x3DEF; 
            drawText(itemNamen[i], 30, 30 + (i * 15), color);
            drawText("x", 170, 30 + (i * 15), color); drawNumber(itemAantal[i], 180, 30 + (i * 15), color);
        }
        if (isKeyJustPressed(KEY_DOWN)) { bBagCursor = (bBagCursor + 1) % 4; drawUIBox(10, 10, 220, 100); drawText("BATTLE BAG:", 15, 15, COLOR_GOLD); }
        if (isKeyJustPressed(KEY_UP)) { bBagCursor = (bBagCursor + 3) % 4; drawUIBox(10, 10, 220, 100); drawText("BATTLE BAG:", 15, 15, COLOR_GOLD); }
        if (isKeyJustPressed(KEY_A)) {
            if (itemAantal[bBagCursor] > 0) {
                if (bBagCursor == 3) { 
                    itemAantal[3]--; bState = 7; bTimer = 60; redrawBattleScene();
                } else if (itemHeal[bBagCursor] > 0) { 
                    itemAantal[bBagCursor]--; team[activeIdx].hp += itemHeal[bBagCursor];
                    if (team[activeIdx].hp > team[activeIdx].max_hp) team[activeIdx].hp = team[activeIdx].max_hp;
                    bState = 15; bTimer = 40; redrawBattleScene();
                }
            }
        }
        if (isKeyJustPressed(KEY_B)) { redrawBattleScene(); bState = 1; }
    }
    else if (bState == 15) { 
        if (stateChanged) { drawTextBox(team[activeIdx].naam, "RECOVERED HP!"); }
        bTimer--; if (bTimer <= 0) { bState = 3; bTimer = 60; } 
    }
    else if (bState == 7) { 
        if (stateChanged) { drawTextBox("YOU THREW A", "CAPTURE NET!"); }
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
        if (stateChanged) {
            if (getEmptyPartySlot() != -1) { drawTextBox("GOTCHA! SENT TO", "PARTY!"); team[getEmptyPartySlot()] = vijand_team[activeEnemyIdx]; } 
            else if (getEmptyPcSlot() != -1) { drawTextBox("GOTCHA! SENT TO", "PC BOX!"); pcBox[getEmptyPcSlot()] = vijand_team[activeEnemyIdx]; } 
            else { drawTextBox("PC BOX IS FULL!", "RELEASED..."); }
        }
        bTimer--; if (bTimer <= 0) { return true; }
    }
    else if (bState == 9) { 
        if (stateChanged) drawTextBox(bBoss ? "YOU CAN'T CATCH" : "OH NO!", bBoss ? "A BOSS!" : "IT BROKE FREE!");
        bTimer--; if (bTimer <= 0) { bState = 3; bTimer = 40; } 
    }
    else if (bState == 2) { 
        if (stateChanged) {
            passiveTriggered = false; typeModifierText = 0;
            drawTextBox(team[activeIdx].naam, team[activeIdx].moves[bMoveSelect].naam);
        }
        if (bTimer == 35) { pOffX = 15; redrawBattleScene(); } 
        if (bTimer == 25) { pOffX = 0; redrawBattleScene(); } 
        if (bTimer == 15) { eVis = false; redrawBattleScene(); } 
        if (bTimer == 10) { eVis = true; redrawBattleScene(); }
        if (bTimer == 5)  { eVis = false; redrawBattleScene(); }
        
        bTimer--;
        if (bTimer <= 0) {
            eVis = true; 
            int damage = calculateDamage(&team[activeIdx], &vijand_team[activeEnemyIdx], &team[activeIdx].moves[bMoveSelect]);
            
            // V2.8 GFX: Setup specialized dynamic HP animation
            dynamic_old_vijand_hp = vijand_team[activeEnemyIdx].hp;
            vijand_team[activeEnemyIdx].hp -= damage; // Apply permanently
            if (vijand_team[activeEnemyIdx].hp < 0) vijand_team[activeEnemyIdx].hp = 0;
            dynamic_target_vijand_hp = vijand_team[activeEnemyIdx].hp;
            
            bState = 25; // Transition VState 25 (HP Drop Vijand)
        }
    }
    // V2.8 GFX: Vijand HP animatie (Flicker Free)
    else if (bState == 25) { 
        // specialized drawing updates VRAM location.
        if (dynamic_old_vijand_hp > dynamic_target_vijand_hp) {
            dynamic_anim_timer++;
            if (dynamic_anim_timer > 1) { // dropping HP 
                dynamic_old_vijand_hp--; 
                dynamic_anim_timer = 0;
                
                // FIX FLICKER: NIET redrawBattleScene(), teken ENKEL het balkje en HP numbers!
                int x = 12, y = 25, w = 100, h = 5;
                int max_hp = vijand_team[activeEnemyIdx].max_hp;
                int hp = dynamic_old_vijand_hp;
                
                uint16_t color = BAR_GREEN;
                if (hp < max_hp / 2) color = BAR_YELLOW;
                if (hp < max_hp / 5) color = BAR_RED;
                drawBar(x, y, w, h, hp, max_hp, color);
            }
        } else {
            // Dynamic animation finished, show outcome text
            if (typeModifierText == 1) { bState = 20; bTimer = 80; redrawBattleScene(); } 
            else if (typeModifierText == 2) { bState = 21; bTimer = 80; redrawBattleScene(); } 
            else if (passiveTriggered) { bState = 22; bTimer = 80; redrawBattleScene(); } 
            else { bState = 3; bTimer = 60; redrawBattleScene(); }
        }
    }
    else if (bState == 20) { 
        if (stateChanged) drawTextBox("IT'S SUPER", "EFFECTIVE!");
        bTimer--; if (bTimer <= 0) { bState = 3; bTimer = 60; }
    }
    else if (bState == 21) { 
        if (stateChanged) drawTextBox("IT'S NOT VERY", "EFFECTIVE...");
        bTimer--; if (bTimer <= 0) { bState = 3; bTimer = 60; }
    }
    else if (bState == 22) { 
        if (stateChanged) drawTextBox("PASSIVE ABILITY", "TRIGGERED!");
        bTimer--; if (bTimer <= 0) { bState = 3; bTimer = 60; }
    }
    else if (bState == 3) { 
        if (vijand_team[activeEnemyIdx].hp <= 0) {
            // V2.8 CRASH FIX: ACCUMULATE pool ONCE when defeated
            if (stateChanged) {
                xp_pool_accumulated += (vijand_team[activeEnemyIdx].lvl * 25);
            }

            int nextIdx = -1;
            for(int i = activeEnemyIdx + 1; i < 6; i++) { if (vijand_team[i].isGevuld && vijand_team[i].hp > 0) { nextIdx = i; break; } }
            if (nextIdx != -1) {
                activeEnemyIdx = nextIdx;
                if (stateChanged) drawTextBox("ENEMY SENT OUT", vijand_team[activeEnemyIdx].naam);
                bTimer--; if (bTimer <= 0) { drawTextBox(NULL,NULL); bState = 1; redrawBattleScene(); }
            } else {
                if (stateChanged) drawTextBox("ALL ENEMIES DEFEATED!", "YOU WIN!");
                bTimer--; if (bTimer <= 0) { 
                    
                    // **V2.8 PRO OVERHAUL: THE BIG PRE-CALCULATION BLOCK**
                    // --- TECHNISCHE CRASH FIX: BEREKEN ALLES NU ---
                    int participants = 0;
                    for(int i=0; i<6; i++) { if(participated[i]) participants++; teamPending[i].levels_to_gain = 0; }
                    if (participants == 0) participants = 1; // Failsafe

                    int shared_xp = xp_pool_accumulated / participants;
                    xp_to_give = shared_xp; // Still needed for V11 Active animation
                    
                    // Loop alle deelnemers (actief en bank)
                    for(int i=0; i<6; i++) {
                        if(participated[i] && team[i].hp > 0) {
                            // Pre-calc XP en Level
                            int tempLvl = team[i].lvl;
                            int tempXp = team[i].xp + shared_xp;
                            int tempXpNodig = team[i].xp_nodig;
                            int lvlGained = 0;
                            
                            while(tempXp >= tempXpNodig) {
                                tempXp -= tempXpNodig;
                                tempLvl++;
                                tempXpNodig = 50 + (tempLvl * 15);
                                lvlGained++;
                            }
                            
                            // Sla rekenkundige uitkomsten op
                            teamPending[i].levels_to_gain = lvlGained;
                            teamPending[i].max_hp_gain = lvlGained * 10;
                            teamPending[i].final_xp_val = tempXp;
                            teamPending[i].final_xp_nodig = tempXpNodig;
                            
                            // Check Moves (gebruik een tijdelijk Karakter)
                            teamPending[i].moves_learned_count = 0;
                            if(lvlGained > 0) {
                                Karakter tK; initKarakter(&tK, team[i].char_id, tempLvl);
                                for(int mI=0; mI<4; mI++) {
                                    if(strcmp(tK.moves[mI].naam, "-") != 0) {
                                        bool alreadyHas = false;
                                        for(int mC=0; mC<4; mC++) { if(strcmp(team[i].moves[mC].naam, tK.moves[mI].naam) == 0) alreadyHas = true; }
                                        if(!alreadyHas) {
                                            int learnedIdx = teamPending[i].moves_learned_count;
                                            strcpy(teamPending[i].moves_names[learnedIdx], tK.moves[mI].naam);
                                            teamPending[i].moves_learned_count++;
                                        }
                                    }
                                }
                            }
                            
                            // Check Evolutie (gebruik een TEMP character)
                            teamPending[i].evolution_triggered = false;
                            if(lvlGained > 0) {
                                Karakter tE; tE = team[i]; // Kopie
                                tE.lvl = tempLvl; // Set new lvl
                                // Gebruik de gamedata checkEvolutie (modificeert tE, niet team[i])
                                if(checkEvolutie(&tE)) { 
                                    teamPending[i].evolution_triggered = true;
                                    strcpy(teamPending[i].evolution_newname, tE.naam);
                                }
                            }
                            
                            // --- PAS PERMANENTE WIJZIGINGEN NU TOE ---
                            // Arithmetic base Level-up stats
                            team[i].lvl = tempLvl;
                            team[i].xp = tempXp;
                            team[i].xp_nodig = tempXpNodig;
                            team[i].max_hp += teamPending[i].max_hp_gain;
                            team[i].hp = team[i].max_hp; // Heal on level up

                            // Leer Moves (apply permanently)
                            checkNewMoves(&team[i]); 
                            
                            // Evolueer (Permanently modify team[i])
                            if(teamPending[i].evolution_triggered) {
                                checkEvolutie(&team[i]); 
                            }
                        }
                    }
                    
                    // Setup visual XP animation voor ACTIEVE held
                    // Visual XP bar moet OldLvl OldXp -> NewXp animeren.
                    int activeOldXp = team[activeIdx].xp - shared_xp; // This might be negative visually
                    if(activeOldXp < 0) activeOldXp = 0; 
                    target_xp = team[activeIdx].xp; // Final XP target

                    // bState = 11: Setup visual state
                    dynamic_anim_timer = activeOldXp; // We hergebruiken dynamic_anim_timer als visualXp counter
                    bState = 11; bTimer = 0; // State changed handles initialization
                    xp_pool_accumulated = 0; // Reset pool
                    levelupQueueCharIdx = 0; // Reset display queue
                    
                    // V2.8 UX FIX: Skip XP animatie als levels gained, ga direct naar goudgele "Has reached Lvl X"
                    if(teamPending[activeIdx].levels_to_gain > 0) {
                        levelupQueueCharIdx = 0; // Start bij Active
                        bState = 16; stateChanged = true;
                    } else {
                        // Alleen XP animatie nodig
                        stateChanged = true;
                    }
                } 
            }
        } else {
            if (stateChanged) {
                passiveTriggered = false; typeModifierText = 0;
                drawTextBox(vijand_team[activeEnemyIdx].naam, vijand_team[activeEnemyIdx].moves[0].naam);
            }
            if (bTimer == 45) { eOffX = -15; redrawBattleScene(); } 
            if (bTimer == 35) { eOffX = 0; redrawBattleScene(); }
            if (bTimer == 25) { pVis = false; redrawBattleScene(); } 
            if (bTimer == 15) { pVis = true; redrawBattleScene(); }
            if (bTimer == 5)  { pVis = false; redrawBattleScene(); }

            bTimer--;
            if (bTimer <= 0) {
                pVis = true;
                int damage = calculateDamage(&vijand_team[activeEnemyIdx], &team[activeIdx], &vijand_team[activeEnemyIdx].moves[0]);
                
                // V2.8 GFX: Setup dynamic HP animation voor Active held (Flicker Free)
                dynamic_old_active_hp = team[activeIdx].hp;
                team[activeIdx].hp -= damage;
                if (team[activeIdx].hp < 0) team[activeIdx].hp = 0;
                dynamic_target_active_hp = team[activeIdx].hp;
                
                bState = 35; // Transition VState 35 (HP Drop Active)
            }
        }
    }
    // V2.8 GFX: Active Held HP animatie (Flicker Free)
    else if (bState == 35) { 
        if (dynamic_old_active_hp > dynamic_target_active_hp) {
            dynamic_anim_timer++;
            if (dynamic_anim_timer > 1) { // dropping HP 
                dynamic_old_active_hp--; 
                dynamic_anim_timer = 0;
                
                // FIX FLICKER: NIET redrawBattleScene(), teken ENKEL het balkje en HP numbers!
                int x = 120, y = 85, w = 105, h = 5; // Active held bar coördinaten
                int max_hp = team[activeIdx].max_hp;
                int hp = dynamic_old_active_hp;
                
                uint16_t color = BAR_GREEN;
                if (hp < max_hp / 2) color = BAR_YELLOW;
                if (hp < max_hp / 5) color = BAR_RED;
                drawBar(x, y, w, h, hp, max_hp, color);
            }
        } else {
            // Dynamic animation finished
            if (typeModifierText == 1) { bState = 30; bTimer = 80; redrawBattleScene(); } 
            else if (typeModifierText == 2) { bState = 31; bTimer = 80; redrawBattleScene(); } 
            else { bState = 4; bTimer = 60; redrawBattleScene(); }
        }
    }
    else if (bState == 30) { 
        if (stateChanged) drawTextBox("IT'S SUPER", "EFFECTIVE!");
        bTimer--; if (bTimer <= 0) { bState = 4; bTimer = 60; }
    }
    else if (bState == 31) { 
        if (stateChanged) drawTextBox("IT'S NOT VERY", "EFFECTIVE...");
        bTimer--; if (bTimer <= 0) { bState = 4; bTimer = 60; }
    }
    else if (bState == 4) { 
        if (stateChanged) {
            if (team[activeIdx].hp > 0 && team[activeIdx].char_id == 4 && team[activeIdx].hp < team[activeIdx].max_hp) {
                team[activeIdx].hp += 10;
                if (team[activeIdx].hp > team[activeIdx].max_hp) team[activeIdx].hp = team[activeIdx].max_hp;
                drawTextBox(team[activeIdx].naam, "RECOVERED HP!");
                redrawBattleScene();
                bTimer = 40; 
            } else {
                bTimer = 0; 
            }
        }

        bTimer--;
        if (bTimer <= 0) {
            if (team[activeIdx].hp <= 0) {
                drawTextBox("YOU FAINTED...", ""); pVis = false; redrawBattleScene();
                if (checkAliveTeam()) { bState = 10; teamSelect = 0; } 
                else { drawTextBox("ALL HEROES FAINTED!", "GAME OVER..."); healWholeTeam(); return true; }
            } else { bState = 1; }
        }
    }
    
    // VState 11: XP Animatie (Alleen XP, geen levels meer berekenen!)
    else if (bState == 11) { 
        if (stateChanged) drawTextBox("TEAM GAINED XP!", "");
        
        // We animeren visual data! `dynamic_anim_timer` bevat visualActiveXp
        if (dynamic_anim_timer < target_xp) {
            dynamic_anim_timer += 2; 
            if (dynamic_anim_timer > target_xp) dynamic_anim_timer = target_xp;
            
            // FIX FLICKER: NIET redrawBattleScene(), teken ENKEL het blauwe balkje!
            // Gebruik visual data coördinaten van Active held
            drawBar(120, 94, 105, 2, dynamic_anim_timer, team[activeIdx].xp_nodig, BAR_BLUE);
            
        } else {
            // Wacht NIET op A.
            return true; 
        }
    }

    // VState 16: Level Up Goud Schermen Queue UX Overhaul
    else if (bState == 16) { 
        if(stateChanged) {
            redrawBattleScene(); 
            // V2.8 UX FIX: Goudgele duidelijke tekst!
            char l2[32];
            sprintf(l2, "HAS REACHED LVL %d!", team[levelupQueueCharIdx].lvl); 
            drawTextBox(team[levelupQueueCharIdx].naam, l2);
        }
        
        if (isKeyJustPressed(KEY_A)) {
            // Check pending data (Moves, Evolutie)
            justEvolved = false;
            
            // Final transition back to VState 1.
            return true; 
        }
    }

    else if (bState == 10) { 
        if (stateChanged) {
            drawUIBox(10, 10, 220, 100); drawText("SELECT FIGHTER:", 15, 15, COLOR_GOLD);
        }
        for(int i = 0; i < 6; i++) {
            if(team[i].isGevuld) {
                uint16_t color = (teamSelect == i) ? COLOR_RED : COLOR_WHITE; if(team[i].hp <= 0) color = 0x3DEF; 
                drawText(team[i].naam, 30, 30 + (i * 12), color); drawNumber(team[i].hp, 150, 30 + (i * 12), color);
            }
        }
        if (isKeyJustPressed(KEY_DOWN)) do { teamSelect = (teamSelect + 1) % 6; } while(!team[teamSelect].isGevuld);
        if (isKeyJustPressed(KEY_UP)) do { teamSelect = (teamSelect - 1 + 6) % 6; } while(!team[teamSelect].isGevuld);
        if (isKeyJustPressed(KEY_A)) {
            if (team[teamSelect].hp > 0 && teamSelect != activeIdx) {
                activeIdx = teamSelect; pVis = true; bState = 12; bTimer = 40; redrawBattleScene();
            }
        }
        if (isKeyJustPressed(KEY_B)) { if (team[activeIdx].hp > 0) { redrawBattleScene(); bState = 1; } }
    }
    else if (bState == 12) { 
        if (stateChanged) { 
            drawTextBox("GO!", team[activeIdx].naam); 
            // Markeer dat deze speler nu ook recht heeft op de XP
            participated[activeIdx] = true; 
        }
        bTimer--; if (bTimer <= 0) bState = 1;
    }
    else if (bState == 13) {
        if (stateChanged) drawTextBox("YOU CAN'T RUN", "FROM A BOSS!");
        bTimer--; if (bTimer <= 0) { bState = 1; }
    }

    return false;
}
