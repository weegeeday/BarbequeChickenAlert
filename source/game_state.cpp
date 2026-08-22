#include "game_state.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

GameState g_gameState;

static float randFloat(float minVal, float maxVal) {
    float r = (float)rand() / (float)RAND_MAX;
    return minVal + r * (maxVal - minVal);
}

void GameState::init() {
    popupTimer = getPopupIntervalSeconds();
    syncBouncingChickens();
}

float GameState::getRebirthMultiplier() const {
    return 1.0f + 0.5f * rebirthCount + rebirthUpgrades.extraMultiplier;
}

int GameState::getClickPowerMultiplier() const {
    return 1 + clickPowerUpgradeLevel;
}

float GameState::getCookCpsMultiplier() const {
    return 1.0f + cookCpsUpgradeLevel * 0.25f;
}

float GameState::getFactoryCpsMultiplier() const {
    return 1.0f + factoryCpsUpgradeLevel * 0.25f;
}

double GameState::getCookCPS() const {
    return cookCount * (1.0 + rebirthCount) * getCookCpsMultiplier();
}

double GameState::getFactoryCPS() const {
    return factoryCount * 2.0 * getRebirthMultiplier() * getFactoryCpsMultiplier();
}

double GameState::getBankCPS() const {
    return bankChickenStored * (bankEfficiencyPercent / 100.0) * 0.01;
}

double GameState::getTotalCPS() const {
    return getCookCPS() + getFactoryCPS() + getBankCPS();
}

uint64_t GameState::getUpgradeGain(int level) const {
    double baseCpp = std::floor(std::pow(level, 2.5) / 3.0 + 1.0 + 1e-9);
    return std::max<uint64_t>(1, (uint64_t)std::floor(baseCpp * getRebirthMultiplier() + 1e-9));
}

uint64_t GameState::getNextUpgradeGain() const {
    return getUpgradeGain(upgradeLevel + 1);
}

uint64_t GameState::getNextUpgradeCost() const {
    return std::max<uint64_t>(1, (uint64_t)std::floor(1.5 * std::pow(upgradeLevel, 3.0) + 1.0 + 1e-9));
}

uint64_t GameState::getCookCost(int level) const {
    return std::max<uint64_t>(125, (uint64_t)std::floor(1.5 * std::pow(level, 3.0) + 1.0 + 1e-9));
}

uint64_t GameState::getNextCookCost() const {
    return getCookCost(cookCount + 1);
}

uint64_t GameState::getFactoryCost(int level) const {
    return std::max<uint64_t>(300, (uint64_t)std::floor(1.5 * std::pow(level, 3.0) + 1.0 + 1e-9));
}

uint64_t GameState::getNextFactoryCost() const {
    return getFactoryCost(factoryCount + 1);
}

uint64_t GameState::getClickPowerUpgradeCost(int level) const {
    return std::max<uint64_t>(80, (uint64_t)std::floor(65.0 * std::pow(level, 2.1) + 1.0));
}

uint64_t GameState::getPopupSpeedUpgradeCost(int level) const {
    return std::max<uint64_t>(125, (uint64_t)std::floor(120.0 * std::pow(level, 2.2) + 1.0));
}

uint64_t GameState::getCookCpsUpgradeCost(int level) const {
    return std::max<uint64_t>(180, (uint64_t)std::floor(165.0 * std::pow(level, 2.25) + 1.0));
}

uint64_t GameState::getFactoryCpsUpgradeCost(int level) const {
    return std::max<uint64_t>(240, (uint64_t)std::floor(220.0 * std::pow(level, 2.25) + 1.0));
}

uint64_t GameState::getBankDelayUpgradeCost(int level) const {
    return std::max<uint64_t>(260, (uint64_t)std::floor(230.0 * std::pow(level, 2.3) + 1.0));
}

uint64_t GameState::getRareChanceUpgradeCost(int level) const {
    return std::max<uint64_t>(320, (uint64_t)std::floor(280.0 * std::pow(level, 2.4) + 1.0));
}

