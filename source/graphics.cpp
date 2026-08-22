#include "graphics.h"
#include "game_state.h"
#include "gfx_assets.h"
#include "save.h"
#include "lang.h"
#include <stdio.h>
#include <string.h>

static int bgSub;
static u16* subGfxBuffer = nullptr;
alignas(32) static u16 subOffscreenBuffer[256 * 256];
static PrintConsole topConsole;

// 3x5 Pixel Font for floating numbers: '0'-'9', '+'
static const u8 font3x5[11][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, // '0'
    {0x2, 0x6, 0x2, 0x2, 0x7}, // '1'
    {0x7, 0x1, 0x7, 0x4, 0x7}, // '2'
    {0x7, 0x1, 0x7, 0x1, 0x7}, // '3'
    {0x5, 0x5, 0x7, 0x1, 0x1}, // '4'
    {0x7, 0x4, 0x7, 0x1, 0x7}, // '5'
    {0x7, 0x4, 0x7, 0x5, 0x7}, // '6'
    {0x7, 0x1, 0x2, 0x4, 0x4}, // '7'
    {0x7, 0x5, 0x7, 0x5, 0x7}, // '8'
    {0x7, 0x5, 0x7, 0x1, 0x7}, // '9'
    {0x0, 0x2, 0x7, 0x2, 0x0}, // '+'
};

void initGraphics() {
    // Top Screen: Mode 0 2D with Console
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    consoleInit(&topConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    
    // Bottom Screen: Mode 5 2D Bitmap (15-bit RGB555)
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    bgSub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    subGfxBuffer = (u16*)bgGetGfxPtr(bgSub);
    
    // Clear entire VRAM framebuffer memory
    u16 blackPixel = ARGB16(1, 0, 0, 0);
    for (int i = 0; i < 256 * 256; i++) {
        subGfxBuffer[i] = blackPixel;
        subOffscreenBuffer[i] = blackPixel;
    }
    DC_FlushRange(subOffscreenBuffer, sizeof(subOffscreenBuffer));
}

// Drawing helper functions for offscreen buffer
static void drawPixel(int x, int y, u16 color) {
    if (x < 0 || x >= 256 || y < 0 || y >= 192) return;
    subOffscreenBuffer[y * 256 + x] = color;
}

static void drawSprite32(int x, int y, const u16* gfxData) {
    for (int sy = 0; sy < 32; sy++) {
        int py = y + sy;
        if (py < 0 || py >= 192) continue;
        for (int sx = 0; sx < 32; sx++) {
            int px = x + sx;
            if (px < 0 || px >= 256) continue;
            u16 pixel = gfxData[sy * 32 + sx];
            if (pixel & 0x8000) { // Alpha bit set
                subOffscreenBuffer[py * 256 + px] = pixel;
            }
        }
    }
}

static void drawSprite100x40(int x, int y, const u16* gfxData) {
    for (int sy = 0; sy < 40; sy++) {
        int py = y + sy;
        if (py < 0 || py >= 192) continue;
        for (int sx = 0; sx < 100; sx++) {
            int px = x + sx;
            if (px < 0 || px >= 256) continue;
            u16 pixel = gfxData[sy * 100 + sx];
            if (pixel & 0x8000) { // Alpha bit set
                subOffscreenBuffer[py * 256 + px] = pixel;
            }
        }
    }
}

static void drawPixelChar(int x, int y, char ch, u16 color, int scale = 2) {
    int idx = 10; // '+'
    if (ch >= '0' && ch <= '9') idx = ch - '0';
    if (ch == '+') idx = 10;
    
    const u8* rows = font3x5[idx];
    for (int r = 0; r < 5; r++) {
        u8 rowVal = rows[r];
        for (int c = 0; c < 3; c++) {
            if (rowVal & (1 << (2 - c))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        drawPixel(x + c * scale + sx, y + r * scale + sy, color);
                    }
                }
            }
        }
    }
}

static void drawPixelString(int x, int y, const char* str, u16 color, int scale = 2) {
    int curX = x;
    for (int i = 0; str[i] != '\0'; i++) {
        drawPixelChar(curX, y, str[i], color, scale);
        curX += (3 * scale + scale);
    }
}

