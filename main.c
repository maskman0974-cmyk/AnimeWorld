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

extern void fillScreen(uint16_t color);
extern void drawText(char* text, int x, int y, uint16_t color);
extern void drawNumber(int num, int x, int y, uint16_t color);
extern void drawTextBox(char* line1, char* line2);
extern void initOAM(void);
extern void setSpriteAttribute(int id, int x, int y, int pal, int tile, int priority, int size);
extern void startBattle(bool isStory); 
extern bool updateBattle(void);
extern void drawBitmapSprite(int x, int y, int width, int height, const uint16_t* data);

void drawRectMain(int x, int y, int w, int h, uint16_t color) {
    volatile uint16_t* vram = (volatile uint16_t*)0x06000000;
    for(int r = 0; r < h; r++) {
        for(int c = 0; c < w; c++) {
            vram[(y + r) * 240 + (x + c)] = color;
        }
    }
}

void drawUIBoxMain(int x, int y, int w, int h) {
    drawRectMain(x, y, w, h, 0x4A69); 
    drawRectMain(x, y, w, 2, 0x7FFF); 
    drawRectMain(x, y + h - 2, w, 2, 0x7FFF); 
    drawRectMain(x, y, 2, h, 0x7FFF); 
    drawRectMain(x + w - 2, y, 2, h, 0x7FFF); 
}

extern char* itemNamen[];
extern int itemHeal[];
extern int itemAantal[];
extern Karakter vijand_team[6]; 
extern bool bBoss;

int berries = 1000; 
static uint16_t current_keys = 0;
static uint16_t previous_keys = 0;

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
int menuCursor = 0, partyCursor = 0, dexCursor = 0, shopCursor = 0, bagCursor = 0;