int GameState::getRebirthLevelRequirement() const {
    return 10 + rebirthCount * 10;
}

bool GameState::canAffordRebirth() const {
    return upgradeLevel >= getRebirthLevelRequirement();
}

float GameState::getPopupIntervalSeconds() const {
    return std::max(1.0f, 5.0f - popupSpeedUpgradeLevel * 0.1f);
}

float GameState::getRarePopupChance() const {
    float bonus = rareChanceUpgradeLevel * 0.005f;
    return std::min(0.75f, 0.02f + bonus);
}

void GameState::addChickens(uint64_t amount) {
    totalChickenCount += amount;
    syncBouncingChickens();
}

void GameState::syncBouncingChickens() {
    int targetCount = (int)std::min<uint64_t>(30, totalChickenCount);
    
    while ((int)chickens.size() < targetCount) {
        BouncingChicken c;
        c.size = 32;
        c.x = randFloat(0, 256 - c.size);
        c.y = randFloat(0, 192 - c.size);
        c.vx = randFloat(-60, 60);
        c.vy = randFloat(-60, 60);
        if (std::abs(c.vx) < 25) c.vx = 40;
        if (std::abs(c.vy) < 25) c.vy = -40;
        
        int typeRoll = rand() % 10;
        if (hasFactoryUnlock && typeRoll < 3) {
            c.type = CHICKEN_NUGGET;
        } else if (hasChickenBreastUnlock && typeRoll < 6) {
            c.type = CHICKEN_BREAST;
        } else {
            c.type = CHICKEN_WING;
        }
        chickens.push_back(c);
    }
    
    while ((int)chickens.size() > targetCount) {
        chickens.pop_back();
    }
}

void GameState::spawnPopup() {
    isPopupVisible = true;
    popupX = randFloat(10.0f, 146.0f);
    popupY = randFloat(10.0f, 142.0f);
    
    float roll = randFloat(0.0f, 1.0f);
    if (roll < 0.0001f) {
        activePopupTier = POPUP_EXTREME;
        rarePopupShownCount++;
    } else if (roll < 0.0001f + getRarePopupChance()) {
        activePopupTier = POPUP_RARE;
        rarePopupShownCount++;
    } else {
        activePopupTier = POPUP_NORMAL;
    }
    
    if (hasAutoPopupClickUpgrade && isAutoClickerEnabled) {
        triggerPopupClick();
    }
}

void GameState::triggerPopupClick() {
    if (!isPopupVisible) return;
    
    uint64_t mult = 1;
    if (activePopupTier == POPUP_RARE) mult = 10;
    if (activePopupTier == POPUP_EXTREME) mult = 100;
    
    uint64_t gain = std::floor(chickensPerPopup * mult * getRebirthMultiplier());
    if (gain < 1) gain = 1;
    
    addChickens(gain);
    isPopupVisible = false;
    popupTimer = getPopupIntervalSeconds();
}

void GameState::tapChicken(int index, float touchX, float touchY) {
    if (index < 0 || index >= (int)chickens.size()) return;
    
    uint64_t clickVal = 1;
    if (chickens[index].type == CHICKEN_BREAST) clickVal = 2;
    if (chickens[index].type == CHICKEN_NUGGET) clickVal = 3;
    
    clickVal *= getClickPowerMultiplier();
    manualChickenClicks++;
    addChickens(clickVal);
    
    FloatingText ft;
    ft.x = touchX;
    ft.y = touchY;
    ft.value = (int)clickVal;
    ft.timer = 1.0f; // 1 second lifetime
    floatingTexts.push_back(ft);
}