void renderBottomScreen() {
    // Solid pitch black background color (0x8000 | RGB15(0, 0, 0))
    u16 bgCol = ARGB16(1, 0, 0, 0);
    for (int i = 0; i < 256 * 192; i++) {
        subOffscreenBuffer[i] = bgCol;
    }
    
    // Draw bouncing chickens (32x32 compact, up to 30)
    for (const auto& c : g_gameState.chickens) {
        const u16* gfx = wing_gfx;
        if (c.type == CHICKEN_BREAST) gfx = breast_gfx;
        if (c.type == CHICKEN_NUGGET) gfx = nugget_gfx;
        
        drawSprite32((int)c.x, (int)c.y, gfx);
    }
    
    // Draw Actual Alert Banner Texture (BCAG.png, 100x40 at randomized popupX, popupY)
    if (g_gameState.isPopupVisible) {
        int boxX = (int)g_gameState.popupX;
        int boxY = (int)g_gameState.popupY;
        
        drawSprite100x40(boxX, boxY, popup_gfx);
    }
    
    // Draw Floating numbers (+1, +2, +10) above tapped chickens
    for (const auto& ft : g_gameState.floatingTexts) {
        int fx = (int)ft.x;
        int fy = (int)ft.y;
        char numStr[16];
        snprintf(numStr, 16, "+%d", ft.value);
        
        // Draw black background outline for text legibility
        drawPixelString(fx - 1, fy - 1, numStr, ARGB16(1, 0, 0, 0), 2);
        drawPixelString(fx + 1, fy + 1, numStr, ARGB16(1, 0, 0, 0), 2);
        
        // Draw bright yellow text
        drawPixelString(fx, fy, numStr, ARGB16(1, 31, 31, 0), 2);
    }
}

void flushBottomScreen() {
    // 1. Crucial for physical NDS hardware: Flush CPU Data Cache to RAM before DMA
    DC_FlushRange(subOffscreenBuffer, 256 * 192 * 2);
    
    // 2. Hardware DMA Copy offscreen buffer to live VRAM in < 0.1ms during VBlank
    dmaCopy(subOffscreenBuffer, subGfxBuffer, 256 * 192 * 2);
}

