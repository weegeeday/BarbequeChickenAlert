#include <nds.h>
#include "game_state.h"
#include "graphics.h"
#include "audio.h"
#include "save.h"
#include <stdio.h>

int main() {
    // 1. Initialize Display Graphics
    initGraphics();
    
    // 2. Initialize Game State
    g_gameState.init();
    
    MenuTab currentTab = TAB_UPGRADES;
    int selectedIndex = 0;
    
    // 3. Render frame 0 immediately (Screens TURN ON INSTANTLY!)
    renderTopScreen(currentTab, selectedIndex);
    renderBottomScreen();
    swiWaitForVBlank();
    flushBottomScreen();
    
    // 4. Initialize Sound and Save System
    initAudio();
    initSaveSystem();
    loadGame();
    
    float autoSaveTimer = 60.0f;
    bool lastTouchState = false;
    
    while (pmMainLoop()) {
        scanKeys();
        u32 keysDownVal = keysDown();
        u32 keysHeldVal = keysHeld();
        
        touchPosition touch;
        touchRead(&touch);
        bool currentTouchState = (keysHeldVal & KEY_TOUCH) != 0;
        
        // Navigation Tabs (Left/Right, L/R)
        if (keysDownVal & (KEY_RIGHT | KEY_R)) {
            currentTab = (MenuTab)(((int)currentTab + 1) % 4);
        }
        if (keysDownVal & (KEY_LEFT | KEY_L)) {
            currentTab = (MenuTab)(((int)currentTab + 3) % 4);
        }
        
        // Navigation Options (Up/Down)
        if (currentTab == TAB_UPGRADES) {
            if (keysDownVal & KEY_DOWN) {
                selectedIndex = (selectedIndex + 1) % 13;
            }
            if (keysDownVal & KEY_UP) {
                selectedIndex = (selectedIndex + 12) % 13;
            }
        }
        
        // Action Button A
        if (keysDownVal & KEY_A) {
            if (currentTab == TAB_UPGRADES) {
                bool bought = false;
                switch (selectedIndex) {
                    case 0: bought = g_gameState.buyMainUpgrade(); break;
                    case 1: bought = g_gameState.buyAutoClicker(); break;
                    case 2: bought = g_gameState.unlockChickenBreast(); break;
                    case 3: bought = g_gameState.hireCook(); break;
                    case 4: bought = g_gameState.buyCookCpsUpgrade(); break;
                    case 5: bought = g_gameState.buyClickPowerUpgrade(); break;
                    case 6: bought = g_gameState.buyPopupSpeedUpgrade(); break;
                    case 7: bought = g_gameState.unlockFactory(); break;
                    case 8: bought = g_gameState.buyFactory(); break;
                    case 9: bought = g_gameState.buyFactoryCpsUpgrade(); break;
                    case 10: bought = g_gameState.unlockBank(); break;
                    case 11: bought = g_gameState.buyBankDelayUpgrade(); break;
                    case 12: bought = g_gameState.buyRareChanceUpgrade(); break;
                }
                if (bought) {
                    playPurchaseSound();
                    saveGame();
                }
            } else if (currentTab == TAB_REBIRTH) {
                if (g_gameState.executeRebirth(1)) {
                    playRarePopupSound();
                    saveGame();
                }
            } else if (currentTab == TAB_STATS) {
                saveGame();
            }
        }
        
        // SELECT Button: Save Game
        if (keysDownVal & KEY_SELECT) {
            saveGame();
        }
        
        // Bank Deposit (X) / Withdraw (Y)
        if (currentTab == TAB_BANK) {
            if (keysDownVal & KEY_X) {
                uint64_t dep = g_gameState.totalChickenCount / 10;
                if (dep < 1 && g_gameState.totalChickenCount > 0) dep = 1;
                if (g_gameState.depositBank(dep)) playPurchaseSound();
            }
            if (keysDownVal & KEY_Y) {
                uint64_t wd = g_gameState.bankChickenStored / 10;
                if (wd < 1 && g_gameState.bankChickenStored > 0) wd = 1;
                if (g_gameState.withdrawBank(wd)) playPurchaseSound();
            }
        }
        
        // Touch Screen Interaction
        if (currentTouchState && !lastTouchState) { // Touch Pressed Event
            bool touchedPopup = false;
            
            // Check Popup Banner Touch (dynamic popupX, popupY, 100x40)
            if (g_gameState.isPopupVisible) {
                if (touch.px >= g_gameState.popupX && touch.px <= g_gameState.popupX + 100 &&
                    touch.py >= g_gameState.popupY && touch.py <= g_gameState.popupY + 40) {
                    if (g_gameState.activePopupTier == POPUP_RARE || g_gameState.activePopupTier == POPUP_EXTREME) {
                        playRarePopupSound();
                    } else {
                        playPopupSound();
                    }
                    g_gameState.triggerPopupClick();
                    touchedPopup = true;
                }
            }
            
            // Check Bouncing Chicken Touch
            if (!touchedPopup) {
                for (size_t i = 0; i < g_gameState.chickens.size(); i++) {
                    auto& c = g_gameState.chickens[i];
                    if (touch.px >= c.x && touch.px <= c.x + c.size &&
                        touch.py >= c.y && touch.py <= c.y + c.size) {
                        playChickenSound();
                        g_gameState.tapChicken(i, touch.px, touch.py);
                        break;
                    }
                }
            }
        }
        lastTouchState = currentTouchState;
        
        // Update Game Engine Logic
        g_gameState.update(1.0f / 60.0f);
        
        // 1-Minute Auto-Save Timer
        autoSaveTimer -= (1.0f / 60.0f);
        if (autoSaveTimer <= 0.0f) {
            autoSaveTimer = 60.0f;
            saveGame();
        }
        
        // Render dual screens
        renderTopScreen(currentTab, selectedIndex);
        renderBottomScreen();
        
        swiWaitForVBlank();
        flushBottomScreen();
    }
    
    return 0;
}
