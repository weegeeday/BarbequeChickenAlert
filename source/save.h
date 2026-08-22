#ifndef SAVE_H
#define SAVE_H

#include <nds.h>
#include "game_state.h"

struct SaveHeader {
    u32 magic = 0x31414342; // "BCA1"
    u32 version = 1;
    u32 dataSize = 0;
    u32 checksum = 0;
};

struct SavePayload {
    u64 totalChickenCount;
    u64 chickensPerPopup;
    u32 upgradeLevel;
    u32 rebirthCount;
    u64 rebirthChickens;
    
    u8 startWithChickens;
    float extraMultiplier;
    u8 earlyChickenUpgrade;
    
    u8 hasAutoPopupClickUpgrade;
    u8 hasChickenBreastUnlock;
    u32 cookCount;
    
    u8 hasFactoryUnlock;
    u32 factoryCount;
    
    u8 hasBankUnlock;
    u64 bankChickenStored;
    u32 bankEfficiencyPercent;
    
    u32 clickPowerUpgradeLevel;
    u32 popupSpeedUpgradeLevel;
    u32 cookCpsUpgradeLevel;
    u32 factoryCpsUpgradeLevel;
    u32 bankDecayDelayUpgradeLevel;
    u32 rareChanceUpgradeLevel;
    
    u64 manualChickenClicks;
    u64 rarePopupShownCount;
    u64 totalCookProduced;
    u64 totalFactoryProduced;
    u64 totalBankDeposited;
};

extern char g_saveStatusMsg[64];

bool initSaveSystem();
bool saveGame();
bool loadGame();

#endif // SAVE_H
