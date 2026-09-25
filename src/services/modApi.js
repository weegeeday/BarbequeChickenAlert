/**
 * Barbeque Chicken Alert - Modding API (BCA_API)
 * Provides hooks, state getters/setters, custom upgrade registration, skin overrides, and UI extensions for mods.
 */

class ModApi {
  constructor() {
    this.version = '1.0.0'
    this.listeners = {
      init: [],
      tick: [],
      chickenSpawn: [],
      chickenClick: [],
      popupClick: [],
      rebirth: [],
      save: [],
      load: [],
    }

    this.customUpgrades = new Map()
    this.customGenerators = new Map()
    this.customHudButtons = new Map()
    this.customMenuItems = new Map()
    this.customSkins = new Map()
    this.activeSkinId = null
    this.customChickenImages = null
    this.customSounds = {}
    this.modSaveData = {}

    // Reference to game context (set by GameView on mount)
    this.gameContext = null
  }

  /**
   * Called by GameView when mounting to bind live game state & callbacks.
   */
  bindGameContext(context) {
    this.gameContext = context
    this.trigger('init', this.getGameState())
  }

  /**
   * Event subscription
   */
  on(event, callback) {
    if (this.listeners[event]) {
      this.listeners[event].push(callback)
    }
  }

  off(event, callback) {
    if (this.listeners[event]) {
      this.listeners[event] = this.listeners[event].filter(cb => cb !== callback)
    }
  }

  trigger(event, payload) {
    if (this.listeners[event]) {
      this.listeners[event].forEach(cb => {
        try {
          cb(payload)
        } catch (err) {
          console.error(`[ModAPI] Error in ${event} listener:`, err)
        }
      })
    }
  }

  // Convenience hook methods
  onInit(cb) { this.on('init', cb) }
  onTick(cb) { this.on('tick', cb) }
  onChickenSpawn(cb) { this.on('chickenSpawn', cb) }
  onChickenClick(cb) { this.on('chickenClick', cb) }
  onPopupClick(cb) { this.on('popupClick', cb) }
  onRebirth(cb) { this.on('rebirth', cb) }
  onSave(cb) { this.on('save', cb) }
  onLoad(cb) { this.on('load', cb) }

  /**
   * Game State Access
   */
  getGameState() {
    if (!this.gameContext) return null
    return {
      totalChickenCount: this.gameContext.totalChickenCount?.value ?? 0,
      cps: this.gameContext.averageCps?.value ?? 0,
      rebirthCount: this.gameContext.rebirthCount?.value ?? 0,
      upgradeLevel: this.gameContext.upgradeLevel?.value ?? 1,
      chickensPerPopup: this.gameContext.chickensPerPopup?.value ?? 1,
      isSoundEnabled: this.gameContext.isSoundEnabled?.value ?? true,
      activeChickenCount: this.gameContext.chickens?.value?.length ?? 0,
      chickenCap: this.gameContext.chickenCap?.value ?? 300,
      bankStored: this.gameContext.bankChickenStored?.value ?? 0,
      cookCount: this.gameContext.cookCount?.value ?? 0,
      factoryCount: this.gameContext.factoryCount?.value ?? 0,
    }
  }

  addChickens(amount) {
    if (this.gameContext && typeof amount === 'number' && !isNaN(amount)) {
      this.gameContext.addChickens?.(amount)
    }
  }

  setChickens(amount) {
    if (this.gameContext && typeof amount === 'number' && !isNaN(amount)) {
      if (this.gameContext.totalChickenCount) {
        this.gameContext.totalChickenCount.value = Math.max(0, amount)
      }
    }
  }

  /**
   * Custom Upgrades API
   */
  addUpgrade(upgradeDef) {
    if (!upgradeDef.id || !upgradeDef.name) {
      console.warn('[ModAPI] Upgrade must include id and name')
      return false
    }
    const upgrade = {
      id: upgradeDef.id,
      name: upgradeDef.name,
      description: upgradeDef.description || '',
      cost: upgradeDef.cost || 100,
      level: 0,
      maxLevel: upgradeDef.maxLevel || Infinity,
      icon: upgradeDef.icon || '[Mod]',
      onBuy: upgradeDef.onBuy || (() => {}),
      costMultiplier: upgradeDef.costMultiplier || 1.5,
      ...upgradeDef,
    }
    this.customUpgrades.set(upgrade.id, upgrade)
    return true
  }