void GameState::update(float dt) {
    // 1. Production CPS
    double cookCps = getCookCPS();
    if (cookCps > 0) {
        cookCarryover += cookCps * dt;
        uint64_t added = (uint64_t)cookCarryover;
        if (added > 0) {
            cookCarryover -= added;
            totalChickenCount += added;
            totalCookProduced += added;
        }
    }
    
    double factoryCps = getFactoryCPS();
    if (factoryCps > 0) {
        factoryCarryover += factoryCps * dt;
        uint64_t added = (uint64_t)factoryCarryover;
        if (added > 0) {
            factoryCarryover -= added;
            totalChickenCount += added;
            totalFactoryProduced += added;
        }
    }
    
    double bankCps = getBankCPS();
    if (bankCps > 0) {
        bankCarryover += bankCps * dt;
        uint64_t added = (uint64_t)bankCarryover;
        if (added > 0) {
            bankCarryover -= added;
            totalChickenCount += added;
        }
    }
    
    // 2. Popup timer
    if (!isPopupVisible) {
        popupTimer -= dt;
        if (popupTimer <= 0.0f) {
            spawnPopup();
        }
    }
    
    // 3. Bouncing chicken physics
    for (auto& c : chickens) {
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        
        if (c.x <= 0) { c.x = 0; c.vx = -c.vx; }
        if (c.x >= 256 - c.size) { c.x = 256 - c.size; c.vx = -c.vx; }
        if (c.y <= 0) { c.y = 0; c.vy = -c.vy; }
        if (c.y >= 192 - c.size) { c.y = 192 - c.size; c.vy = -c.vy; }
    }
    
    // 4. Floating texts update
    for (size_t i = 0; i < floatingTexts.size();) {
        floatingTexts[i].y -= 20.0f * dt;
        floatingTexts[i].timer -= dt;
        if (floatingTexts[i].timer <= 0.0f) {
            floatingTexts.erase(floatingTexts.begin() + i);
        } else {
            i++;
        }
    }
}

// Upgrade Buyers
bool GameState::buyMainUpgrade() {
    uint64_t cost = getNextUpgradeCost();
    if (totalChickenCount >= cost) {
        totalChickenCount -= cost;
        chickensPerPopup = getNextUpgradeGain();
        upgradeLevel++;
        syncBouncingChickens();
        return true;
    }
    return false;
}

bool GameState::buyAutoClicker() {
    if (hasAutoPopupClickUpgrade || totalChickenCount < 1000) return false;
    totalChickenCount -= 1000;
    hasAutoPopupClickUpgrade = true;
    return true;
}

bool GameState::unlockChickenBreast() {
    if (hasChickenBreastUnlock || totalChickenCount < 200) return false;
    totalChickenCount -= 200;
    hasChickenBreastUnlock = true;
    syncBouncingChickens();
    return true;
}

bool GameState::hireCook() {
    uint64_t cost = getNextCookCost();
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    cookCount++;
    bankEfficiencyPercent = std::min(90, bankEfficiencyPercent + 1);
    return true;
}

