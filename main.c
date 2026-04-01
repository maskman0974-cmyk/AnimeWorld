#include "gamedata.h"
#include "sound.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 

#include "bg.h"
#include "marineford.h"
#include "alabasta.h"
#include "konoha.h"
#include "shibuya.h"
#include "luffy.h" 

// --- V2.6 TRADING CARD HEADERS ---
#include "LuffyG5_card.h"
#include "Zoro_card.h"
#include "Shanks_card.h"
#include "Mvegeta_card.h"
// ---------------------------------

#define REG_DISPCNT  *(volatile uint16_t*)0x04000000
#define REG_VCOUNT   *(volatile uint16_t*)0x04000006
#define REG_BG0CNT   *(volatile uint16_t*)0x04000008
#define REG_BG0HOFS  *(volatile uint16_t*)0x04000010 
#define REG_BG0VOFS  *(volatile uint16_t*)0x04000012 
#define REG_KEYINPUT *(volatile uint16_t*)0x04000130

#define KEY_A      (1<<0)
#define KEY_B      (1<<1)
#define KEY_START  (1<<3)
#define KEY_RIGHT  (1<<4)
#define KEY_LEFT   (1<<5)
#define KEY_UP     (1<<6)
#define KEY_DOWN   (1<<7)

#define COLOR_BLACK 0x0000
#define COLOR_GOLD  0x03FF
#define COLOR_WHITE 0x7FFF
#define COLOR_RED   0x001F
#define COLOR_GREEN 0x03E0
#define UI_BG       0x4A69

extern void fillScreen(uint16_t color);
extern void drawText(char* text, int x, int y, uint16_t color);
extern void drawNumber(int num, int x, int y, uint16_t color);
extern void drawTextBox(char* line1, char* line2);
extern void initOAM(void);
extern void setSpriteAttribute(int id, int x, int y, int pal, int tile, int priority, int size);
extern void startBattle(bool isStory); 
extern bool updateBattle(void);
extern void drawBitmapSprite(int x, int y, int width, int height, const uint16_t* data);
extern int getEmptyPartySlot(void);
extern int getEmptyPcSlot(void);

// --- V2.6 TRADING CARD RENDER FUNCTIE ---
void drawTradingCard(const uint16_t* bitmap) {
    volatile uint16_t* vram = (volatile uint16_t*)0x06000000;
    for(int y = 0; y < 80; y++) {
        for(int x = 0; x < 64; x++) {
            // Centreer: X=88. Y=10 (zodat hij mooi boven de dialoogbox staat)
            vram[(y + 10) * 240 + (x + 88)] = bitmap[y * 64 + x];
        }
    }
}
// ----------------------------------------

void drawRectMain(int x, int y, int w, int h, uint16_t color) {
    volatile uint16_t* vram = (volatile uint16_t*)0x06000000;
    for(int r = 0; r < h; r++) {
        for(int c = 0; c < w; c++) {
            vram[(y + r) * 240 + (x + c)] = color;
        }
    }
}

void drawUIBoxMain(int x, int y, int w, int h) {
    drawRectMain(x, y, w, h, UI_BG); 
    drawRectMain(x, y, w, 2, 0x7FFF); 
    drawRectMain(x, y + h - 2, w, 2, 0x7FFF); 
    drawRectMain(x, y, 2, h, 0x7FFF); 
    drawRectMain(x + w - 2, y, 2, h, 0x7FFF); 
}

extern char* itemNamen[];
extern int itemHeal[];
extern int itemAantal[];
extern Karakter vijand_team[6]; 
extern Karakter pcBox[30];
extern int activeIdx;
extern bool bBoss;

int berries = 1000; 
static uint16_t current_keys = 0;
static uint16_t previous_keys = 0;
extern bool justEvolved;

// V2.5 NIEUWE GLOBALS VOOR PUZZELS & ENDGAME
bool gameCleared = false;
bool hasDragonBall = false;
bool hasMasterNet = false;
int colosseumWave = 1;
int prevRegio = 0;

