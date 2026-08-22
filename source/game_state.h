#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <nds.h>
#include <cstdint>
#include <vector>

enum ChickenType {
    CHICKEN_WING = 0,
    CHICKEN_BREAST = 1,
    CHICKEN_NUGGET = 2
};

enum PopupTier {
    POPUP_NORMAL = 0,
    POPUP_RARE = 1,
    POPUP_EXTREME = 2
};

struct BouncingChicken {
    float x, y;
    float vx, vy;
    int size;
    ChickenType type;
};

struct FloatingText {
    float x, y;
    int value;
    float timer; // Countdown in seconds
};

struct RebirthUpgrades {
    bool startWithChickens = false;
    float extraMultiplier = 0.0f;
    bool earlyChickenUpgrade = false;
};

class GameState {
public:
    uint64_t totalChickenCount = 0;
    uint64_t chickensPerPopup = 1;
    int upgradeLevel = 1;
    
    bool hasAutoPopupClickUpgrade = false;
    bool isAutoClickerEnabled = true;
    bool isSoundEnabled = true;
    
    int rebirthCount = 0;
    uint64_t rebirthChickens = 0;
    RebirthUpgrades rebirthUpgrades;
    
    bool hasChickenBreastUnlock = false;
    int popupSpeedUpgradeLevel = 0;
    
    int cookCount = 0;
    int cookCpsUpgradeLevel = 0;
    
    bool hasFactoryUnlock = false;
    int factoryCount = 0;
    int factoryCpsUpgradeLevel = 0;
    
    bool hasBankUnlock = false;
    uint64_t bankChickenStored = 0;
    int bankEfficiencyPercent = 35;
    int bankDecayDelayUpgradeLevel = 0;
    
    int clickPowerUpgradeLevel = 0;
    int rareChanceUpgradeLevel = 0;
    
    uint64_t manualChickenClicks = 0;
    uint64_t rarePopupShownCount = 0;
    uint64_t totalCookProduced = 0;
    uint64_t totalFactoryProduced = 0;
    uint64_t totalBankDeposited = 0;

    // Active runtime state
    bool isPopupVisible = false;
    PopupTier activePopupTier = POPUP_NORMAL;
    float popupTimer = 0.0f;
    float popupX = 78.0f;
    float popupY = 76.0f;
    
    float cookCarryover = 0.0f;
    float factoryCarryover = 0.0f;
    float bankCarryover = 0.0f;
    
    std::vector<BouncingChicken> chickens;
    std::vector<FloatingText> floatingTexts;
    
    void init();
    void update(float dt);
    
    // Multipliers & Calculations
    float getRebirthMultiplier() const;
    int getClickPowerMultiplier() const;
    float getCookCpsMultiplier() const;
    float getFactoryCpsMultiplier() const;
    
    double getCookCPS() const;
    double getFactoryCPS() const;
    double getBankCPS() const;
    double getTotalCPS() const;
    
    // Upgrades Pricing & Gains
    uint64_t getUpgradeGain(int level) const;
    uint64_t getNextUpgradeGain() const;
    uint64_t getNextUpgradeCost() const;
    
    uint64_t getCookCost(int level) const;
    uint64_t getNextCookCost() const;
    
    uint64_t getFactoryCost(int level) const;
    uint64_t getNextFactoryCost() const;
    
    uint64_t getClickPowerUpgradeCost(int level) const;
    uint64_t getPopupSpeedUpgradeCost(int level) const;
    uint64_t getCookCpsUpgradeCost(int level) const;
    uint64_t getFactoryCpsUpgradeCost(int level) const;
    uint64_t getBankDelayUpgradeCost(int level) const;
    uint64_t getRareChanceUpgradeCost(int level) const;
    
    int getRebirthLevelRequirement() const;
    bool canAffordRebirth() const;
    
    float getPopupIntervalSeconds() const;
    float getRarePopupChance() const;
    
    // Actions
    bool buyMainUpgrade();
    bool buyAutoClicker();
    bool unlockChickenBreast();
    bool hireCook();
    bool buyCookCpsUpgrade();
    bool buyClickPowerUpgrade();
    bool buyPopupSpeedUpgrade();
    bool unlockFactory();
    bool buyFactory();
    bool buyFactoryCpsUpgrade();
    bool unlockBank();
    bool depositBank(uint64_t amount);
    bool withdrawBank(uint64_t amount);
    bool buyBankDelayUpgrade();
    bool buyRareChanceUpgrade();
    
    bool executeRebirth(int upgradeChoice);
    
    void triggerPopupClick();
    void tapChicken(int index, float touchX, float touchY);
    void spawnPopup();
    
    void addChickens(uint64_t amount);
    void syncBouncingChickens();
};

extern GameState g_gameState;

#endif // GAME_STATE_H