void renderTopScreen(MenuTab currentTab, int selectedIndex) {
    consoleSelect(&topConsole);
    
    // Every row is padded to exactly 30 columns to NEVER trigger console line wrapping!
    printf("\x1b[1;1H------------------------------");
    printf("\x1b[2;1H%s", STR_TITLE);
    printf("\x1b[3;1H------------------------------");
    
    char lineBuf[64];
    snprintf(lineBuf, 64, STR_CHICKENS, (unsigned long long)g_gameState.totalChickenCount);
    printf("\x1b[4;1H\x1b[32;1m%-30s\x1b[0m", lineBuf);
    
    snprintf(lineBuf, 64, STR_CPS_REBIRTH, 
             g_gameState.getTotalCPS(), g_gameState.rebirthCount, g_gameState.getRebirthMultiplier());
    printf("\x1b[5;1H\x1b[36;1m%-30s\x1b[0m", lineBuf);
    
    printf("\x1b[6;1H------------------------------");
    
    // Clean Tab Bar (29 columns wide)
    const char* t1 = (currentTab == TAB_UPGRADES) ? "\x1b[47;30m " STR_TAB_UPG " \x1b[0m" : " " STR_TAB_UPG " ";
    const char* t2 = (currentTab == TAB_BANK)     ? "\x1b[47;30m " STR_TAB_BANK " \x1b[0m" : " " STR_TAB_BANK " ";
    const char* t3 = (currentTab == TAB_REBIRTH)  ? "\x1b[47;30m " STR_TAB_REB " \x1b[0m" : " " STR_TAB_REB " ";
    const char* t4 = (currentTab == TAB_STATS)    ? "\x1b[47;30m " STR_TAB_STAT " \x1b[0m" : " " STR_TAB_STAT " ";
    printf("\x1b[7;1H%s%s%s%s", t1, t2, t3, t4);
    printf("\x1b[8;1H------------------------------");
    
    // WIPE ROWS 9..16 CLEAN BEFORE RENDERING ANY TAB CONTENT
    for (int r = 9; r <= 16; r++) {
        printf("\x1b[%d;1H                              ", r);
    }
    
    // Rows 9..16: Content Area (4 items, 2 lines per item = 8 lines)
    if (currentTab == TAB_UPGRADES) {
        struct UpgradeItem {
            const char* title;
            uint64_t cost;
            bool bought;
            bool unlocked;
            char info[24];
        };
        
        UpgradeItem items[13];
        
        items[0] = {STR_UPG_0, g_gameState.getNextUpgradeCost(), false, true, ""};
        snprintf(items[0].info, 24, STR_UPG_0_INFO, (unsigned long long)g_gameState.getNextUpgradeGain());
        
        items[1] = {STR_UPG_1, 1000, g_gameState.hasAutoPopupClickUpgrade, true, STR_UPG_1_INFO};
        items[2] = {STR_UPG_2, 200, g_gameState.hasChickenBreastUnlock, true, STR_UPG_2_INFO};
        
        items[3] = {STR_UPG_3, g_gameState.getNextCookCost(), false, true, ""};
        snprintf(items[3].info, 24, STR_UPG_3_INFO, g_gameState.cookCount);
        
        items[4] = {STR_UPG_4, g_gameState.getCookCpsUpgradeCost(g_gameState.cookCpsUpgradeLevel + 1), false, g_gameState.cookCount > 0, ""};
        snprintf(items[4].info, 24, STR_UPG_4_INFO, g_gameState.cookCpsUpgradeLevel);
        
        items[5] = {STR_UPG_5, g_gameState.getClickPowerUpgradeCost(g_gameState.clickPowerUpgradeLevel + 1), false, true, ""};
        snprintf(items[5].info, 24, STR_UPG_5_INFO, g_gameState.clickPowerUpgradeLevel);
        
        items[6] = {STR_UPG_6, g_gameState.getPopupSpeedUpgradeCost(g_gameState.popupSpeedUpgradeLevel + 1), false, true, ""};
        snprintf(items[6].info, 24, STR_UPG_6_INFO, g_gameState.popupSpeedUpgradeLevel);
        
        items[7] = {STR_UPG_7, 300, g_gameState.hasFactoryUnlock, true, STR_UPG_7_INFO};
        
        items[8] = {STR_UPG_8, g_gameState.getNextFactoryCost(), false, g_gameState.hasFactoryUnlock, ""};
        snprintf(items[8].info, 24, STR_UPG_8_INFO, g_gameState.factoryCount);
        
        items[9] = {STR_UPG_9, g_gameState.getFactoryCpsUpgradeCost(g_gameState.factoryCpsUpgradeLevel + 1), false, g_gameState.factoryCount > 0, ""};
        snprintf(items[9].info, 24, STR_UPG_9_INFO, g_gameState.factoryCpsUpgradeLevel);
        
        items[10] = {STR_UPG_10, 5000, g_gameState.hasBankUnlock, true, STR_UPG_10_INFO};
        
        items[11] = {STR_UPG_11, g_gameState.getBankDelayUpgradeCost(g_gameState.bankDecayDelayUpgradeLevel + 1), false, g_gameState.hasBankUnlock, ""};
        snprintf(items[11].info, 24, STR_UPG_11_INFO, g_gameState.bankDecayDelayUpgradeLevel);
        
        items[12] = {STR_UPG_12, g_gameState.getRareChanceUpgradeCost(g_gameState.rareChanceUpgradeLevel + 1), false, true, ""};
        snprintf(items[12].info, 24, STR_UPG_12_INFO, g_gameState.rareChanceUpgradeLevel);
        
        // Scroll window calculation: 4 items shown at a time
        int startIdx = selectedIndex - 1;
        if (startIdx < 0) startIdx = 0;
        if (startIdx > 13 - 4) startIdx = 13 - 4;
        
        for (int i = 0; i < 4; i++) {
            int itemIdx = startIdx + i;
            int r1 = 9 + i * 2;
            int r2 = r1 + 1;
            
            bool isSelected = (itemIdx == selectedIndex);
            const char* prefix = isSelected ? " > " : "   ";
            
            if (!items[itemIdx].unlocked) {
                snprintf(lineBuf, 64, "%s" STR_LOCKED, prefix);
                printf("\x1b[%d;1H\x1b[30;1m%-30s\x1b[0m", r1, lineBuf);
                printf("\x1b[%d;1H     %-25s", r2, STR_LOCKED);
            } else if (items[itemIdx].bought) {
                snprintf(lineBuf, 64, "%s%s", prefix, items[itemIdx].title);
                printf("\x1b[%d;1H\x1b[32;1m%-30s\x1b[0m", r1, lineBuf);
                printf("\x1b[%d;1H     \x1b[32m%-25s\x1b[0m", r2, STR_OWNED);
            } else {
                bool canAfford = (g_gameState.totalChickenCount >= items[itemIdx].cost);
                const char* titleCol = isSelected ? "\x1b[33;1m" : (canAfford ? "\x1b[32m" : "\x1b[37m");
                
                snprintf(lineBuf, 64, "%s%s", prefix, items[itemIdx].title);
                printf("\x1b[%d;1H%s%-30s\x1b[0m", r1, titleCol, lineBuf);
                
                char line2Buf[64];
                snprintf(line2Buf, 64, STR_COST_FMT, (unsigned long long)items[itemIdx].cost, items[itemIdx].info);
                printf("\x1b[%d;1H     %-25s", r2, line2Buf);
            }
        }
    } else if (currentTab == TAB_BANK) {
        printf("\x1b[9;1H\x1b[33;1m%s\x1b[0m", STR_BANK_TITLE);
        if (!g_gameState.hasBankUnlock) {
            printf("\x1b[11;1H%s", STR_BANK_LOCKED_1);
            printf("\x1b[12;1H%s", STR_BANK_LOCKED_2);
            printf("\x1b[13;1H%s", STR_BANK_LOCKED_3);
        } else {
            snprintf(lineBuf, 64, STR_BANK_STORED, (unsigned long long)g_gameState.bankChickenStored);
            printf("\x1b[11;1H\x1b[32;1m%-30s\x1b[0m", lineBuf);
            
            snprintf(lineBuf, 64, STR_BANK_RATE, g_gameState.getBankCPS(), g_gameState.bankEfficiencyPercent);
            printf("\x1b[12;1H\x1b[36;1m%-30s\x1b[0m", lineBuf);
            
            printf("\x1b[14;1H%s", STR_BANK_CTRL_HEADER);
            printf("\x1b[15;1H%s", STR_BANK_CTRL_DEP);
            printf("\x1b[16;1H%s", STR_BANK_CTRL_WD);
        }
    } else if (currentTab == TAB_REBIRTH) {
        printf("\x1b[9;1H\x1b[35;1m%s\x1b[0m", STR_REB_TITLE);
        snprintf(lineBuf, 64, STR_REB_COUNT, g_gameState.rebirthCount);
        printf("\x1b[11;1H%-30s", lineBuf);
        
        snprintf(lineBuf, 64, STR_REB_MULTI, g_gameState.getRebirthMultiplier());
        printf("\x1b[12;1H\x1b[33;1m%-30s\x1b[0m", lineBuf);
        
        int req = g_gameState.getRebirthLevelRequirement();
        snprintf(lineBuf, 64, STR_REB_REQ, req, g_gameState.upgradeLevel);
        printf("\x1b[13;1H%-30s", lineBuf);
        
        uint64_t gainRC = (g_gameState.totalChickenCount / 2) / 10000;
        snprintf(lineBuf, 64, STR_REB_GAIN, (unsigned long long)gainRC);
        printf("\x1b[14;1H\x1b[35;1m%-30s\x1b[0m", lineBuf);
        
        if (g_gameState.canAffordRebirth()) {
            printf("\x1b[16;1H%s", STR_REB_READY);
        } else {
            snprintf(lineBuf, 64, STR_REB_NOT_READY, req);
            printf("\x1b[16;1H\x1b[31m%-30s\x1b[0m", lineBuf);
        }
    } else if (currentTab == TAB_STATS) {
        printf("\x1b[9;1H\x1b[36;1m%s\x1b[0m", STR_STAT_TITLE);
        snprintf(lineBuf, 64, STR_STAT_TAPS, (unsigned long long)g_gameState.manualChickenClicks);
        printf("\x1b[11;1H%-30s", lineBuf);
        
        snprintf(lineBuf, 64, STR_STAT_RARE, (unsigned long long)g_gameState.rarePopupShownCount);
        printf("\x1b[12;1H%-30s", lineBuf);
        
        snprintf(lineBuf, 64, STR_STAT_COOK, (unsigned long long)g_gameState.totalCookProduced);
        printf("\x1b[13;1H%-30s", lineBuf);
        
        snprintf(lineBuf, 64, STR_STAT_FACTORY, (unsigned long long)g_gameState.totalFactoryProduced);
        printf("\x1b[14;1H%-30s", lineBuf);
        
        snprintf(lineBuf, 64, STR_STAT_STATUS, g_saveStatusMsg);
        printf("\x1b[16;1H%-30s", lineBuf);
    }
    
    // Rows 17..20: Footer Controls
    printf("\x1b[17;1H------------------------------");
    printf("\x1b[18;1H%s", STR_FOOTER_1);
    printf("\x1b[19;1H%s", STR_FOOTER_2);
    printf("\x1b[20;1H------------------------------");
}
