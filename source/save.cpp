#include "save.h"
#include <nds.h>
#include <fat.h>
#include <cstdio>
#include <cstring>

static bool g_fatInitialized = false;
char g_saveStatusMsg[64] = "Save Status: Ready";

static const char* SAVE_FILENAME = "fat:/bca_save.dat";
static const char* SAVE_FILENAME_ALT = "bca_save.dat";

static u32 calculateChecksum(const u8* data, size_t len) {
    u32 sum = 0x5A5A5A5A;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 5) + sum + data[i];
    }
    return sum;
}

bool initSaveSystem() {
    if (!g_fatInitialized) {
        g_fatInitialized = fatInitDefault();
    }
    return g_fatInitialized;
}

bool saveGame() {
    initSaveSystem();
    
    SavePayload payload;
    memset(&payload, 0, sizeof(payload));
    
    payload.totalChickenCount = g_gameState.totalChickenCount;
    payload.chickensPerPopup = g_gameState.chickensPerPopup;
    payload.upgradeLevel = g_gameState.upgradeLevel;
    payload.rebirthCount = g_gameState.rebirthCount;
    payload.rebirthChickens = g_gameState.rebirthChickens;
    
    payload.startWithChickens = g_gameState.rebirthUpgrades.startWithChickens ? 1 : 0;
    payload.extraMultiplier = g_gameState.rebirthUpgrades.extraMultiplier;
    payload.earlyChickenUpgrade = g_gameState.rebirthUpgrades.earlyChickenUpgrade ? 1 : 0;
    
    payload.hasAutoPopupClickUpgrade = g_gameState.hasAutoPopupClickUpgrade ? 1 : 0;
    payload.hasChickenBreastUnlock = g_gameState.hasChickenBreastUnlock ? 1 : 0;
    payload.cookCount = g_gameState.cookCount;
    
    payload.hasFactoryUnlock = g_gameState.hasFactoryUnlock ? 1 : 0;
    payload.factoryCount = g_gameState.factoryCount;
    
    payload.hasBankUnlock = g_gameState.hasBankUnlock ? 1 : 0;
    payload.bankChickenStored = g_gameState.bankChickenStored;
    payload.bankEfficiencyPercent = g_gameState.bankEfficiencyPercent;
    
    payload.clickPowerUpgradeLevel = g_gameState.clickPowerUpgradeLevel;
    payload.popupSpeedUpgradeLevel = g_gameState.popupSpeedUpgradeLevel;
    payload.cookCpsUpgradeLevel = g_gameState.cookCpsUpgradeLevel;
    payload.factoryCpsUpgradeLevel = g_gameState.factoryCpsUpgradeLevel;
    payload.bankDecayDelayUpgradeLevel = g_gameState.bankDecayDelayUpgradeLevel;
    payload.rareChanceUpgradeLevel = g_gameState.rareChanceUpgradeLevel;
    
    payload.manualChickenClicks = g_gameState.manualChickenClicks;
    payload.rarePopupShownCount = g_gameState.rarePopupShownCount;
    payload.totalCookProduced = g_gameState.totalCookProduced;
    payload.totalFactoryProduced = g_gameState.totalFactoryProduced;
    payload.totalBankDeposited = g_gameState.totalBankDeposited;
    
    SaveHeader header;
    header.magic = 0x31414342;
    header.version = 1;
    header.dataSize = sizeof(SavePayload);
    header.checksum = calculateChecksum((const u8*)&payload, sizeof(SavePayload));
    
    // Save to SD Card file via libfat
    bool fileSaved = false;
    if (g_fatInitialized) {
        FILE* f = fopen(SAVE_FILENAME, "wb");
        if (!f) f = fopen(SAVE_FILENAME_ALT, "wb");
        if (f) {
            fwrite(&header, sizeof(SaveHeader), 1, f);
            fwrite(&payload, sizeof(SavePayload), 1, f);
            fflush(f);
            fclose(f);
            fileSaved = true;
        }
    }
    
    if (fileSaved) {
        snprintf(g_saveStatusMsg, 64, "Saved OK!");
    } else {
        snprintf(g_saveStatusMsg, 64, "Save Ready");
    }
    
    return fileSaved;
}