void updateInput() { 
    previous_keys = current_keys; 
    current_keys = ~REG_KEYINPUT & 0x03FF; 
}
bool isKeyJustPressed(uint16_t key) { return (current_keys & key) && !(previous_keys & key); }
bool isKeyHeld(uint16_t key) { return (current_keys & key); }
void waitVBlank() { while(REG_VCOUNT >= 160); while(REG_VCOUNT < 160); }

void copy16(const void* src, volatile uint16_t* dest, int sizeInBytes) {
    const uint16_t* src16 = (const uint16_t*)src;
    int numberOfShorts = sizeInBytes / 2;
    for(int i = 0; i < numberOfShorts; i++) { dest[i] = src16[i]; }
}

void loadMap() {
    REG_BG0CNT = 0x1F00; 
    if (regio == 0) {
        copy16(bgPal, (volatile uint16_t*)0x05000000, bgPalLen);
        copy16(bgTiles, (volatile uint16_t*)0x06000000, bgTilesLen);
        copy16(bgMap, (volatile uint16_t*)0x0600F800, bgMapLen);
    } else if (regio == 1) {
        copy16(marinefordPal, (volatile uint16_t*)0x05000000, marinefordPalLen);
        copy16(marinefordTiles, (volatile uint16_t*)0x06000000, marinefordTilesLen);
        copy16(marinefordMap, (volatile uint16_t*)0x0600F800, marinefordMapLen);
    } else if (regio == 2) {
        copy16(alabastaPal, (volatile uint16_t*)0x05000000, alabastaPalLen);
        copy16(alabastaTiles, (volatile uint16_t*)0x06000000, alabastaTilesLen);
        copy16(alabastaMap, (volatile uint16_t*)0x0600F800, alabastaMapLen);
    } else if (regio == 3) {
        copy16(konohaPal, (volatile uint16_t*)0x05000000, konohaPalLen);
        copy16(konohaTiles, (volatile uint16_t*)0x06000000, konohaTilesLen);
        copy16(konohaMap, (volatile uint16_t*)0x0600F800, konohaMapLen);
    } else if (regio == 4) {
        copy16(shibuyaPal, (volatile uint16_t*)0x05000000, shibuyaPalLen);
        copy16(shibuyaTiles, (volatile uint16_t*)0x06000000, shibuyaTilesLen);
        copy16(shibuyaMap, (volatile uint16_t*)0x0600F800, shibuyaMapLen);
    }
    copy16(luffyPal, (volatile uint16_t*)0x05000200, luffyPalLen);
    copy16(luffyTiles, (volatile uint16_t*)0x06010000, luffyTilesLen);
}

int state = 0;
int worldX = 120, worldY = 80;
static bool stateChanged = true;
int menuCursor = 0, partyCursor = 0, dexCursor = 0, shopCursor = 0, bagCursor = 0, pcCursor = 0;
int animTimer = 0;

int regio_stage[6] = {1, 1, 1, 1, 1, 1}; 
int side_quest_stage[6] = {0, 0, 0, 0, 0, 0}; 
int interactie_npc = 0; 
int dialoguePage = 0; 