int regio_stage[5] = {1, 1, 1, 1, 1}; 
int interactie_npc = 0; 

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
                drawText("V1.9.2", 110, 70, COLOR_WHITE); 
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

            // VASTE LOCATIE VOOR DE BOSS: RECHTS OP DE MAP
            int npcX = 200; 
            int npcY = 80;
            
            if (regio_stage[regio] <= 5) {
                int bx = npcX - camX; 
                int by = npcY - camY;
                if (bx > -16 && bx < 240 && by > -16 && by < 160) {
                    setSpriteAttribute(1, bx, by, 0, 0, 0, 1);
                } else { setSpriteAttribute(1, 0, 160, 0, 0, 0, 1); }
            } else { setSpriteAttribute(1, 0, 160, 0, 0, 0, 1); }

            // NURSE JOY LOCATIE: LINKSBOVEN
            int nx = 40 - camX; int ny = 40 - camY;
            if (nx > -16 && nx < 240 && ny > -16 && ny < 160) {
                setSpriteAttribute(2, nx, ny, 0, 0, 0, 1);
            } else { setSpriteAttribute(2, 0, 160, 0, 0, 0, 1); }

            // A-KNOP ACTIES: NU KAN JE ER RUSTIG LANGSLOPEN
            if (isKeyJustPressed(KEY_A)) {
                if (abs(worldX-40) < 30 && abs(worldY-40) < 30) { 
                    state = 9; stateChanged = true; 
                }
                else if (regio_stage[regio] <= 5 && abs(worldX - npcX) < 30 && abs(worldY - npcY) < 30) {
                    int stage = regio_stage[regio];
                    if (regio == 0) {
                        if (stage == 1) interactie_npc = 1; else if (stage == 2) interactie_npc = 0; 
                        else if (stage == 3) interactie_npc = 2; else if (stage == 4) interactie_npc = 6; else interactie_npc = 2;
                    } else if (regio == 1) { 
                        if (stage == 1) interactie_npc = 1; else if (stage == 2) interactie_npc = 4; 
                        else if (stage == 3) interactie_npc = 9; else if (stage == 4) interactie_npc = 6; else interactie_npc = 7;
                    } else if (regio == 2) { 
                        if (stage == 1) interactie_npc = 3; else if (stage == 2) interactie_npc = 5; 
                        else if (stage == 3) interactie_npc = 8; else if (stage == 4) interactie_npc = 3; else interactie_npc = 5;
                    } else if (regio == 3) { 
                        if (stage == 1) interactie_npc = 10; else if (stage == 2) interactie_npc = 8; 
                        else if (stage == 3) interactie_npc = 9; else if (stage == 4) interactie_npc = 4; else interactie_npc = 9;
                    } else if (regio == 4) { 
                        if (stage == 1) interactie_npc = 10; else if (stage == 2) interactie_npc = 5; 
                        else if (stage == 3) interactie_npc = 2; else if (stage == 4) interactie_npc = 8; else interactie_npc = 7;
                    }
                    state = 8; stateChanged = true;
                }
            }
        }
        else if (state == 9) { // SHOP 
            if (stateChanged) {
                REG_DISPCNT = 0x0403; 
                drawRectMain(0, 0, 240, 160, 0x2108); 
                drawUIBoxMain(10, 10, 220, 140); 
                drawText("NURSE JOY'S SHOP", 65, 20, COLOR_GOLD);
                drawText("BERRIES:", 20, 40, COLOR_WHITE); drawNumber(berries, 90, 40, COLOR_GREEN);
                
                drawText("1. HEAL TEAM (FREE)", 30, 70, (shopCursor==0?COLOR_RED:COLOR_WHITE));
                drawText("2. BUY SENZU BEAN (500B)", 30, 90, (shopCursor==1?COLOR_RED:COLOR_WHITE));
                drawText("3. BUY CAPTURE NET (200B)", 30, 110, (shopCursor==2?COLOR_RED:COLOR_WHITE));
                drawText("4. EXIT", 30, 130, (shopCursor==3?COLOR_RED:COLOR_WHITE));
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { shopCursor = (shopCursor+1)%4; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { shopCursor = (shopCursor+3)%4; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (shopCursor == 0) { healWholeTeam(); state = 88; stateChanged = true; }
                else if (shopCursor == 1 && berries >= 500) { berries -= 500; itemAantal[2]++; stateChanged = true; }
                else if (shopCursor == 2 && berries >= 200) { berries -= 200; itemAantal[3]++; stateChanged = true; }
                else if (shopCursor == 3) { state = 1; stateChanged = true; }
            }
            if (isKeyJustPressed(KEY_B)) { state = 1; stateChanged = true; }
        }
        else if (state == 7) { // PAUZE MENU 
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; 
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(60, 20, 120, 120); 
                drawText("STAGE:", 10, 10, COLOR_WHITE);
                drawNumber(regio_stage[regio], 60, 10, COLOR_WHITE);
                drawText("/ 5", 70, 10, COLOR_WHITE);

                drawText("PARTY", 95, 40, (menuCursor==0?COLOR_RED:COLOR_WHITE));
                drawText("BAG", 95, 60, (menuCursor==1?COLOR_RED:COLOR_WHITE));
                drawText("ANIME DEX", 95, 80, (menuCursor==2?COLOR_RED:COLOR_WHITE));
                drawText("CLOSE", 95, 100, (menuCursor==3?COLOR_RED:COLOR_WHITE));
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
        else if (state == 73) { // OVERWORLD BAG 
            if (stateChanged) {
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(10, 10, 220, 140);
                drawText("YOUR BAG:", 15, 20, COLOR_GOLD);
                
                for(int i = 0; i < 4; i++) {
                    uint16_t color = (bagCursor == i) ? COLOR_RED : COLOR_WHITE;
                    if (itemAantal[i] <= 0) color = 0x3DEF; 
                    drawText(itemNamen[i], 30, 50 + (i * 15), color);
                    drawText("x", 170, 50 + (i * 15), color);
                    drawNumber(itemAantal[i], 180, 50 + (i * 15), color);
                }
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_DOWN)) { bagCursor = (bagCursor + 1) % 4; stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { bagCursor = (bagCursor + 3) % 4; stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) {
                if (itemAantal[bagCursor] > 0 && itemHeal[bagCursor] > 0) { state = 74; partyCursor = 0; stateChanged = true; }
            }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 74) { // SELECTEER CHAR OM TE HEALEN
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(20, 10, 200, 140);
                drawText("USE ITEM ON:", 85, 20, COLOR_GOLD);
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
        else if (state == 71) { // PARTY
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(20, 10, 200, 140);
                drawText("YOUR TEAM", 85, 20, COLOR_GOLD);
                for(int i=0; i<6; i++) { 
                    if(team[i].isGevuld) {
                        uint16_t c = (partyCursor == i) ? COLOR_RED : COLOR_WHITE;
                        drawText(team[i].naam, 40, 40+i*15, c); 
                    }
                }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_DOWN)) { do { partyCursor=(partyCursor+1)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_UP)) { do { partyCursor=(partyCursor+5)%6; } while(!team[partyCursor].isGevuld); stateChanged = true; }
            if (isKeyJustPressed(KEY_A)) { activeIdx = partyCursor; stateChanged = true; }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 72) { // DEX
            if (stateChanged) { 
                drawRectMain(0, 0, 240, 160, 0x2108);
                drawUIBoxMain(10, 10, 220, 140);
                Karakter d; initKarakter(&d, dexCursor, 1);
                drawText(d.naam, 20, 30, COLOR_RED); 
                if (d.battle_front_bitmap != NULL) { drawBitmapSprite(140, 55, 64, 64, d.battle_front_bitmap); }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_RIGHT)) { dexCursor = (dexCursor+1)%11; stateChanged = true; }
            if (isKeyJustPressed(KEY_LEFT)) { dexCursor = (dexCursor+10)%11; stateChanged = true; }
            if (isKeyJustPressed(KEY_B)) { state = 7; stateChanged = true; }
        }
        else if (state == 8) { // STAGE DIALOOG
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108);
                int stage = regio_stage[regio];
                if (regio == 0) {
                    if (stage == 1) drawTextBox("ZORO: LET'S TEST", "YOUR SWORDSMANSHIP!"); else if (stage == 2) drawTextBox("LUFFY: KAIDO IS MINE!", "LET'S SPAR FIRST!");
                    else if (stage == 3) drawTextBox("KAIDO: YOU BRATS WANT", "TO PLAY PIRATE? WORORO!"); else if (stage == 4) drawTextBox("SHANKS: I'M HERE TO", "TEST YOUR RESOLVE.");
                    else drawTextBox("KAIDO: I'LL SHOW YOU", "TRUE DESPAIR!");
                } else if (regio == 1) { 
                    if (stage == 1) drawTextBox("ZORO: THE MARINES ARE", "SWARMING THIS PLACE."); else if (stage == 2) drawTextBox("NARUTO: I WON'T LET", "MY FRIENDS DIE HERE!");
                    else if (stage == 3) drawTextBox("PAIN: WAR ONLY", "BREEDS MORE PAIN."); else if (stage == 4) drawTextBox("SHANKS: I'VE COME TO", "PUT AN END TO THIS WAR.");
                    else drawTextBox("MADARA: THIS BATTLEFIELD", "LACKS TRUE DESPAIR.");
                } else if (regio == 2) { 
                    if (stage == 1) drawTextBox("GOKU: HEY! VEGETA IS", "ACTING WEIRD, HELP ME!"); else if (stage == 2) drawTextBox("VEGETA: I WILL CONQUER", "THIS WASTELAND!");
                    else if (stage == 3) drawTextBox("OBITO: THE SAIYANS ARE", "EASILY MANIPULATED..."); else if (stage == 4) drawTextBox("GOKU: WE HAVE TO", "STOP HIM TOGETHER!");
                    else drawTextBox("VEGETA: FINAL FLASH!", "BOW TO THE PRINCE!");
                } else if (regio == 3) { 
                    if (stage == 1) drawTextBox("ITACHI: YOU LACK", "HATRED... TURN BACK."); else if (stage == 2) drawTextBox("OBITO: THE AKATSUKI'S", "PLAN IS IN MOTION.");
                    else if (stage == 3) drawTextBox("PAIN: DO YOU HATE", "ME NOW? GOOD."); else if (stage == 4) drawTextBox("NARUTO: I'LL PROTECT", "MY VILLAGE! RASENGAN!");
                    else drawTextBox("PAIN: KNOW PAIN.", "ALMIGHTY PUSH!");
                } else if (regio == 4) { 
                    if (stage == 1) drawTextBox("ITACHI: THE TSUKUYOMI", "HAS BEGUN..."); else if (stage == 2) drawTextBox("VEGETA: GET OUT OF", "MY HEAD, MADARA!!");
                    else if (stage == 3) drawTextBox("KAIDO: A DREAM WORLD?", "BORING! LET'S FIGHT!"); else if (stage == 4) drawTextBox("OBITO: THE REAL WORLD", "IS HELL. SLEEP.");
                    else drawTextBox("MADARA: WAKE UP TO", "REALITY! I AM THE SAVIOR!");
                }
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_A)) { state = 2; startBattle(true); stateChanged = true; }
        }
        else if (state == 88 || state == 89) { 
            if (stateChanged) { 
                REG_DISPCNT = 0x0403; drawRectMain(0, 0, 240, 160, 0x2108); 
                if(state == 88) drawTextBox("NURSE: YOUR TEAM", "IS FULLY RESTORED!"); else drawTextBox("THE PATH IS BLOCKED!", "DEFEAT THE 5 STAGES."); 
                stateChanged = false; 
            }
            if (isKeyJustPressed(KEY_A)) { state = 1; stateChanged = true; }
        }
        else if (state == 99) { // VICTORY SCREEN
            if (stateChanged) {
                REG_DISPCNT = 0x0403;
                drawRectMain(0, 0, 240, 160, COLOR_BLACK);
                drawUIBoxMain(20, 20, 200, 120);
                drawText("CONGRATULATIONS!", 60, 40, COLOR_GOLD);
                drawText("YOU SAVED THE ANIME WORLD!", 25, 70, COLOR_WHITE);
                drawText("PRESS START TO PLAY AGAIN", 35, 110, COLOR_RED);
                stateChanged = false;
            }
            if (isKeyJustPressed(KEY_START)) { 
                regio = 0; for(int i=0; i<5; i++) regio_stage[i] = 1; berries = 1000; initTeam();
                state = 0; stateChanged = true; 
            }
        }
        else if (state == 2) { 
            if (stateChanged) { REG_DISPCNT = 0x0403; stateChanged = false; }
            if (updateBattle()) { 
                if (vijand_team[0].hp <= 0 && bBoss) { 
                    berries += (500 * regio_stage[regio]); itemAantal[0] += 1; regio_stage[regio]++; 
                    
                    if (regio == 4 && regio_stage[regio] > 5) {
                        state = 99; stateChanged = true; initOAM(); continue; 
                    }
                }
                if (bBoss) { worldX -= 15; interactie_npc = 0; }
                state = 1; stateChanged = true; initOAM(); 
            }
        }
    }
    return 0;
}