bool loadGame() {
    initSaveSystem();
    
    if (!g_fatInitialized) {
        snprintf(g_saveStatusMsg, 64, "New Game");
        return false;
    }
    
    FILE* f = fopen(SAVE_FILENAME, "rb");
    if (!f) f = fopen(SAVE_FILENAME_ALT, "rb");
    if (!f) {
        snprintf(g_saveStatusMsg, 64, "New Game");
        return false;
    }
    
    SaveHeader header;
    if (fread(&header, sizeof(SaveHeader), 1, f) == 1 &&
        header.magic == 0x31414342 && header.version == 1 && header.dataSize == sizeof(SavePayload)) {
        SavePayload payload;
        if (fread(&payload, sizeof(SavePayload), 1, f) == 1) {
            fclose(f);
            u32 check = calculateChecksum((const u8*)&payload, sizeof(SavePayload));
            if (check == header.checksum) {
                g_gameState.totalChickenCount = payload.totalChickenCount;
                g_gameState.chickensPerPopup = payload.chickensPerPopup;
                g_gameState.upgradeLevel = payload.upgradeLevel;
                g_gameState.rebirthCount = payload.rebirthCount;
                g_gameState.rebirthChickens = payload.rebirthChickens;
                
                g_gameState.rebirthUpgrades.startWithChickens = (payload.startWithChickens != 0);
                g_gameState.rebirthUpgrades.extraMultiplier = payload.extraMultiplier;
                g_gameState.rebirthUpgrades.earlyChickenUpgrade = (payload.earlyChickenUpgrade != 0);
                
                g_gameState.hasAutoPopupClickUpgrade = (payload.hasAutoPopupClickUpgrade != 0);
                g_gameState.hasChickenBreastUnlock = (payload.hasChickenBreastUnlock != 0);
                g_gameState.cookCount = payload.cookCount;
                
                g_gameState.hasFactoryUnlock = (payload.hasFactoryUnlock != 0);
                g_gameState.factoryCount = payload.factoryCount;
                
                g_gameState.hasBankUnlock = (payload.hasBankUnlock != 0);
                g_gameState.bankChickenStored = payload.bankChickenStored;
                g_gameState.bankEfficiencyPercent = payload.bankEfficiencyPercent;
                
                g_gameState.clickPowerUpgradeLevel = payload.clickPowerUpgradeLevel;
                g_gameState.popupSpeedUpgradeLevel = payload.popupSpeedUpgradeLevel;
                g_gameState.cookCpsUpgradeLevel = payload.cookCpsUpgradeLevel;
                g_gameState.factoryCpsUpgradeLevel = payload.factoryCpsUpgradeLevel;
                g_gameState.bankDecayDelayUpgradeLevel = payload.bankDecayDelayUpgradeLevel;
                g_gameState.rareChanceUpgradeLevel = payload.rareChanceUpgradeLevel;
                
                g_gameState.manualChickenClicks = payload.manualChickenClicks;
                g_gameState.rarePopupShownCount = payload.rarePopupShownCount;
                g_gameState.totalCookProduced = payload.totalCookProduced;
                g_gameState.totalFactoryProduced = payload.totalFactoryProduced;
                g_gameState.totalBankDeposited = payload.totalBankDeposited;
                
                g_gameState.syncBouncingChickens();
                snprintf(g_saveStatusMsg, 64, "Loaded Save OK!");
                return true;
            }
        } else {
            fclose(f);
        }
    } else {
        fclose(f);
    }
    
    snprintf(g_saveStatusMsg, 64, "New Game");
    return false;
}
