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

// Nu 6 slots zodat regio 5 (Colosseum) niet de array breekt
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
                drawText("V2.5 ENDGAME", 100, 70, COLOR_WHITE); 
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
                // V2.5: VERBORGEN KEY ITEMS (MAP PUZZELS)
                if (regio == 0 && !hasMasterNet && abs(worldX - 20) < 30 && abs(worldY - 200) < 30) {
                    hasMasterNet = true; itemAantal[3] += 10; state = 101; stateChanged = true;
                }
                else if (regio == 2 && !hasDragonBall && abs(worldX - 20) < 30 && abs(worldY - 40) < 30) {
                    hasDragonBall = true; state = 101; stateChanged = true;
                }

                // NURSE JOY
                else if (abs(worldX-40) < 30 && abs(worldY-40) < 30) { state = 9; shopCursor = 0; stateChanged = true; }
                
                // SIDE QUESTS
                else if (side_quest_stage[regio] == 0 && abs(worldX - questX) < 30 && abs(worldY - questY) < 30) {
                    dialoguePage = 0; state = 80; stateChanged = true;
                }

                // BOSS BATTLES
                else if (regio_stage[regio] <= 5 && abs(worldX - npcX) < 30 && abs(worldY - npcY) < 30) {
                    // V2.5: De Magische Barrière (Alabasta Babidi's Ship)
                    if (regio == 2 && regio_stage[regio] >= 3 && !hasDragonBall) {
                        state = 102; stateChanged = true; // Barrière Pop-up
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
        else if (state == 101) { // FOUND ITEM POP-UP
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                if (regio == 0) drawTextBox("YOU FOUND 10 SECRET", "CAPTURE NETS!");
                if (regio == 2) drawTextBox("YOU FOUND THE", "4-STAR DRAGON BALL!");
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 102) { // BARRIER POP-UP
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                drawTextBox("A MYSTICAL BARRIER", "BLOCKS BABIDI'S SHIP...");
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 100) { // V2.5: VISUAL STORYTELLING (ANIME CUTSCENES)
            if (stateChanged) {
                REG_DISPCNT = 0x0403; 
                // Hier tekenen we straks je .PNG bitmap (zoals background_map)
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
        else if (state == 9) { // SHOP 
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
                else if (shopCursor == 4 && gameCleared) { state = 110; stateChanged = true; } // Naar Colosseum
                else { state = 1; stateChanged = true; } // Exit
            }
            if (isKeyJustPressed(KEY_B)) { state = 1; stateChanged = true; }
        }
        else if (state == 110) { // V2.5: COLOSSEUM LOBBY
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
                    regio = 5; // Colosseum Modus
                    regio_stage[5] = (colosseumWave > 5) ? 5 : colosseumWave; 
                    interactie_npc = rand() % 11; 
                    state = 2; startBattle(true); stateChanged = true;
                } else {
                    state = 9; stateChanged = true; 
                }
            }
            if (isKeyJustPressed(KEY_B)) { state = 9; stateChanged = true; }
        }
        else if (state == 112) { // COLOSSEUM DEFEAT
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
            if (isKeyJustPressed(KEY_B)) { state = 75; stateChanged = true; }
        }
        else if (state == 77) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(20, 10, 200, 140); drawText("WITHDRAW FROM PC", 65, 20, COLOR_GOLD);
                int displayCount = 0;
                for(int i=0; i<30; i++) {
                    if (pcBox[i].isGevuld) {
                        if (pcCursor == i) {
                            drawText(pcBox[i].naam, 30, 50, COLOR_RED); drawText("LVL", 150, 50, COLOR_RED); drawNumber(pcBox[i].lvl, 180, 50, COLOR_RED);
                            drawBitmapSprite(88, 70, 64, 64, pcBox[i].battle_front_bitmap);
                        }
                        displayCount++;
                    }
                }
                if (displayCount == 0) drawText("PC IS EMPTY!", 75, 70, COLOR_WHITE);
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_RIGHT)) { do { pcCursor=(pcCursor+1)%30; } while(!pcBox[pcCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_LEFT)) { do { pcCursor=(pcCursor+29)%30; } while(!pcBox[pcCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (pcBox[pcCursor].isGevuld) {
                    int slot = getEmptyPartySlot();
                    if (slot != -1) { team[slot] = pcBox[pcCursor]; pcBox[pcCursor].isGevuld = false; state = 75; stateChanged = true; } 
                    else { drawTextBox("PARTY IS FULL!", ""); }
                }
            }
            if (isKeyJustPressed(KEY_B)) { state = 75; stateChanged = true; }
        }
        else if (state == 7) { 
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(60, 20, 120, 120); 
                drawText("STAGE:", 10, 10, COLOR_WHITE); drawNumber(regio_stage[regio], 60, 10, COLOR_WHITE); drawText("/ 5", 70, 10, COLOR_WHITE);
                drawText("PARTY", 95, 40, (menuCursor==0?COLOR_RED:COLOR_WHITE)); drawText("BAG", 95, 60, (menuCursor==1?COLOR_RED:COLOR_WHITE));
                drawText("ANIME DEX", 95, 80, (menuCursor==2?COLOR_RED:COLOR_WHITE)); drawText("CLOSE", 95, 100, (menuCursor==3?COLOR_RED:COLOR_WHITE));
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_DOWN)) { menuCursor = (menuCursor+1)%4; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { menuCursor = (menuCursor+3)%4; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) { 
                if (menuCursor == 0) { state = 71; } else if (menuCursor == 1) { state = 73; bagCursor = 0; } 
                else if (menuCursor == 2) { state = 72; } else { state = 1; }
                stateChanged = true; 
            }
            if (isKeyJustPressed(KEY_B)) { state = 1; stateChanged = true; }
        }
        else if (state == 73) { 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(10, 10, 220, 140); drawText("YOUR BAG:", 15, 20, COLOR_GOLD);
                for(int i = 0; i < 4; i++) {
                    uint16_t color = (bagCursor == i) ? COLOR_RED : COLOR_WHITE; if (itemAantal[i] <= 0) color = 0x3DEF; 
                    drawText(itemNamen[i], 30, 50 + (i * 15), color); drawText("x", 170, 50 + (i * 15), color); drawNumber(itemAantal[i], 180, 50 + (i * 15), color);
                }
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { bagCursor = (bagCursor + 1) % 4; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { bagCursor = (bagCursor + 3) % 4; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) { if (itemAantal[bagCursor] > 0 && itemHeal[bagCursor] > 0) { state = 74; partyCursor = 0; stateChanged = true; } }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 74) { 
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(20, 10, 200, 140); drawText("USE ITEM ON:", 85, 20, COLOR_GOLD);
                for(int i=0; i<6; i++) { 
                    if(team[i].isGevuld) {
                        uint16_t c = (partyCursor == i) ? COLOR_RED : COLOR_WHITE;
                        drawText(team[i].naam, 30, 40+i*15, c); drawText("HP:", 130, 40+i*15, c); drawNumber(team[i].hp, 160, 40+i*15, c);
                    }
                }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_DOWN)) { do { partyCursor=(partyCursor+1)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { do { partyCursor=(partyCursor+5)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) { 
                itemAantal[bagCursor]--; team[partyCursor].hp += itemHeal[bagCursor];
                if(team[partyCursor].hp > team[partyCursor].max_hp) team[partyCursor].hp = team[partyCursor].max_hp;
                state = 73; stateChanged = true; 
            }
            if (isKeyJustPressed(KEY_B)) { state = 73; stateChanged = true; }
        }
        else if (state == 71) { 
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(20, 10, 200, 140); drawText("YOUR TEAM", 85, 20, COLOR_GOLD);
                for(int i=0; i<6; i++) { 
                    if(team[i].isGevuld) { uint16_t c = (partyCursor == i) ? COLOR_RED : COLOR_WHITE; drawText(team[i].naam, 40, 40+i*15, c); }
                }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_DOWN)) { do { partyCursor=(partyCursor+1)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { do { partyCursor=(partyCursor+5)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) { activeIdx = partyCursor; stateChanged = true; }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 72) { 
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108); drawUIBoxMain(10, 10, 220, 140);
                Karakter d; initKarakter(&d, dexCursor, 1); drawText(d.naam, 20, 30, COLOR_RED); 
                if (d.battle_front_bitmap != NULL) { drawBitmapSprite(140, 55, 64, 64, d.battle_front_bitmap); }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_RIGHT)) { dexCursor = (dexCursor+1)%11; stateChanged = true; }
            if (isKeyJustPressed(KEY_LEFT)) { dexCursor = (dexCursor+10)%11; stateChanged = true; }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 80) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                if (regio == 0) {
                    if (dialoguePage == 0) drawTextBox("VILLAGER: HELP! A", "ROGUE SAMURAI...");
                    else if (dialoguePage == 1) drawTextBox("VILLAGER: HE IS", "ATTACKING PEOPLE!");
                    else drawTextBox("VILLAGER: PLEASE,", "STOP HIM!");
                } else { drawTextBox("VILLAGER: NO QUEST", "AVAILABLE HERE."); }
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_A)) {
                int maxPages = (regio == 0) ? 2 : 0;
                if (dialoguePage < maxPages) { dialoguePage++; stateChanged = true; }
                else if (regio == 0) { interactie_npc = 1; state = 2; startBattle(false); stateChanged = true; } 
                else { state = 1; stateChanged = true; }
            }
        }
        else if (state == 8) { 
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                int stage = regio_stage[regio];
                
                if (regio == 0) {
                    if (stage == 1) {
                        if (dialoguePage == 0) drawTextBox("ZORO: HEY YOU!", "YOU LOOK STRONG.");
                        else if (dialoguePage == 1) drawTextBox("ZORO: I NEED TO TEST", "MY NEW SWORDS.");
                        else drawTextBox("ZORO: SHOW ME WHAT", "YOU CAN DO!");
                    } else if (stage == 2) drawTextBox("LUFFY: KAIDO IS MINE!", "LET'S SPAR FIRST!");
                    else if (stage == 3) drawTextBox("KAIDO: YOU BRATS WANT", "TO PLAY PIRATE? WORORO!");
                    else if (stage == 4) drawTextBox("SHANKS: I'M HERE TO", "TEST YOUR RESOLVE.");
                    else {
                        if (dialoguePage == 0) drawTextBox("KAIDO: YOU MADE IT", "THIS FAR...");
                        else if (dialoguePage == 1) drawTextBox("KAIDO: BUT THIS IS", "WHERE YOU DIE.");
                        else drawTextBox("KAIDO: I WILL SHOW", "YOU TRUE DESPAIR!");
                    }
                } 
                else if (regio == 1) { 
                    if (stage == 1) drawTextBox("ZORO: THE MARINES ARE", "SWARMING THIS PLACE."); else if (stage == 2) drawTextBox("NARUTO: I WON'T LET", "MY FRIENDS DIE HERE!");
                    else if (stage == 3) drawTextBox("PAIN: WAR ONLY", "BREEDS MORE PAIN."); else if (stage == 4) drawTextBox("SHANKS: I'VE COME TO", "PUT AN END TO THIS WAR.");
                    else drawTextBox("MADARA: THIS BATTLEFIELD", "LACKS TRUE DESPAIR.");
                } 
                else if (regio == 2) { 
                    if (stage == 1) {
                        if (dialoguePage == 0) drawTextBox("GOKU: THIS IS BAD!", "BABIDI IS HERE.");
                        else if (dialoguePage == 1) drawTextBox("GOKU: HE TAKES CONTROL", "OF EVIL HEARTS.");
                        else if (dialoguePage == 2) drawTextBox("GOKU: VEGETA LET HIM", "IN PURPOSELY!");
                        else drawTextBox("GOKU: WE HAVE TO", "STOP HIM NOW!");
                    } else if (stage == 2) {
                        if (dialoguePage == 0) drawTextBox("OBITO: WELCOME TO", "BABIDI'S SPACESHIP.");
                        else if (dialoguePage == 1) drawTextBox("OBITO: YOU ARE TOO LATE.", "THE PRINCE IS OURS.");
                        else if (dialoguePage == 2) drawTextBox("OBITO: HIS POWER IS", "FEEDING MAJIN BUU.");
                        else drawTextBox("OBITO: YOU WILL NOT", "PASS THIS FLOOR!");
                    } else if (stage == 3) {
                        if (dialoguePage == 0) drawTextBox("VEGETA: I WANTED TO", "BE EVIL AGAIN!");
                        else if (dialoguePage == 1) drawTextBox("VEGETA: PEACE AND FAMILY", "MADE ME WEAK.");
                        else if (dialoguePage == 2) drawTextBox("VEGETA: BABIDI FREED", "MY TRUE SAIYAN PRIDE.");
                        else drawTextBox("VEGETA: NOW DIE!", "GALICK GUN!");
                    } else if (stage == 4) {
                        if (dialoguePage == 0) drawTextBox("GOKU: VEGETA HAS LOST", "HIS MIND COMPLETELY.");
                        else if (dialoguePage == 1) drawTextBox("GOKU: MAJIN BUU IS", "ABOUT TO HATCH!");
                        else if (dialoguePage == 2) drawTextBox("GOKU: I NEED TO USE", "SUPER SAIYAN 3.");
                        else drawTextBox("GOKU: BUT FIRST, SHOW", "ME YOUR STRENGTH!");
                    } else {
                        if (dialoguePage == 0) drawTextBox("VEGETA: BUU IS AWAKE.", "IT'S MY FAULT.");
                        else if (dialoguePage == 1) drawTextBox("VEGETA: I AM A PROUD", "SAIYAN PRINCE!");
                        else if (dialoguePage == 2) drawTextBox("VEGETA: TRUNKS, BULMA...", "AND EVEN YOU, KAKAROT.");
                        else drawTextBox("VEGETA: FAREWELL!", "FINAL EXPLOSION!");
                    }
                } 
                else if (regio == 3) { 
                    if (stage == 1) {
                        if (dialoguePage == 0) drawTextBox("ITACHI: NARUTO... WHY", "SO OBSESSED WITH SASUKE?");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: BECAUSE HE IS", "MY FRIEND!");
                        else if (dialoguePage == 2) drawTextBox("ITACHI: YOUR IDEALS", "ARE NAIVE. TURN BACK.");
                        else drawTextBox("ITACHI: I WILL TEST", "YOUR RESOLVE NOW.");
                    } else if (stage == 2) {
                        if (dialoguePage == 0) drawTextBox("PAIN: THIS VILLAGE HAS", "ENJOYED PEACE TOO LONG.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: WHO ARE YOU?!", "WHAT DID YOU DO?!");
                        else if (dialoguePage == 2) drawTextBox("PAIN: I AM A GOD.", "I BRING ORDER.");
                        else drawTextBox("PAIN: FEEL THE PAIN", "OF THE WORLD!");
                    } else if (stage == 3) {
                        if (dialoguePage == 0) drawTextBox("PAIN: DO YOU HATE ME", "NOW? GOOD.");
                        else if (dialoguePage == 1) drawTextBox("PAIN: BUT YOUR JUSTICE", "IS LIKE MINE.");
                        else if (dialoguePage == 2) drawTextBox("PAIN: WE ARE MEN DRIVEN", "BY PURE REVENGE.");
                        else drawTextBox("NARUTO: I'LL BREAK THAT", "CYCLE! I WON'T GIVE UP!");
                    } else if (stage == 4) {
                        if (dialoguePage == 0) drawTextBox("OBITO: PAIN IS TAKING", "TOO LONG. I'LL STEP IN.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: MADARA?! I", "THOUGHT YOU WERE DEAD!");
                        else if (dialoguePage == 2) drawTextBox("OBITO: I AM NOBODY.", "NAMES HOLD NO MEANING.");
                        else drawTextBox("OBITO: HAND OVER THE", "NINE-TAILS FOX!");
                    } else {
                        if (dialoguePage == 0) drawTextBox("PAIN: MY PAIN IS STILL", "FAR GREATER THAN YOURS!");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: I UNDERSTAND", "YOUR PAIN NOW...");
                        else if (dialoguePage == 2) drawTextBox("NARUTO: BUT I CANNOT", "LET YOU WIN!");
                        else drawTextBox("PAIN: ALMIGHTY PUSH!", "");
                    }
                } 
                else if (regio == 4) { 
                    if (stage == 1) {
                        if (dialoguePage == 0) drawTextBox("ITACHI: I AM EDO TENSEI.", "I CANNOT STOP MYSELF.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: ITACHI! WE'LL", "FIND A WAY TO FREE YOU!");
                        else if (dialoguePage == 2) drawTextBox("ITACHI: NO. YOU MUST", "DEFEAT ME TO ADVANCE.");
                        else drawTextBox("ITACHI: PROTECT THE", "FUTURE OF THE SHINOBI!");
                    } else if (stage == 2) {
                        if (dialoguePage == 0) drawTextBox("OBITO: YOU BROKE MY", "MASK... SO WHAT.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: OBITO! WHY", "ARE YOU DOING THIS?!");
                        else if (dialoguePage == 2) drawTextBox("OBITO: LOOK AT THIS", "WORLD... ONLY SUFFERING.");
                        else drawTextBox("OBITO: I WILL CREATE", "A PERFECT DREAM.");
                    } else if (stage == 3) {
                        if (dialoguePage == 0) drawTextBox("MADARA: SO, THE TIME", "HAS FINALLY COME.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: THE REAL", "MADARA UCHIHA...");
                        else if (dialoguePage == 2) drawTextBox("MADARA: YOU CHILDREN", "WANT TO DANCE?");
                        else drawTextBox("MADARA: LET'S SEE HOW", "YOU HANDLE A METEOR!");
                    } else if (stage == 4) {
                        if (dialoguePage == 0) drawTextBox("OBITO: I HAVE ABSORBED", "THE TEN-TAILS.");
                        else if (dialoguePage == 1) drawTextBox("NARUTO: WE WON'T LET", "YOU ERASE OUR WORLD!");
                        else if (dialoguePage == 2) drawTextBox("OBITO: IT IS FUTILE.", "MY POWER IS ABSOLUTE.");
                        else drawTextBox("OBITO: SLEEP IN THE", "INFINITE TSUKUYOMI.");
                    } else {
                        if (dialoguePage == 0) drawTextBox("MADARA: YOU FOUGHT WELL", "BUT IT IS OVER.");
                        else if (dialoguePage == 1) drawTextBox("MADARA: THE MOON IS RED.", "THE ILLUSION IS CAST.");
                        else if (dialoguePage == 2) drawTextBox("NARUTO: I WON'T GIVE", "UP! EVERYONE TRUSTS ME!");
                        else drawTextBox("MADARA: WAKE UP TO", "REALITY! DIE IN DESPAIR!");
                    }
                }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_A)) { 
                int maxPages = 0;
                if (regio == 2 || regio == 3 || regio == 4) { maxPages = 3; } 
                else if (regio == 0 && (regio_stage[regio] == 1 || regio_stage[regio] == 5)) { maxPages = 2; }
                
                if (dialoguePage < maxPages) { dialoguePage++; stateChanged = true; } 
                else { 
                    // V2.5 CUTSCENE TRIGGER VOOR MADARA!
                    if (regio == 4 && regio_stage[regio] == 5) {
                        state = 100; stateChanged = true; 
                    } else {
                        state = 2; startBattle(true); stateChanged = true; 
                    }
                }
            }
        }
        else if (state == 88 || state == 89) { 
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108); 
                if(state == 88) drawTextBox("NURSE: YOUR TEAM", "IS FULLY RESTORED!"); else drawTextBox("THE PATH IS BLOCKED!", "DEFEAT THE 5 STAGES."); 
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 90) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, COLOR_WHITE); animTimer = 120; stateChanged = false;
            }
            animTimer--;
            if (animTimer == 100) {
                drawRectMain(0, 0, 240, 160, COLOR_BLACK); drawUIBoxMain(20, 40, 200, 80);
                drawText("WHAT? YOUR HERO", 50, 60, COLOR_WHITE); drawText("IS EVOLVING!", 65, 80, COLOR_GOLD);
            }
            if (animTimer == 40) {
                drawRectMain(0, 0, 240, 160, COLOR_BLACK); drawUIBoxMain(20, 40, 200, 80);
                drawText("CONGRATULATIONS!", 50, 60, COLOR_GREEN); drawText(team[activeIdx].naam, 60, 80, COLOR_WHITE);
            }
            if (animTimer <= 0 && isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; initOAM(); }
        }
        else if (state == 99) { 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, COLOR_BLACK); drawUIBoxMain(20, 20, 200, 120);
                drawText("CONGRATULATIONS!", 60, 40, COLOR_GOLD); drawText("YOU SAVED THE ANIME WORLD!", 25, 70, COLOR_WHITE);
                drawText("PRESS START TO CONTINUE", 35, 110, COLOR_RED); stateChanged = false;
            }
            if (isKeyJustPressed(KEY_START)) { 
                gameCleared = true; // V2.5 UNLOCK COLOSSEUM!
                state = 1; stateChanged = true; initOAM();
            }
        }
        else if (state == 2) { 
            if (stateChanged) { REG_DISPCNT = 0x0403; stateChanged = false; }
            if (updateBattle()) { 
                
                // V2.5 ENDGAME COLOSSEUM AFHANDELING
                if (regio == 5) {
                    if (vijand_team[0].hp <= 0) {
                        colosseumWave++;
                        berries += 1000 * colosseumWave;
                        state = 110; stateChanged = true; initOAM();
                    } else {
                        state = 112; stateChanged = true; initOAM();
                    }
                    continue; // Sla de rest van de story logic over
                }

                if (vijand_team[0].hp <= 0 && bBoss) { 
                    berries += (500 * regio_stage[regio]); itemAantal[0] += 1; regio_stage[regio]++; 
                    if (regio == 4 && regio_stage[regio] > 5) { state = 99; stateChanged = true; initOAM(); continue; }
                }
                
                if (vijand_team[0].hp <= 0 && interactie_npc == 1 && side_quest_stage[regio] == 0 && state == 80) {
                    side_quest_stage[regio] = 1; berries += 1000; 
                }
                
                if (bBoss) { worldX -= 15; interactie_npc = 0; }
                if (justEvolved) { state = 90; stateChanged = true; justEvolved = false; } else { state = 1; stateChanged = true; initOAM(); }
            }
        }
    }
    return 0;
}