int main() {
    initSound(); 
    initTeam(); 
    initOAM();

    while(1) {
        waitVBlank(); 
        updateInput();

        if (state == 0) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; 
                drawRectMain(0, 0, 240, 160, 0x1084); 
                drawUIBoxMain(40, 30, 160, 80);
                drawText("ANIME WORLD", 80, 50, COLOR_GOLD); 
                drawText("V2.6 TRADING CARDS", 60, 70, COLOR_WHITE); 
                drawText("PRESS START", 80, 130, COLOR_WHITE); 
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_START)) { srand(REG_VCOUNT); state = 1; stateChanged = true; }
        } 
        else if (state == 1) { 
            if (stateChanged) { REG_DISPCNT = 0x1140; loadMap(); stateChanged = false; }
            if (isKeyJustPressed(KEY_START)) { state = 7; menuCursor = 0; stateChanged = true; continue; }

            if (isKeyHeld(KEY_RIGHT)) { worldX += 2; }
            if (isKeyHeld(KEY_LEFT)) { worldX -= 2; }
            if (isKeyHeld(KEY_UP)) { worldY -= 2; }
            if (isKeyHeld(KEY_DOWN)) { worldY += 2; }

            if (worldX < 10) { worldX = 10; }
            if (worldY < 20) { worldY = 20; }
            if (worldY > 230) { worldY = 230; }
            
            if (worldX > 250) { 
                if (regio_stage[regio] > 5 && regio < 4) {
                    regio++; worldX = 20; stateChanged = true; initOAM(); 
                } else if (regio_stage[regio] <= 5) {
                    worldX = 240; state = 89; stateChanged = true; 
                } else { worldX = 250; }
            }
            else if (worldX < 15 && regio > 0) { regio--; worldX = 240; stateChanged = true; initOAM(); }

            if (isKeyHeld(KEY_LEFT) || isKeyHeld(KEY_RIGHT) || isKeyHeld(KEY_UP) || isKeyHeld(KEY_DOWN)) {
                if ((rand() % 1000) > 980) { state = 2; stateChanged = true; startBattle(false); }
            }

            int camX = worldX - 112; 
            int camY = worldY - 72;  
            if (camX < 0) { camX = 0; }
            if (camY < 0) { camY = 0; }
            if (camX > 16) { camX = 16; } 
            if (camY > 96) { camY = 96; } 
            
            REG_BG0HOFS = camX; 
            REG_BG0VOFS = camY;
            
            int playerScreenX = worldX - camX;
            int playerScreenY = worldY - camY;
            setSpriteAttribute(0, playerScreenX, playerScreenY, 0, 0, 0, 1); 

            // VASTE BOSS
            int npcX = 200; 
            int npcY = 80;
            if (regio_stage[regio] <= 5) {
                int bx = npcX - camX; 
                int by = npcY - camY;
                if (bx > -16 && bx < 240 && by > -16 && by < 160) {
                    setSpriteAttribute(1, bx, by, 0, 0, 0, 1);
                } else { setSpriteAttribute(1, 0, 160, 0, 0, 0, 1); }
            } else { setSpriteAttribute(1, 0, 160, 0, 0, 0, 1); }

            // SIDE QUEST NPC
            int questX = 80;
            int questY = 140;
            if (side_quest_stage[regio] == 0) {
                int qx = questX - camX; 
                int qy = questY - camY;
                if (qx > -16 && qx < 240 && qy > -16 && qy < 160) {
                    setSpriteAttribute(3, qx, qy, 0, 0, 0, 1);
                } else { setSpriteAttribute(3, 0, 160, 0, 0, 0, 1); }
            } else { setSpriteAttribute(3, 0, 160, 0, 0, 0, 1); }

            // NURSE JOY
            int nx = 40 - camX; int ny = 40 - camY;
            if (nx > -16 && nx < 240 && ny > -16 && ny < 160) {
                setSpriteAttribute(2, nx, ny, 0, 0, 0, 1);
            } else { setSpriteAttribute(2, 0, 160, 0, 0, 0, 1); }

            if (isKeyJustPressed(KEY_A)) {
                if (regio == 0 && !hasMasterNet && abs(worldX - 20) < 30 && abs(worldY - 200) < 30) {
                    hasMasterNet = true; itemAantal[3] += 10; state = 101; stateChanged = true;
                }
                else if (regio == 2 && !hasDragonBall && abs(worldX - 20) < 30 && abs(worldY - 40) < 30) {
                    hasDragonBall = true; state = 101; stateChanged = true;
                }
                else if (abs(worldX-40) < 30 && abs(worldY-40) < 30) { state = 9; shopCursor = 0; stateChanged = true; }
                else if (side_quest_stage[regio] == 0 && abs(worldX - questX) < 30 && abs(worldY - questY) < 30) {
                    dialoguePage = 0; state = 80; stateChanged = true;
                }
                else if (regio_stage[regio] <= 5 && abs(worldX - npcX) < 30 && abs(worldY - npcY) < 30) {
                    if (regio == 2 && regio_stage[regio] >= 3 && !hasDragonBall) {
                        state = 102; stateChanged = true; 
                    } else {
                        int stage = regio_stage[regio];
                        if (regio == 0) {
                            if (stage == 1) interactie_npc = 1; else if (stage == 2) interactie_npc = 0; 
                            else if (stage == 3) interactie_npc = 2; else if (stage == 4) interactie_npc = 6; else interactie_npc = 2;
                        } else if (regio == 1) { 
                            if (stage == 1) interactie_npc = 1; else if (stage == 2) interactie_npc = 4; 
                            else if (stage == 3) interactie_npc = 9; else if (stage == 4) interactie_npc = 6; else interactie_npc = 7;
                        } else if (regio == 2) { 
                            if (stage == 1) interactie_npc = 3; else if (stage == 2) interactie_npc = 8; 
                            else if (stage == 3) interactie_npc = 5; else if (stage == 4) interactie_npc = 3; else interactie_npc = 5;
                        } else if (regio == 3) { 
                            if (stage == 1) interactie_npc = 10; else if (stage == 2) interactie_npc = 9; 
                            else if (stage == 3) interactie_npc = 9; else if (stage == 4) interactie_npc = 8; else interactie_npc = 9;
                        } else if (regio == 4) { 
                            if (stage == 1) interactie_npc = 10; else if (stage == 2) interactie_npc = 8; 
                            else if (stage == 3) interactie_npc = 7; else if (stage == 4) interactie_npc = 8; else interactie_npc = 7;
                        }
                        dialoguePage = 0; state = 8; stateChanged = true;
                    }
                }
            }
        }
        else if (state == 101) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                if (regio == 0) drawTextBox("YOU FOUND 10 SECRET", "CAPTURE NETS!");
                if (regio == 2) drawTextBox("YOU FOUND THE", "4-STAR DRAGON BALL!");
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 102) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                drawTextBox("A MYSTICAL BARRIER", "BLOCKS BABIDI'S SHIP...");
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 100) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; 
                drawRectMain(0, 0, 240, 160, COLOR_BLACK); 
                drawUIBoxMain(10, 10, 220, 100);
                drawText("--- ANIME CUTSCENE ---", 40, 40, COLOR_GOLD);
                drawText("(INSERT YOUR PNG HERE LATER)", 20, 60, COLOR_WHITE);
                
                drawTextBox("MADARA:", "PREPARE TO BE CRUSHED!");
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) {
                state = 2; startBattle(true); stateChanged = true;
            }
        }
        else if (state == 9) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; 
                drawRectMain(0, 0, 240, 160, 0x2108); 
                drawUIBoxMain(10, 10, 220, 140); 
                drawText("NURSE JOY'S DESK", 65, 20, COLOR_GOLD);
                drawText("BERRIES:", 20, 40, COLOR_WHITE); drawNumber(berries, 90, 40, COLOR_GREEN);
                
                drawText("1. HEAL TEAM (FREE)", 30, 60, (shopCursor==0?COLOR_RED:COLOR_WHITE));
                drawText("2. BUY SENZU (500B)", 30, 75, (shopCursor==1?COLOR_RED:COLOR_WHITE));
                drawText("3. BUY NET (200B)", 30, 90, (shopCursor==2?COLOR_RED:COLOR_WHITE));
                drawText("4. ACCESS PC BOX", 30, 105, (shopCursor==3?COLOR_RED:COLOR_WHITE));
                if (gameCleared) {
                    drawText("5. COLOSSEUM (ENDGAME)", 30, 120, (shopCursor==4?COLOR_RED:COLOR_WHITE));
                    drawText("6. EXIT", 30, 135, (shopCursor==5?COLOR_RED:COLOR_WHITE));
                } else {
                    drawText("5. EXIT", 30, 120, (shopCursor==4?COLOR_RED:COLOR_WHITE));
                }
                stateChanged = false;
            }
            int maxOptions = gameCleared ? 6 : 5;
            if (isKeyJustPressed(KEY_DOWN)) { shopCursor = (shopCursor+1)%maxOptions; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { shopCursor = (shopCursor+(maxOptions-1))%maxOptions; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (shopCursor == 0) { healWholeTeam(); state = 88; stateChanged = true; }
                else if (shopCursor == 1 && berries >= 500) { berries -= 500; itemAantal[2]++; stateChanged = true; }
                else if (shopCursor == 2 && berries >= 200) { berries -= 200; itemAantal[3]++; stateChanged = true; }
                else if (shopCursor == 3) { state = 75; stateChanged = true; } 
                else if (shopCursor == 4 && gameCleared) { state = 110; stateChanged = true; } 
                else { state = 1; stateChanged = true; } 
            }
            if (isKeyJustPressed(KEY_B)) { state = 1; stateChanged = true; }
        }
        else if (state == 110) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(40, 40, 160, 80);
                drawText("ENDLESS COLOSSEUM", 50, 50, COLOR_GOLD);
                drawText("WAVE:", 60, 70, COLOR_WHITE); drawNumber(colosseumWave, 110, 70, COLOR_WHITE);
                drawText("FIGHT", 60, 90, (menuCursor==0?COLOR_RED:COLOR_WHITE));
                drawText("LEAVE", 60, 105, (menuCursor==1?COLOR_RED:COLOR_WHITE));
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { menuCursor = (menuCursor+1)%2; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { menuCursor = (menuCursor+1)%2; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (menuCursor == 0) {
                    prevRegio = regio;
                    regio = 5; 
                    regio_stage[5] = (colosseumWave > 5) ? 5 : colosseumWave; 
                    interactie_npc = rand() % 11; 
                    state = 2; startBattle(true); stateChanged = true;
                } else {
                    state = 9; stateChanged = true; 
                }
            }
            if (isKeyJustPressed(KEY_B)) { state = 9; stateChanged = true; }
        }
        else if (state == 112) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, COLOR_BLACK);
                drawUIBoxMain(20, 40, 200, 80);
                drawText("COLOSSEUM DEFEAT...", 50, 60, COLOR_RED);
                drawText("WAVES CLEARED:", 40, 80, COLOR_WHITE); drawNumber(colosseumWave - 1, 150, 80, COLOR_GOLD);
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) {
                colosseumWave = 1; healWholeTeam(); regio = prevRegio; state = 1; stateChanged = true;
            }
        }
        else if (state == 75) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(40, 40, 160, 80);
                drawText("--- PC SYSTEM ---", 60, 50, COLOR_GOLD);
                drawText("DEPOSIT", 60, 70, (menuCursor==0?COLOR_RED:COLOR_WHITE)); drawText("WITHDRAW", 60, 90, (menuCursor==1?COLOR_RED:COLOR_WHITE));
                drawText("EXIT", 60, 110, (menuCursor==2?COLOR_RED:COLOR_WHITE)); stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { menuCursor = (menuCursor+1)%3; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { menuCursor = (menuCursor+2)%3; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (menuCursor == 0) { state = 76; partyCursor = 0; stateChanged = true; } 
                else if (menuCursor == 1) { state = 77; pcCursor = 0; stateChanged = true; } 
                else if (menuCursor == 2) { state = 9; stateChanged = true; } 
            }
            if (isKeyJustPressed(KEY_B)) { state = 9; stateChanged = true; }
        }
        else if (state == 76) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(20, 10, 200, 140); drawText("DEPOSIT TO PC", 75, 20, COLOR_GOLD);
                for(int i=0; i<6; i++) { 
                    if(team[i].isGevuld) {
                        uint16_t c = (partyCursor == i) ? COLOR_RED : COLOR_WHITE; drawText(team[i].naam, 30, 40+i*15, c); 
                        drawText("LVL", 150, 40+i*15, c); drawNumber(team[i].lvl, 180, 40+i*15, c);
                    }
                }
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { do { partyCursor=(partyCursor+1)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { do { partyCursor=(partyCursor+5)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                int count = 0; for(int i=0; i<6; i++) { if(team[i].isGevuld) count++; }
                if (count > 1) {
                    int slot = getEmptyPcSlot();
                    if (slot != -1) {
                        pcBox[slot] = team[partyCursor]; team[partyCursor].isGevuld = false;
                        if (partyCursor == activeIdx) { for(int i=0; i<6; i++) { if(team[i].isGevuld) { activeIdx = i; break; } } }
                        state = 75; stateChanged = true;
                    }
                } else { drawTextBox("CANNOT DEPOSIT", "LAST MEMBER!"); }
            }
            if (isKeyJustPressed