  getCustomUpgrades() {
    return Array.from(this.customUpgrades.values())
  }

  buyCustomUpgrade(id) {
    const upgrade = this.customUpgrades.get(id)
    if (!upgrade) return false

    const currentCount = this.gameContext?.totalChickenCount?.value ?? 0
    if (currentCount >= upgrade.cost && upgrade.level < upgrade.maxLevel) {
      this.addChickens(-upgrade.cost)
      upgrade.level += 1
      upgrade.cost = Math.floor(upgrade.cost * upgrade.costMultiplier)
      try {
        upgrade.onBuy(upgrade.level, this.getGameState())
      } catch (e) {
        console.error(`[ModAPI] Error in upgrade ${id} onBuy:`, e)
      }
      return true
    }
    return false
  }

  /**
   * Custom Generator / Passive Income API
   */
  addGenerator(generatorDef) {
    if (!generatorDef.id || !generatorDef.name) return false
    const gen = {
      id: generatorDef.id,
      name: generatorDef.name,
      description: generatorDef.description || '',
      baseCost: generatorDef.baseCost || 500,
      baseCps: generatorDef.baseCps || 5,
      count: 0,
      costMultiplier: generatorDef.costMultiplier || 1.15,
      icon: generatorDef.icon || '[Mod]',
      ...generatorDef,
    }
    this.customGenerators.set(gen.id, gen)
    return true
  }

  getCustomGenerators() {
    return Array.from(this.customGenerators.values())
  }

  buyCustomGenerator(id) {
    const gen = this.customGenerators.get(id)
    if (!gen) return false
    const currentCost = Math.floor(gen.baseCost * Math.pow(gen.costMultiplier, gen.count))
    const currentChickens = this.gameContext?.totalChickenCount?.value ?? 0

    if (currentChickens >= currentCost) {
      this.addChickens(-currentCost)
      gen.count += 1
      return true
    }
    return false
  }

  /**
   * UI Extension API
   */
  addHUDButton(buttonDef) {
    if (!buttonDef.id || !buttonDef.label) return false
    this.customHudButtons.set(buttonDef.id, buttonDef)
    return true
  }

  getHUDButtons() {
    return Array.from(this.customHudButtons.values())
  }

  addMenuItem(itemDef) {
    if (!itemDef.id || !itemDef.label) return false
    this.customMenuItems.set(itemDef.id, itemDef)
    return true
  }

  getMenuItems() {
    return Array.from(this.customMenuItems.values())
  }

  injectHTML(htmlString, targetSelector = '#mod-container') {
    let target = document.querySelector(targetSelector)
    if (!target) {
      target = document.createElement('div')
      target.id = targetSelector.replace('#', '')
      document.body.appendChild(target)
    }
    const container = document.createElement('div')
    container.innerHTML = htmlString
    target.appendChild(container)
    return container
  }

  /**
   * Skin / Graphics / Sound Overrides
   */
  registerSkin(skinDef) {
    if (!skinDef.id || !skinDef.name) return false
    this.customSkins.set(skinDef.id, skinDef)
    return true
  }

  setSkin(skinId) {
    const skin = this.customSkins.get(skinId)
    if (skin) {
      this.activeSkinId = skinId
      if (skin.chickenImages && skin.chickenImages.length > 0) {
        this.overrideChickenImages(skin.chickenImages)
      }
      if (skin.sounds) {
        this.overrideSounds(skin.sounds)
      }
    }
  }

  overrideChickenImages(imageUrls) {
    if (Array.isArray(imageUrls) && imageUrls.length > 0) {
      this.customChickenImages = imageUrls
    }
  }

  overrideSounds(soundObject) {
    if (soundObject) {
      this.customSounds = { ...this.customSounds, ...soundObject }
    }
  }

  /**
   * Mod Storage API
   */
  setModData(modId, key, value) {
    if (!this.modSaveData[modId]) {
      this.modSaveData[modId] = {}
    }
    this.modSaveData[modId][key] = value
  }

  getModData(modId, key, defaultValue = null) {
    if (this.modSaveData[modId] && this.modSaveData[modId][key] !== undefined) {
      return this.modSaveData[modId][key]
    }
    return defaultValue
  }
}

// Create singleton instance and bind to global window scope
const modApi = new ModApi()
if (typeof window !== 'undefined') {
  window.BCA_API = modApi
  window.BCAMods = modApi
}

export default modApi