bool GameState::buyCookCpsUpgrade() {
    if (cookCpsUpgradeLevel >= 5) return false;
    uint64_t cost = getCookCpsUpgradeCost(cookCpsUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    cookCpsUpgradeLevel++;
    return true;
}

bool GameState::buyClickPowerUpgrade() {
    if (clickPowerUpgradeLevel >= 10) return false;
    uint64_t cost = getClickPowerUpgradeCost(clickPowerUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    clickPowerUpgradeLevel++;
    return true;
}

bool GameState::buyPopupSpeedUpgrade() {
    uint64_t cost = getPopupSpeedUpgradeCost(popupSpeedUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    popupSpeedUpgradeLevel++;
    return true;
}

bool GameState::unlockFactory() {
    if (hasFactoryUnlock || totalChickenCount < 300) return false;
    totalChickenCount -= 300;
    hasFactoryUnlock = true;
    syncBouncingChickens();
    return true;
}

bool GameState::buyFactory() {
    uint64_t cost = getNextFactoryCost();
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    factoryCount++;
    return true;
}

bool GameState::buyFactoryCpsUpgrade() {
    if (factoryCpsUpgradeLevel >= 5) return false;
    uint64_t cost = getFactoryCpsUpgradeCost(factoryCpsUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    factoryCpsUpgradeLevel++;
    return true;
}

bool GameState::unlockBank() {
    if (hasBankUnlock || totalChickenCount < 5000) return false;
    totalChickenCount -= 5000;
    hasBankUnlock = true;
    return true;
}

bool GameState::depositBank(uint64_t amount) {
    uint64_t dep = std::min(amount, totalChickenCount);
    if (dep == 0) return false;
    totalChickenCount -= dep;
    bankChickenStored += dep;
    totalBankDeposited += dep;
    return true;
}

bool GameState::withdrawBank(uint64_t amount) {
    uint64_t wd = std::min(amount, bankChickenStored);
    if (wd == 0) return false;
    bankChickenStored -= wd;
    totalChickenCount += wd;
    return true;
}

bool GameState::buyBankDelayUpgrade() {
    if (bankDecayDelayUpgradeLevel >= 5) return false;
    uint64_t cost = getBankDelayUpgradeCost(bankDecayDelayUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    bankDecayDelayUpgradeLevel++;
    return true;
}

bool GameState::buyRareChanceUpgrade() {
    if (rareChanceUpgradeLevel >= 3) return false;
    uint64_t cost = getRareChanceUpgradeCost(rareChanceUpgradeLevel + 1);
    if (totalChickenCount < cost) return false;
    totalChickenCount -= cost;
    rareChanceUpgradeLevel++;
    return true;
}

bool GameState::executeRebirth(int upgradeChoice) {
    if (!canAffordRebirth()) return false;
    
    uint64_t chickensForRebirth = totalChickenCount / 2;
    uint64_t gainedRC = chickensForRebirth / 10000;
    
    rebirthChickens += gainedRC;
    rebirthCount++;
    
    // Apply rebirth upgrade choice: 1 = +0.5x mult, 2 = start with chickens, 3 = early chicken upgrade
    if (upgradeChoice == 1 && rebirthChickens >= (1 + (uint64_t)rebirthCount * 2)) {
        rebirthChickens -= (1 + rebirthCount * 2);
        rebirthUpgrades.extraMultiplier += 0.5f;
    } else if (upgradeChoice == 2 && !rebirthUpgrades.startWithChickens && rebirthChickens >= 100) {
        rebirthChickens -= 100;
        rebirthUpgrades.startWithChickens = true;
    } else if (upgradeChoice == 3 && !rebirthUpgrades.earlyChickenUpgrade && rebirthChickens >= 30) {
        rebirthChickens -= 30;
        rebirthUpgrades.earlyChickenUpgrade = true;
    }
    
    uint64_t startingChickens = 0;
    if (rebirthUpgrades.startWithChickens) {
        startingChickens = chickensForRebirth * 5 / 10000;
    }
    
    totalChickenCount = startingChickens;
    chickensPerPopup = (rebirthUpgrades.earlyChickenUpgrade) ? getUpgradeGain(2) : 1;
    upgradeLevel = (rebirthUpgrades.earlyChickenUpgrade) ? 2 : 1;
    
    hasAutoPopupClickUpgrade = false;
    hasChickenBreastUnlock = rebirthUpgrades.earlyChickenUpgrade;
    hasBankUnlock = false;
    bankChickenStored = 0;
    hasFactoryUnlock = rebirthUpgrades.earlyChickenUpgrade;
    factoryCount = 0;
    popupSpeedUpgradeLevel = 0;
    cookCount = 0;
    clickPowerUpgradeLevel = 0;
    cookCpsUpgradeLevel = 0;
    factoryCpsUpgradeLevel = 0;
    bankDecayDelayUpgradeLevel = 0;
    rareChanceUpgradeLevel = 0;
    manualChickenClicks = 0;
    totalCookProduced = 0;
    totalFactoryProduced = 0;
    totalBankDeposited = 0;
    
    syncBouncingChickens();
    return true;
}
