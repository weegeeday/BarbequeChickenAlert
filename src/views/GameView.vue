<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import popupImage from '../assets/BCA.svg'
import soundFile from '../assets/sound.mp3'
import sound2File from '../assets/sound2.mp3'
import sound3File from '../assets/sound3.mp3'

const chickenImageModules = import.meta.glob('../assets/chicken/**/*.{png,jpg,jpeg,webp,avif,gif}', {
  eager: true,
  import: 'default',
})

const linearFrictionPerSecond = 0.08
const angularFrictionPerSecond = 0.55
const saveStorageKey = 'barbequeChickenAlert.save.v1'
const repoUrl = 'https://github.com/weegeeday/BarbequeChickenAlert'
const bugReportUrl = `${repoUrl}/issues/new`
const baseRarePopupChance = 0.02
const rarePopupMultiplier = 10
const extremelyRarePopupChance = 1 / 10000
const extremelyRarePopupMultiplier = 100
const isPopupVisible = ref(false)
const activePopupTier = ref('normal')
const isRarePopupActive = ref(false)
const isExtremePopupActive = ref(false)
const popupCycleKey = ref(0)
const viewportWidth = ref(window.innerWidth)
const viewportHeight = ref(window.innerHeight)
const viewportScale = ref(window.visualViewport?.scale ?? 1)
const chickens = ref([])
const areCollisionsEnabled = ref(true)
const isPerformanceMode = ref(false)
const isMenuOpen = ref(false)
const isLeftMenuOpen = ref(false)
const chickensPerPopup = ref(1)
const upgradeLevel = ref(1)
const hasAutoPopupClickUpgrade = ref(false)
const isAutoClickerEnabled = ref(true)
const isSoundEnabled = ref(true)
const rebirthCount = ref(0)
const rebirthChickens = ref(0)
// Permanent rebirth upgrades
const rebirthUpgrades = ref({
  startWithChickens: false, // Start with 0.05% of previous chickens
  extraMultiplier: 0,      // Additional multiplier
  earlyChickenUpgrade: false // Upgrade all chickens early
})
const hasChickenBreastUnlock = ref(false)
const hasBankUnlock = ref(false)
const bankChickenStored = ref(0)
const hasFactoryUnlock = ref(false)
const factoryCount = ref(0)
const cookCount = ref(0)
const isUnlimitedCap = ref(false)
const chickenCap = ref(300)
const fpsCounter = ref(0)
const averageCps = ref(0)
const totalChickenCount = ref(0)
const showSavePrompt = ref(false)
const sessionSavingEnabled = ref(true)
const autosaveStatus = ref('Autosave: On')
const pendingSavedState = ref(null)
const saveImportInput = ref(null)
const activeConfirmPrompt = ref(null)
const deleteSaveConfirmationText = ref('')
const lockedLeftMenuNoticeOpen = ref(false)
const bankDepositAmount = ref(0)
const bankWithdrawAmount = ref(0)
const floatingNumbers = ref([])
const shouldFlashLeftMenu = ref(false)
const chickenHueShift = ref(0)
const rainbowCycleEnabled = ref(false)
const performanceNoticeVisible = ref(false)
const isHelpOpen = ref(false)
const isOtherUpgradesOpen = ref(false)
const shouldFlashRightMenu = ref(false)
const hoveredUpgradeId = ref(null)
const manualChickenClicks = ref(0)
const isDocumentVisible = ref(true)
const rarePopupShownCount = ref(0)
const totalCookProduced = ref(0)
const totalFactoryProduced = ref(0)
const totalBankDeposited = ref(0)
const clickPowerUpgradeLevel = ref(0)
const popupSpeedOtherUpgradeLevel = ref(0)
const cookCpsUpgradeLevel = ref(0)
const factoryCpsUpgradeLevel = ref(0)
const bankDecayDelayUpgradeLevel = ref(0)
const rareChanceUpgradeLevel = ref(0)
const iOSInteractiveTouchSelector = '.menu-panel, .left-menu-panel, .hud, .hud-left, .save-dialog, .save-overlay, button, input, label, a'

let popupTimerId = null
let animationFrameId = null
let previousTimestamp = 0
let chickenIdSeed = 0
let activeDragId = null
let lowFpsDurationMs = 0
let stableFpsDurationMs = 0
let smoothedFps = 60
let cookProductionCarry = 0
let bankProductionCarry = 0
let factoryProductionCarry = 0
let totalChickenEarned = 0
let rainbowCycleHue = 0
let bankDecayDelayRemainingMs = 60000
let sessionStartTimestamp = performance.now()
let touchStartHandler = null
let gestureStartHandler = null
let gestureChangeHandler = null
let handleVisibilityChange = null

const popupAudio = new Audio(soundFile)
const chickenAudio = new Audio(sound2File)
const rarePopupAudio = new Audio(sound3File)
const chickenImageEntries = Object.entries(chickenImageModules)
const wingChickenImages = chickenImageEntries
  .filter(([path]) => path.toLowerCase().includes('/wings/'))
  .map(([, src]) => src)
const chickenBreastImages = chickenImageEntries
  .filter(([path]) => path.toLowerCase().includes('/full/'))
  .map(([, src]) => src)
const nuggetImages = chickenImageEntries
  .filter(([path]) => path.toLowerCase().includes('/nugget/'))
  .map(([, src]) => src)
const chickenImages = [...wingChickenImages, ...chickenBreastImages]

popupAudio.preload = 'auto'
chickenAudio.preload = 'auto'
rarePopupAudio.preload = 'auto'

popupAudio.muted = true
chickenAudio.muted = true
rarePopupAudio.muted = true

const syncSoundPreference = () => {
  const muted = !isSoundEnabled.value
  popupAudio.muted = muted
  chickenAudio.muted = muted
  rarePopupAudio.muted = muted
}

syncSoundPreference()

const getUpgradeGain = (level) => {
  const baseCpp = Math.floor((level ** 2.5) / 3 + 1 + 1e-9)
  return Math.max(1, Math.floor(baseCpp * rebirthMultiplier.value + 1e-9))
}

const getUpgradeCost = (level) => {
  return Math.max(1, Math.floor(1.5 * (level ** 3) + 1 + 1e-9))
}

const chickenCount = computed(() => totalChickenCount.value)
const nextUpgradeGain = computed(() => getUpgradeGain(upgradeLevel.value + 1))
const nextUpgradeCost = computed(() => getUpgradeCost(upgradeLevel.value))
const canAffordUpgrade = computed(() => chickenCount.value >= nextUpgradeCost.value)
const autoPopupClickUpgradeCost = 1000
const chickenBreastUnlockCost = 200
const bankUnlockCost = 5000
const factoryUnlockCost = 300
const cookBaseCost = 125
const getCookCost = (level) => {
  return Math.max(cookBaseCost, Math.floor(1.5 * (level ** 3) + 1 + 1e-9))
}
const getFactoryCost = (level) => {
  return Math.max(factoryUnlockCost, Math.floor(1.5 * (level ** 3) + 1 + 1e-9))
}
const getClickPowerUpgradeCost = (level) => {
  return Math.max(80, Math.floor(65 * (level ** 2.1) + 1))
}
const getOtherPopupSpeedUpgradeCost = (level) => {
  return Math.max(125, Math.floor(120 * (level ** 2.2) + 1))
}
const getCookCpsUpgradeCost = (level) => {
  return Math.max(180, Math.floor(165 * (level ** 2.25) + 1))
}
const getFactoryCpsUpgradeCost = (level) => {
  return Math.max(240, Math.floor(220 * (level ** 2.25) + 1))
}
const getBankDelayUpgradeCost = (level) => {
  return Math.max(260, Math.floor(230 * (level ** 2.3) + 1))
}
const getRareChanceUpgradeCost = (level) => {
  return Math.max(320, Math.floor(280 * (level ** 2.4) + 1))
}
const clickUpgradeUnlockThresholds = [12, 28, 50, 80, 120, 170, 230, 300, 380, 470]
const cookUpgradeUnlockThresholds = [40, 180, 700, 2100, 5400]
const factoryUpgradeUnlockThresholds = [60, 280, 900, 2600, 6800]
const bankDelayUpgradeUnlockThresholds = [250, 1200, 4500, 14000, 42000]
const rareChanceUpgradeUnlockThresholds = [4, 14, 45]
const rareChanceUpgradeBonuses = [0.003, 0.006, 0.01]
const bankDecayBaseDelayMs = 60000
const rebirthMultiplier = computed(() => 1 + 0.5 * rebirthCount.value + rebirthUpgrades.value.extraMultiplier)
const clickPowerMultiplier = computed(() => 1 + clickPowerUpgradeLevel.value)
const cookCpsUpgradeMultiplier = computed(() => 1 + cookCpsUpgradeLevel.value * 0.25)
const factoryCpsUpgradeMultiplier = computed(() => 1 + factoryCpsUpgradeLevel.value * 0.25)
const canAffordChickenBreastUnlock = computed(() => {
  return !hasChickenBreastUnlock.value && chickenCount.value >= chickenBreastUnlockCost
})
const canAffordBankUnlock = computed(() => {
  return !hasBankUnlock.value && chickenCount.value >= bankUnlockCost
})
const canAffordFactoryUnlock = computed(() => {
  return !hasFactoryUnlock.value && chickenCount.value >= factoryUnlockCost
})
const canOpenLeftMenu = computed(() => rebirthCount.value > 0 || hasBankUnlock.value)
const bankEfficiencyPercent = ref(35)
const bankCpsGeneration = computed(() => bankChickenStored.value * (bankEfficiencyPercent.value / 100) * 0.01)
const factoryCpsGeneration = computed(() => factoryCount.value * 2 * rebirthMultiplier.value * factoryCpsUpgradeMultiplier.value)
const nextPopupSpeedCost = computed(() => getOtherPopupSpeedUpgradeCost(popupSpeedOtherUpgradeLevel.value + 1))
const canAffordPopupSpeedUpgrade = computed(() => chickenCount.value >= nextPopupSpeedCost.value)
const nextCookCost = computed(() => getCookCost(cookCount.value + 1))
const canAffordCook = computed(() => chickenCount.value >= nextCookCost.value)
const cookOutputPerSecond = computed(() => cookCount.value * (1 + rebirthCount.value) * cookCpsUpgradeMultiplier.value)
const currentPopupIntervalMs = computed(() => {
  return Math.max(1000, 5000 - popupSpeedOtherUpgradeLevel.value * 100)
})
const currentRarePopupChance = computed(() => {
  const bonus = rareChanceUpgradeBonuses.slice(0, rareChanceUpgradeLevel.value).reduce((total, value) => total + value, 0)
  return Math.min(0.75, baseRarePopupChance + bonus)
})
const activePopupMultiplier = computed(() => {
  if (activePopupTier.value === 'extremelyRare') {
    return extremelyRarePopupMultiplier
  }

  if (activePopupTier.value === 'rare') {
    return rarePopupMultiplier
  }

  return 1
})
const bankDecayDelayMs = computed(() => bankDecayBaseDelayMs + bankDecayDelayUpgradeLevel.value * 10000)
const hasPopupTierBonus = computed(() => activePopupMultiplier.value > 1)
const isCompactHud = computed(() => viewportWidth.value < 520)
const isPopupSpeedFullyUpgraded = computed(() => currentPopupIntervalMs.value <= 1000)
const getRebirthLevelRequirement = () => {
  return 10 + rebirthCount.value * 10
}
const nextRebirthLevelRequirement = computed(() => getRebirthLevelRequirement())
const canAffordRebirth = computed(() => upgradeLevel.value >= nextRebirthLevelRequirement.value)
const canAffordAutoPopupClickUpgrade = computed(() => {
  return !hasAutoPopupClickUpgrade.value && chickenCount.value >= autoPopupClickUpgradeCost
})
const nextClickPowerCost = computed(() => getClickPowerUpgradeCost(clickPowerUpgradeLevel.value + 1))
const canAffordClickPowerUpgrade = computed(() => {
  return clickPowerUpgradeLevel.value < 10 && chickenCount.value >= nextClickPowerCost.value
})
const nextCookCpsUpgradeCost = computed(() => getCookCpsUpgradeCost(cookCpsUpgradeLevel.value + 1))
const canAffordCookCpsUpgrade = computed(() => {
  return cookCpsUpgradeLevel.value < 5 && chickenCount.value >= nextCookCpsUpgradeCost.value
})
const nextFactoryCpsUpgradeCost = computed(() => getFactoryCpsUpgradeCost(factoryCpsUpgradeLevel.value + 1))
const canAffordFactoryCpsUpgrade = computed(() => {
  return factoryCpsUpgradeLevel.value < 5 && chickenCount.value >= nextFactoryCpsUpgradeCost.value
})
const nextBankDelayUpgradeCost = computed(() => getBankDelayUpgradeCost(bankDecayDelayUpgradeLevel.value + 1))
const canAffordBankDelayUpgrade = computed(() => {
  return bankDecayDelayUpgradeLevel.value < 5 && chickenCount.value >= nextBankDelayUpgradeCost.value
})
const nextRareChanceUpgradeCost = computed(() => getRareChanceUpgradeCost(rareChanceUpgradeLevel.value + 1))
const canAffordRareChanceUpgrade = computed(() => {
  return rareChanceUpgradeLevel.value < 3 && chickenCount.value >= nextRareChanceUpgradeCost.value
})
const nextClickPowerUnlockThreshold = computed(() => {
  return clickUpgradeUnlockThresholds[clickPowerUpgradeLevel.value] ?? Number.MAX_SAFE_INTEGER
})
const isClickPowerUpgradeUnlocked = computed(() => manualChickenClicks.value >= nextClickPowerUnlockThreshold.value)
const nextCookUpgradeUnlockThreshold = computed(() => {
  return cookUpgradeUnlockThresholds[cookCpsUpgradeLevel.value] ?? Number.MAX_SAFE_INTEGER
})
const isCookUpgradeUnlocked = computed(() => {
  return cookCount.value > 0 && totalCookProduced.value >= nextCookUpgradeUnlockThreshold.value
})
const nextFactoryUpgradeUnlockThreshold = computed(() => {
  return factoryUpgradeUnlockThresholds[factoryCpsUpgradeLevel.value] ?? Number.MAX_SAFE_INTEGER
})
const isFactoryUpgradeUnlocked = computed(() => {
  return factoryCount.value > 0 && totalFactoryProduced.value >= nextFactoryUpgradeUnlockThreshold.value
})
const nextBankDelayUnlockThreshold = computed(() => {
  return bankDelayUpgradeUnlockThresholds[bankDecayDelayUpgradeLevel.value] ?? Number.MAX_SAFE_INTEGER
})
const isBankDelayUpgradeUnlocked = computed(() => {
  return hasBankUnlock.value && totalBankDeposited.value >= nextBankDelayUnlockThreshold.value
})
const nextRareChanceUnlockThreshold = computed(() => {
  return rareChanceUpgradeUnlockThresholds[rareChanceUpgradeLevel.value] ?? Number.MAX_SAFE_INTEGER
})
const isRareChanceUpgradeUnlocked = computed(() => {
  return rarePopupShownCount.value >= nextRareChanceUnlockThreshold.value
})
const shouldShowRightMenuAffordHint = computed(() => {
  return rebirthCount.value === 0 && upgradeLevel.value === 1 && canAffordUpgrade.value
})
const activePopupMultiplierLabel = computed(() => `${activePopupMultiplier.value}x`)
const effectiveRenderLimit = computed(() => {
  if (isUnlimitedCap.value) {
    return totalChickenCount.value
  }

  return Math.min(totalChickenCount.value, normalizeCapValue(chickenCap.value))
})

const randomBetween = (minimum, maximum) => {
  return minimum + Math.random() * (maximum - minimum)
}

const capSpeed = (value, maxMagnitude) => {
  if (Math.abs(value) <= maxMagnitude) {
    return value
  }

  return value > 0 ? maxMagnitude : -maxMagnitude
}

const getChickenSize = () => {
  const scaleAdjustedSize = Math.min(viewportWidth.value, viewportHeight.value) * 0.12 * viewportScale.value
  const baseSize = Math.max(56, scaleAdjustedSize)
  return Math.max(56, Math.min(128, Math.round(baseSize)))
}

const normalizeCapValue = (value) => {
  const safeValue = Number.isFinite(value) ? value : 300
  return Math.max(25, Math.min(2000, Math.round(safeValue)))
}

const normalizePositiveInteger = (value, fallback = 1, minimum = 1, maximum = Number.MAX_SAFE_INTEGER) => {
  const safeValue = Number.isFinite(value) ? value : fallback
  return Math.max(minimum, Math.min(maximum, Math.round(safeValue)))
}

const enforceChickenCap = () => {
  chickenCap.value = normalizeCapValue(chickenCap.value)
}

const clampPosition = (chicken) => {
  const maxX = Math.max(0, viewportWidth.value - chicken.size)
  const maxY = Math.max(0, viewportHeight.value - chicken.size)
  chicken.x = Math.max(0, Math.min(chicken.x, maxX))
  chicken.y = Math.max(0, Math.min(chicken.y, maxY))
}

const playPopupAudio = async () => {
  if (!isSoundEnabled.value || !isDocumentVisible.value) {
    return
  }

  try {
    popupAudio.currentTime = 0
    await popupAudio.play()
  } catch (error) {
    void error
  }
}

const playChickenAudio = async () => {
  if (!isSoundEnabled.value || !isDocumentVisible.value) {
    return
  }

  try {
    chickenAudio.currentTime = 0
    await chickenAudio.play()
  } catch (error) {
    void error
  }
}

const playRarePopupAudio = async () => {
  if (!isSoundEnabled.value || !isDocumentVisible.value) {
    return
  }

  try {
    rarePopupAudio.currentTime = 0
    await rarePopupAudio.play()
  } catch (error) {
    void error
  }
}

const updateViewport = () => {
  const visualViewport = window.visualViewport

  viewportWidth.value = Math.round(visualViewport?.width ?? window.innerWidth)
  viewportHeight.value = Math.round(visualViewport?.height ?? window.innerHeight)
  viewportScale.value = visualViewport?.scale ?? window.devicePixelRatio ?? 1
  chickens.value.forEach((chicken) => {
    clampPosition(chicken)
  })
}

const updatePopupTierFlags = (tier) => {
  activePopupTier.value = tier
  isRarePopupActive.value = tier === 'rare' || tier === 'extremelyRare'
  isExtremePopupActive.value = tier === 'extremelyRare'
}

const registerRarePopupAppearance = (tier) => {
  if (tier === 'rare' || tier === 'extremelyRare') {
    rarePopupShownCount.value += 1
  }
}

const showPopup = (tier = 'normal') => {
  if (isPopupVisible.value) {
    const activeIsRareTier = activePopupTier.value === 'rare' || activePopupTier.value === 'extremelyRare'
    const incomingIsRareTier = tier === 'rare' || tier === 'extremelyRare'

    if (incomingIsRareTier && !activeIsRareTier) {
      registerRarePopupAppearance(tier)
      updatePopupTierFlags(tier)
      popupCycleKey.value += 1
      void playRarePopupAudio()
    }
    return
  }

  updatePopupTierFlags(tier)
  registerRarePopupAppearance(tier)
  isPopupVisible.value = true
  popupCycleKey.value += 1
  if (tier === 'rare' || tier === 'extremelyRare') {
    void playRarePopupAudio()
  } else {
    void playPopupAudio()
  }

  if (hasAutoPopupClickUpgrade.value && isAutoClickerEnabled.value) {
    window.setTimeout(() => {
      handlePopupClick()
    }, 0)
  }
}

const rollPopupTier = () => {
  const roll = Math.random()
  const clampedRareChance = Math.max(0, Math.min(0.9999, currentRarePopupChance.value))

  if (roll < extremelyRarePopupChance) {
    return 'extremelyRare'
  }

  if (roll < extremelyRarePopupChance + clampedRareChance) {
    return 'rare'
  }

  return 'normal'
}

const spawnPopup = () => {
  const popupTier = rollPopupTier()
  showPopup(popupTier)
}

const forceRarePopup = () => {
  showPopup('rare')
}

const createChicken = () => {
  const size = getChickenSize()
  const maxX = Math.max(0, viewportWidth.value - size)
  const maxY = Math.max(0, viewportHeight.value - size)

  const imageTypes = []

  if (wingChickenImages.length > 0) {
    imageTypes.push({ key: 'wing', images: wingChickenImages })
  }

  if (hasChickenBreastUnlock.value && chickenBreastImages.length > 0) {
    imageTypes.push({ key: 'breast', images: chickenBreastImages })
  }

  if (hasFactoryUnlock.value && nuggetImages.length > 0) {
    imageTypes.push({ key: 'nugget', images: nuggetImages })
  }

  if (imageTypes.length === 0) {
    imageTypes.push({ key: 'wing', images: chickenImages })
  }

  const selectedTypeObj = imageTypes[Math.floor(Math.random() * imageTypes.length)]
  const selectedImage = selectedTypeObj.images[Math.floor(Math.random() * selectedTypeObj.images.length)]

  return {
    id: ++chickenIdSeed,
    src: selectedImage,
    type: selectedTypeObj.key,
    size,
    x: randomBetween(0, maxX),
    y: randomBetween(0, maxY),
    velocityX: randomBetween(-240, 240) || 160,
    velocityY: randomBetween(-240, 240) || -160,
    dragOffsetX: 0,
    dragOffsetY: 0,
    pointerLastX: 0,
    pointerLastY: 0,
    pointerLastTime: 0,
    dragVelocityX: 0,
    dragVelocityY: 0,
    dragAngularVelocity: 0,
    angle: randomBetween(0, 360),
    angularVelocity: randomBetween(-180, 180),
    movedWhileDragging: false,
    isDragging: false,
  }
}

const syncRenderedChickens = () => {
  const targetCount = effectiveRenderLimit.value

  while (chickens.value.length > targetCount) {
    chickens.value.shift()
  }

  while (chickens.value.length < targetCount) {
    chickens.value.push(createChicken())
  }
}

const addChicken = (amount) => {
  if (amount <= 0) {
    return
  }

  totalChickenCount.value += amount
  totalChickenEarned += amount
  syncRenderedChickens()
}

const handlePopupClick = () => {
  void playChickenAudio()

  const requested = chickensPerPopup.value * activePopupMultiplier.value
  const total = Math.floor(requested * rebirthMultiplier.value)

  if (total > 0) {
    addChicken(total)
  }

  updatePopupTierFlags('normal')
  isPopupVisible.value = false

  if (popupTimerId !== null) {
    window.clearTimeout(popupTimerId)
  }

  popupTimerId = window.setTimeout(spawnPopup, currentPopupIntervalMs.value)
}

const toggleMenu = () => {
  isMenuOpen.value = !isMenuOpen.value
}

const toggleOtherUpgrades = () => {
  isOtherUpgradesOpen.value = !isOtherUpgradesOpen.value
}

const closeOtherUpgrades = () => {
  isOtherUpgradesOpen.value = false
}

const handleUpgradeCardClick = (cardId, purchaseHandler) => {
  const isMobile = window.innerWidth <= 480

  if (isMobile) {
    if (hoveredUpgradeId.value === cardId) {
      purchaseHandler()
      hoveredUpgradeId.value = null
    } else {
      hoveredUpgradeId.value = cardId
    }
  } else {
    purchaseHandler()
  }
}

const openHelp = () => {
  isHelpOpen.value = true
}

const closeHelp = () => {
  isHelpOpen.value = false
}

const toggleLeftMenu = () => {
  if (!canOpenLeftMenu.value) {
    return
  }

  isLeftMenuOpen.value = !isLeftMenuOpen.value
  shouldFlashLeftMenu.value = false
}

const handleLeftMenuButtonClick = () => {
  if (canOpenLeftMenu.value) {
    toggleLeftMenu()
    return
  }

  lockedLeftMenuNoticeOpen.value = true
}

const closeLockedLeftMenuNotice = () => {
  lockedLeftMenuNoticeOpen.value = false
}

watch(shouldShowRightMenuAffordHint, (value) => {
  shouldFlashRightMenu.value = value
})

const upgradeChickenSpawn = () => {
  if (!canAffordUpgrade.value) {
    return
  }

  const upgradeCost = nextUpgradeCost.value
  const newCpp = nextUpgradeGain.value

  totalChickenCount.value -= upgradeCost
  syncRenderedChickens()
  chickensPerPopup.value = newCpp
  upgradeLevel.value += 1
}

const upgradeAutoPopupClick = () => {
  if (!canAffordAutoPopupClickUpgrade.value) {
    return
  }

  totalChickenCount.value -= autoPopupClickUpgradeCost
  hasAutoPopupClickUpgrade.value = true
  syncRenderedChickens()
}

const unlockChickenBreast = () => {
  if (!canAffordChickenBreastUnlock.value) {
    return
  }

  totalChickenCount.value -= chickenBreastUnlockCost
  hasChickenBreastUnlock.value = true
  syncRenderedChickens()
}

const upgradePopupSpeed = () => {
  if (!canAffordPopupSpeedUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextPopupSpeedCost.value
  popupSpeedOtherUpgradeLevel.value += 1
  syncRenderedChickens()
}

const upgradeClickPower = () => {
  if (!isClickPowerUpgradeUnlocked.value || !canAffordClickPowerUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextClickPowerCost.value
  clickPowerUpgradeLevel.value += 1
  syncRenderedChickens()
}

const upgradeCookCps = () => {
  if (!isCookUpgradeUnlocked.value || !canAffordCookCpsUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextCookCpsUpgradeCost.value
  cookCpsUpgradeLevel.value += 1
  syncRenderedChickens()
}

const upgradeFactoryCps = () => {
  if (!isFactoryUpgradeUnlocked.value || !canAffordFactoryCpsUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextFactoryCpsUpgradeCost.value
  factoryCpsUpgradeLevel.value += 1
  syncRenderedChickens()
}

const upgradeBankDecayDelay = () => {
  if (!isBankDelayUpgradeUnlocked.value || !canAffordBankDelayUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextBankDelayUpgradeCost.value
  bankDecayDelayUpgradeLevel.value += 1
  bankDecayDelayRemainingMs = bankDecayDelayMs.value
  syncRenderedChickens()
}

const upgradeRareChance = () => {
  if (!isRareChanceUpgradeUnlocked.value || !canAffordRareChanceUpgrade.value) {
    return
  }

  totalChickenCount.value -= nextRareChanceUpgradeCost.value
  rareChanceUpgradeLevel.value += 1
  syncRenderedChickens()
}

const hireCook = () => {
  if (!canAffordCook.value) {
    return
  }

  totalChickenCount.value -= nextCookCost.value
  cookCount.value += 1
  bankEfficiencyPercent.value = Math.min(90, bankEfficiencyPercent.value + 1)
  syncRenderedChickens()
}

const unlockBank = () => {
  if (!canAffordBankUnlock.value) {
    return
  }

  totalChickenCount.value -= bankUnlockCost
  hasBankUnlock.value = true
  shouldFlashLeftMenu.value = true
  syncRenderedChickens()
}

const depositToBank = (amount) => {
  const depositAmount = Math.min(amount, totalChickenCount.value)
  if (depositAmount <= 0) {
    return
  }

  totalChickenCount.value -= depositAmount
  bankChickenStored.value += depositAmount
  totalBankDeposited.value += depositAmount
  bankDecayDelayRemainingMs = bankDecayDelayMs.value
  syncRenderedChickens()
}

const rebirthUpgradeOptions = [
  { key: 'none', label: 'Do nothing (free)' },
  { key: 'mult', label: '+0.5× Multiplier' },
  { key: 'startWithChickens', label: 'Start with 0.05% of previous chickens' },
  { key: 'earlyChickenUpgrade', label: 'Upgrade all chickens early' },
]

const getBaseRebirthUpgradeCost = (key) => {
  if (key === 'mult') {
    return 1 + rebirthCount.value * 2
  }

  if (key === 'startWithChickens') {
    return 100
  }

  if (key === 'earlyChickenUpgrade') {
    return 30
  }

  return 0
}

const getEffectiveRebirthUpgradeCost = (key) => {
  if (key === 'startWithChickens' && rebirthUpgrades.value.startWithChickens) {
    return 0
  }

  if (key === 'earlyChickenUpgrade' && rebirthUpgrades.value.earlyChickenUpgrade) {
    return 0
  }

  return getBaseRebirthUpgradeCost(key)
}

const selectedRebirthUpgradeCost = computed(() => getEffectiveRebirthUpgradeCost(rebirthMenu.value.selected))
const projectedRebirthRcBalance = computed(() => {
  return rebirthChickens.value + rebirthMenu.value.pendingRC - selectedRebirthUpgradeCost.value
})

const rebirthMenu = ref({ visible: false, selected: 'none', error: '', pendingRC: 0 })

const openRebirthMenu = () => {
  if (!canAffordRebirth.value) return
  const chickensForRebirth = Math.floor(totalChickenCount.value * 0.5)
  const gainedRebirthChickens = Math.floor(chickensForRebirth * 0.0001)
  rebirthMenu.value = { visible: true, selected: 'none', error: '', pendingRC: gainedRebirthChickens }
}

const cancelRebirthMenu = () => {
  rebirthMenu.value.visible = false
  rebirthMenu.value.selected = 'none'
  rebirthMenu.value.error = ''
  rebirthMenu.value.pendingRC = 0
}

const confirmRebirthMenu = () => {
  if (!canAffordRebirth.value) return
  const chickensForRebirth = Math.floor(totalChickenCount.value * 0.5)
  const gainedRebirthChickens = Math.floor(chickensForRebirth * 0.0001)
  let availableRC = rebirthChickens.value + gainedRebirthChickens

  let startingChickens = 0
  let upgradeLevelOverride = 1
  let chickensPerPopupOverride = 1

  // Handle upgrade selection
  const sel = rebirthMenu.value.selected
  const selectedCost = getEffectiveRebirthUpgradeCost(sel)

  if (selectedCost > availableRC) {
    if (sel === 'mult') {
      rebirthMenu.value.error = 'Not enough RC for multiplier upgrade.'
    } else if (sel === 'startWithChickens') {
      rebirthMenu.value.error = 'Not enough RC for start with chickens.'
    } else if (sel === 'earlyChickenUpgrade') {
      rebirthMenu.value.error = 'Not enough RC for early chicken upgrade.'
    }

    return
  }

  if (sel === 'mult') {
    availableRC -= selectedCost
    rebirthUpgrades.value.extraMultiplier += 0.5
  } else if (sel === 'startWithChickens' && !rebirthUpgrades.value.startWithChickens) {
    availableRC -= selectedCost
    rebirthUpgrades.value.startWithChickens = true
  } else if (sel === 'earlyChickenUpgrade' && !rebirthUpgrades.value.earlyChickenUpgrade) {
    availableRC -= selectedCost
    rebirthUpgrades.value.earlyChickenUpgrade = true
  }

  if (rebirthUpgrades.value.startWithChickens) {
    startingChickens = Math.floor(chickensForRebirth * 0.0005)
  }

  if (rebirthUpgrades.value.earlyChickenUpgrade) {
    upgradeLevelOverride = 2
    chickensPerPopupOverride = getUpgradeGain(2)
  }

  rebirthChickens.value = availableRC
  rebirthCount.value += 1
  totalChickenCount.value = startingChickens
  chickensPerPopup.value = chickensPerPopupOverride
  upgradeLevel.value = upgradeLevelOverride
  hasAutoPopupClickUpgrade.value = false
  hasChickenBreastUnlock.value = rebirthUpgrades.value.earlyChickenUpgrade
  hasBankUnlock.value = false
  bankChickenStored.value = 0
  hasFactoryUnlock.value = rebirthUpgrades.value.earlyChickenUpgrade
  factoryCount.value = 0
  popupSpeedOtherUpgradeLevel.value = 0
  cookCount.value = 0
  clickPowerUpgradeLevel.value = 0
  cookCpsUpgradeLevel.value = 0
  factoryCpsUpgradeLevel.value = 0
  bankDecayDelayUpgradeLevel.value = 0
  rareChanceUpgradeLevel.value = 0
  manualChickenClicks.value = 0
  totalCookProduced.value = 0
  totalFactoryProduced.value = 0
  totalBankDeposited.value = 0
  bankDecayDelayRemainingMs = bankDecayBaseDelayMs
  isOtherUpgradesOpen.value = false
  rainbowCycleHue = 0
  rebirthMenu.value.visible = false
  syncRenderedChickens()
}

// Duplicate removed
const withdrawFromBank = (amount) => {
  const withdrawAmount = Math.min(amount, bankChickenStored.value)
  if (withdrawAmount <= 0) {
    return
  }

  bankChickenStored.value -= withdrawAmount
  totalChickenCount.value += withdrawAmount
  syncRenderedChickens()
}

const unlockFactory = () => {
  if (!canAffordFactoryUnlock.value) {
    return
  }

  totalChickenCount.value -= factoryUnlockCost
  hasFactoryUnlock.value = true
  syncRenderedChickens()
}

const buyFactory = () => {
  const nextFactoryCost = getFactoryCost(factoryCount.value + 1)
  if (chickenCount.value < nextFactoryCost) {
    return
  }

  totalChickenCount.value -= nextFactoryCost
  factoryCount.value += 1
  syncRenderedChickens()
}

const nextFactoryCost = computed(() => getFactoryCost(factoryCount.value + 1))
const canAffordFactory = computed(() => chickenCount.value >= nextFactoryCost.value)

const applyChickenCap = () => {
  chickenCap.value = normalizeCapValue(chickenCap.value)
  enforceChickenCap()
  syncRenderedChickens()
}

const requestUnlimitedCapToggle = (event) => {
  const desiredValue = Boolean(event?.target?.checked)

  if (!desiredValue) {
    isUnlimitedCap.value = false
    applyChickenCap()
    return
  }

  activeConfirmPrompt.value = {
    title: 'Unlimited cap',
    message: 'Are you sure you want to enable unlimited cap? This can cause lag or crash your browser.',
    confirmLabel: 'Yes, enable it',
    cancelLabel: 'Cancel',
    onConfirm: () => {
      isUnlimitedCap.value = true
      syncRenderedChickens()
    },
  }
}

const createSavePayload = () => {
  return {
    version: 2,
    upgradePricingVersion: 1,
    savedAt: Date.now(),
    totalChickenCount: totalChickenCount.value,
    chickensPerPopup: chickensPerPopup.value,
    upgradeLevel: upgradeLevel.value,
    hasAutoPopupClickUpgrade: hasAutoPopupClickUpgrade.value,
    isAutoClickerEnabled: isAutoClickerEnabled.value,
    isSoundEnabled: isSoundEnabled.value,
    rebirthCount: rebirthCount.value,
    rebirthChickens: rebirthChickens.value,
    rebirthUpgrades: { ...rebirthUpgrades.value },
    hasChickenBreastUnlock: hasChickenBreastUnlock.value,
    popupSpeedOtherUpgradeLevel: popupSpeedOtherUpgradeLevel.value,
    popupSpeedUpgradeLevel: popupSpeedOtherUpgradeLevel.value,
    cookCount: cookCount.value,
    chickenCap: chickenCap.value,
    areCollisionsEnabled: areCollisionsEnabled.value,
    isPerformanceMode: isPerformanceMode.value,
    hasBankUnlock: hasBankUnlock.value,
    bankChickenStored: bankChickenStored.value,
    hasFactoryUnlock: hasFactoryUnlock.value,
    factoryCount: factoryCount.value,
    chickenHueShift: chickenHueShift.value,
    rainbowCycleEnabled: rainbowCycleEnabled.value,
    bankEfficiencyPercent: bankEfficiencyPercent.value,
    manualChickenClicks: manualChickenClicks.value,
    rarePopupShownCount: rarePopupShownCount.value,
    totalCookProduced: totalCookProduced.value,
    totalFactoryProduced: totalFactoryProduced.value,
    totalBankDeposited: totalBankDeposited.value,
    clickPowerUpgradeLevel: clickPowerUpgradeLevel.value,
    cookCpsUpgradeLevel: cookCpsUpgradeLevel.value,
    factoryCpsUpgradeLevel: factoryCpsUpgradeLevel.value,
    bankDecayDelayUpgradeLevel: bankDecayDelayUpgradeLevel.value,
    rareChanceUpgradeLevel: rareChanceUpgradeLevel.value,
  }
}

const writeAutosave = () => {
  if (!sessionSavingEnabled.value) {
    return
  }

  try {
    window.localStorage.setItem(saveStorageKey, JSON.stringify(createSavePayload()))
    autosaveStatus.value = 'Autosave: On'
  } catch {
    autosaveStatus.value = 'Autosave: Failed'
  }
}

const resetRebirthUnlocks = () => {
  rebirthUpgrades.value = {
    startWithChickens: false,
    extraMultiplier: 0,
    earlyChickenUpgrade: false,
  }
}

const removeSavedProgress = () => {
  try {
    window.localStorage.removeItem(saveStorageKey)
  } catch (error) {
    void error
  }

  resetRebirthUnlocks()
}

const openDeleteSavePrompt = () => {
  deleteSaveConfirmationText.value = ''
  activeConfirmPrompt.value = {
    title: 'Delete save',
    message: 'Type "yes im sure to delete" to confirm.',
    confirmLabel: 'Delete save',
    cancelLabel: 'Cancel',
    requiresTypedConfirmation: true,
    requiredTypedValue: 'yes im sure to delete',
    onConfirm: () => {
      removeSavedProgress()
      autosaveStatus.value = sessionSavingEnabled.value ? 'Autosave: On' : 'Autosave: Off (session)'
      window.location.reload()
    },
  }
}

const openRepository = () => {
  window.open(repoUrl, '_blank', 'noopener,noreferrer')
}

const reportBug = () => {
  window.open(bugReportUrl, '_blank', 'noopener,noreferrer')
}

const exportSaveFile = () => {
  const serializedSave = JSON.stringify(createSavePayload(), null, 2)
  const blob = new Blob([serializedSave], { type: 'application/json' })
  const objectUrl = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  const timestamp = new Date().toISOString().replace(/[.:]/g, '-').slice(0, 19)

  anchor.href = objectUrl
  anchor.download = `barbeque-chicken-save-${timestamp}.json`
  document.body.appendChild(anchor)
  anchor.click()
  document.body.removeChild(anchor)
  URL.revokeObjectURL(objectUrl)
}

const triggerSaveImport = () => {
  saveImportInput.value?.click()
}

const handleSaveImportSelection = async (event) => {
  const file = event.target.files?.[0]

  if (event.target) {
    event.target.value = ''
  }

  if (!file) {
    return
  }

  if (!window.confirm('Importing a save will overwrite your current progress. Continue?')) {
    return
  }

  try {
    const fileText = await file.text()
    const parsedSave = JSON.parse(fileText)

    if (!parsedSave || typeof parsedSave !== 'object') {
      throw new Error('Imported file does not contain a valid save object.')
    }

    applySavedProgress(parsedSave)
    resetRebirthUnlocks()
    sessionSavingEnabled.value = true
    autosaveStatus.value = 'Autosave: On'
    writeAutosave()
  } catch (error) {
    window.alert(`Could not import save: ${error instanceof Error ? error.message : 'Unknown error'}`)
  }
}

const confirmAndDeleteSave = () => {
  openDeleteSavePrompt()
}

const applySavedProgress = (savedState) => {
  const savedPopupCount = normalizePositiveInteger(savedState.chickensPerPopup, 1)
  const resolvedUpgradeLevel = normalizePositiveInteger(savedState.upgradeLevel, 1, 1)

  totalChickenCount.value = normalizePositiveInteger(savedState.totalChickenCount, 0, 0)
  chickensPerPopup.value = savedPopupCount
  upgradeLevel.value = resolvedUpgradeLevel
  hasAutoPopupClickUpgrade.value = Boolean(savedState.hasAutoPopupClickUpgrade)
  isAutoClickerEnabled.value = savedState.isAutoClickerEnabled !== false
  isSoundEnabled.value = savedState.isSoundEnabled !== false
  rebirthCount.value = normalizePositiveInteger(savedState.rebirthCount, 0, 0)
  rebirthChickens.value = normalizePositiveInteger(savedState.rebirthChickens, 0, 0)
  resetRebirthUnlocks()
  if (savedState.rebirthUpgrades) {
    rebirthUpgrades.value = {
      startWithChickens: !!savedState.rebirthUpgrades.startWithChickens,
      extraMultiplier: Number(savedState.rebirthUpgrades.extraMultiplier) || 0,
      earlyChickenUpgrade: !!savedState.rebirthUpgrades.earlyChickenUpgrade,
    }
  }
  hasChickenBreastUnlock.value = Boolean(savedState.hasChickenBreastUnlock)
  popupSpeedOtherUpgradeLevel.value = normalizePositiveInteger(
    savedState.popupSpeedOtherUpgradeLevel ?? savedState.popupSpeedUpgradeLevel,
    0,
    0,
  )
  cookCount.value = normalizePositiveInteger(savedState.cookCount, 0, 0)
  chickenCap.value = normalizeCapValue(savedState.chickenCap)
  areCollisionsEnabled.value = savedState.areCollisionsEnabled !== false
  isPerformanceMode.value = Boolean(savedState.isPerformanceMode)
  hasBankUnlock.value = Boolean(savedState.hasBankUnlock)
  bankChickenStored.value = normalizePositiveInteger(savedState.bankChickenStored, 0, 0)
  hasFactoryUnlock.value = Boolean(savedState.hasFactoryUnlock)
  factoryCount.value = normalizePositiveInteger(savedState.factoryCount, 0, 0)
  chickenHueShift.value = normalizePositiveInteger(savedState.chickenHueShift, 0, 0, 360)
  rainbowCycleEnabled.value = Boolean(savedState.rainbowCycleEnabled)
  manualChickenClicks.value = normalizePositiveInteger(savedState.manualChickenClicks, 0, 0)
  rarePopupShownCount.value = normalizePositiveInteger(savedState.rarePopupShownCount, 0, 0)
  totalCookProduced.value = normalizePositiveInteger(savedState.totalCookProduced, 0, 0)
  totalFactoryProduced.value = normalizePositiveInteger(savedState.totalFactoryProduced, 0, 0)
  totalBankDeposited.value = normalizePositiveInteger(savedState.totalBankDeposited, 0, 0)
  clickPowerUpgradeLevel.value = normalizePositiveInteger(savedState.clickPowerUpgradeLevel, 0, 0, 10)
  cookCpsUpgradeLevel.value = normalizePositiveInteger(savedState.cookCpsUpgradeLevel, 0, 0, 5)
  factoryCpsUpgradeLevel.value = normalizePositiveInteger(savedState.factoryCpsUpgradeLevel, 0, 0, 5)
  bankDecayDelayUpgradeLevel.value = normalizePositiveInteger(savedState.bankDecayDelayUpgradeLevel, 0, 0, 5)
  rareChanceUpgradeLevel.value = normalizePositiveInteger(savedState.rareChanceUpgradeLevel, 0, 0, 3)

  // Restore bank efficiency percent if present, else default to 35 + cookCount (capped at 90)
  if (typeof savedState.bankEfficiencyPercent === 'number') {
    bankEfficiencyPercent.value = Math.min(90, Math.max(35, savedState.bankEfficiencyPercent))
  } else {
    bankEfficiencyPercent.value = Math.min(90, 35 + cookCount.value)
  }

  bankDecayDelayRemainingMs = bankDecayDelayMs.value
  shouldFlashRightMenu.value = shouldShowRightMenuAffordHint.value

  syncSoundPreference()
  enforceChickenCap()
  syncRenderedChickens()
}

const handleLoadSavedProgress = () => {
  if (pendingSavedState.value) {
    applySavedProgress(pendingSavedState.value)
  }

  sessionSavingEnabled.value = true
  autosaveStatus.value = 'Autosave: On'
  showSavePrompt.value = false
  pendingSavedState.value = null
  writeAutosave()
}

const handleKeepWithoutLoad = () => {
  sessionSavingEnabled.value = false
  autosaveStatus.value = 'Autosave: Off (session)'
  showSavePrompt.value = false
  pendingSavedState.value = null
}

const closeConfirmPrompt = () => {
  activeConfirmPrompt.value = null
  deleteSaveConfirmationText.value = ''
}

const confirmActivePrompt = () => {
  const prompt = activeConfirmPrompt.value
  if (!prompt) {
    return
  }

  if (prompt.requiresTypedConfirmation) {
    if (deleteSaveConfirmationText.value !== prompt.requiredTypedValue) {
      return
    }
  }

  closeConfirmPrompt()
  prompt.onConfirm?.()
}

const cancelActivePrompt = () => {
  const prompt = activeConfirmPrompt.value
  if (!prompt) {
    return
  }

  closeConfirmPrompt()
  prompt.onCancel?.()
}

const findChickenById = (id) => {
  return chickens.value.find((chicken) => chicken.id === id) || null
}

const handleChickenPointerDown = (chicken, event) => {
  activeDragId = chicken.id
  chicken.isDragging = true
  chicken.movedWhileDragging = false
  chicken.dragOffsetX = event.clientX - chicken.x
  chicken.dragOffsetY = event.clientY - chicken.y
  chicken.pointerLastX = event.clientX
  chicken.pointerLastY = event.clientY
  chicken.pointerLastTime = event.timeStamp || performance.now()
  chicken.dragVelocityX = 0
  chicken.dragVelocityY = 0
  chicken.dragAngularVelocity = 0

  if (event.target && typeof event.target.setPointerCapture === 'function') {
    event.target.setPointerCapture(event.pointerId)
  }
}

const handlePointerMove = (event) => {
  if (activeDragId === null) {
    return
  }

  const chicken = findChickenById(activeDragId)
  if (!chicken) {
    activeDragId = null
    return
  }

  const nextX = event.clientX - chicken.dragOffsetX
  const nextY = event.clientY - chicken.dragOffsetY
  const movedDistance = Math.abs(nextX - chicken.x) + Math.abs(nextY - chicken.y)
  const eventTime = event.timeStamp || performance.now()
  const elapsedSeconds = Math.max(0.001, (eventTime - chicken.pointerLastTime) / 1000)
  const pointerDeltaX = event.clientX - chicken.pointerLastX
  const pointerDeltaY = event.clientY - chicken.pointerLastY

  const sampledVelocityX = pointerDeltaX / elapsedSeconds
  const sampledVelocityY = pointerDeltaY / elapsedSeconds
  const sampledAngularVelocity = ((pointerDeltaX - pointerDeltaY) / elapsedSeconds) * 0.08

  chicken.dragVelocityX = chicken.dragVelocityX * 0.35 + sampledVelocityX * 0.65
  chicken.dragVelocityY = chicken.dragVelocityY * 0.35 + sampledVelocityY * 0.65
  chicken.dragAngularVelocity = chicken.dragAngularVelocity * 0.45 + sampledAngularVelocity * 0.55

  chicken.pointerLastX = event.clientX
  chicken.pointerLastY = event.clientY
  chicken.pointerLastTime = eventTime

  if (movedDistance > 3) {
    chicken.movedWhileDragging = true
  }

  chicken.x = nextX
  chicken.y = nextY
  clampPosition(chicken)
}

const handlePointerUp = (event) => {
  if (activeDragId === null) {
    return
  }

  const chicken = findChickenById(activeDragId)
  if (chicken) {
    const releaseTime = event?.timeStamp || performance.now()
    const elapsedSeconds = Math.max(0.001, (releaseTime - chicken.pointerLastTime) / 1000)
    const releaseDeltaX = (event?.clientX ?? chicken.pointerLastX) - chicken.pointerLastX
    const releaseDeltaY = (event?.clientY ?? chicken.pointerLastY) - chicken.pointerLastY

    if (releaseDeltaX !== 0 || releaseDeltaY !== 0) {
      const releaseVelocityX = releaseDeltaX / elapsedSeconds
      const releaseVelocityY = releaseDeltaY / elapsedSeconds
      const releaseAngularVelocity = ((releaseDeltaX - releaseDeltaY) / elapsedSeconds) * 0.08

      chicken.dragVelocityX = chicken.dragVelocityX * 0.5 + releaseVelocityX * 0.5
      chicken.dragVelocityY = chicken.dragVelocityY * 0.5 + releaseVelocityY * 0.5
      chicken.dragAngularVelocity = chicken.dragAngularVelocity * 0.5 + releaseAngularVelocity * 0.5
    }

    chicken.isDragging = false

    if (chicken.movedWhileDragging) {
      chicken.velocityX = capSpeed(chicken.dragVelocityX, 420)
      chicken.velocityY = capSpeed(chicken.dragVelocityY, 420)
      chicken.angularVelocity = capSpeed(chicken.dragAngularVelocity, 540)
    } else {
      if (Math.abs(chicken.velocityX) < 60) {
        chicken.velocityX = chicken.velocityX >= 0 ? 120 : -120
      }
      if (Math.abs(chicken.velocityY) < 60) {
        chicken.velocityY = chicken.velocityY >= 0 ? 120 : -120
      }
      if (Math.abs(chicken.angularVelocity) < 30) {
        chicken.angularVelocity = chicken.angularVelocity >= 0 ? 80 : -80
      }
    }

    chicken.pointerLastTime = releaseTime
  }

  activeDragId = null
}

const handleChickenClick = async (chicken, event) => {
  if (chicken.movedWhileDragging) {
    chicken.movedWhileDragging = false
    return
  }

  await playChickenAudio()

  let clickValue = 1
  if (chicken.type === 'breast') {
    clickValue = 2
  }
  clickValue *= clickPowerMultiplier.value
  manualChickenClicks.value += 1

  addChicken(clickValue)

  // Create floating number
  const floatingId = Math.random()
  const floatingNumber = {
    id: floatingId,
    value: clickValue,
    x: event.clientX,
    y: event.clientY,
  }
  floatingNumbers.value.push(floatingNumber)

  // Remove after animation
  window.setTimeout(() => {
    floatingNumbers.value = floatingNumbers.value.filter(n => n.id !== floatingId)
  }, 1000)
}

const resolveChickenCollisions = () => {
  const chickenList = chickens.value

  for (let firstIndex = 0; firstIndex < chickenList.length; firstIndex += 1) {
    const firstChicken = chickenList[firstIndex]
    if (firstChicken.isDragging) {
      continue
    }

    for (let secondIndex = firstIndex + 1; secondIndex < chickenList.length; secondIndex += 1) {
      const secondChicken = chickenList[secondIndex]
      if (secondChicken.isDragging) {
        continue
      }

      const firstCenterX = firstChicken.x + firstChicken.size / 2
      const firstCenterY = firstChicken.y + firstChicken.size / 2
      const secondCenterX = secondChicken.x + secondChicken.size / 2
      const secondCenterY = secondChicken.y + secondChicken.size / 2

      let deltaX = secondCenterX - firstCenterX
      let deltaY = secondCenterY - firstCenterY
      let distanceSquared = deltaX * deltaX + deltaY * deltaY

      if (distanceSquared === 0) {
        deltaX = randomBetween(-1, 1)
        deltaY = randomBetween(-1, 1)
        distanceSquared = deltaX * deltaX + deltaY * deltaY
      }

      const firstRadius = firstChicken.size * 0.45
      const secondRadius = secondChicken.size * 0.45
      const minimumDistance = firstRadius + secondRadius

      if (distanceSquared >= minimumDistance * minimumDistance) {
        continue
      }

      const distance = Math.sqrt(distanceSquared)
      const normalX = deltaX / distance
      const normalY = deltaY / distance
      const overlap = minimumDistance - distance

      firstChicken.x -= normalX * overlap * 0.5
      firstChicken.y -= normalY * overlap * 0.5
      secondChicken.x += normalX * overlap * 0.5
      secondChicken.y += normalY * overlap * 0.5

      clampPosition(firstChicken)
      clampPosition(secondChicken)

      const relativeVelocityX = secondChicken.velocityX - firstChicken.velocityX
      const relativeVelocityY = secondChicken.velocityY - firstChicken.velocityY
      const velocityAlongNormal = relativeVelocityX * normalX + relativeVelocityY * normalY

      if (velocityAlongNormal >= 0) {
        continue
      }

      const bounceFactor = 0.95
      const impulse = (-(1 + bounceFactor) * velocityAlongNormal) / 2

      firstChicken.velocityX -= impulse * normalX
      firstChicken.velocityY -= impulse * normalY
      secondChicken.velocityX += impulse * normalX
      secondChicken.velocityY += impulse * normalY

      firstChicken.velocityX = capSpeed(firstChicken.velocityX, 320)
      firstChicken.velocityY = capSpeed(firstChicken.velocityY, 320)
      secondChicken.velocityX = capSpeed(secondChicken.velocityX, 320)
      secondChicken.velocityY = capSpeed(secondChicken.velocityY, 320)

      firstChicken.angularVelocity = capSpeed(firstChicken.angularVelocity - impulse * 0.08, 540)
      secondChicken.angularVelocity = capSpeed(secondChicken.angularVelocity + impulse * 0.08, 540)
    }
  }
}

const animateChickens = (timestamp) => {
  if (previousTimestamp === 0) {
    previousTimestamp = timestamp
  }

  const frameMs = Math.max(1, timestamp - previousTimestamp)
  const deltaSeconds = Math.min(0.032, frameMs / 1000)
  previousTimestamp = timestamp
  const instantaneousFps = 1000 / frameMs

  smoothedFps = smoothedFps * 0.9 + instantaneousFps * 0.1
  fpsCounter.value = Math.round(smoothedFps)

  if (rainbowCycleEnabled.value) {
    rainbowCycleHue = (rainbowCycleHue + 60 * deltaSeconds) % 360
    chickenHueShift.value = Math.round(rainbowCycleHue)
  }

  if (bankChickenStored.value > 0) {
    if (bankDecayDelayRemainingMs > 0) {
      bankDecayDelayRemainingMs = Math.max(0, bankDecayDelayRemainingMs - frameMs)
    } else {
      bankChickenStored.value *= Math.pow(0.99, deltaSeconds / 60)
    }
  }

  let totalProducedWhole = 0

  const cookRate = cookOutputPerSecond.value
  if (cookRate > 0) {
    const cookProduced = cookRate * deltaSeconds + cookProductionCarry
    const cookProducedWhole = Math.floor(cookProduced)
    cookProductionCarry = cookProduced - cookProducedWhole

    if (cookProducedWhole > 0) {
      totalCookProduced.value += cookProducedWhole
      totalProducedWhole += cookProducedWhole
    }
  }

  const factoryRate = factoryCpsGeneration.value
  if (factoryRate > 0) {
    const factoryProduced = factoryRate * deltaSeconds + factoryProductionCarry
    const factoryProducedWhole = Math.floor(factoryProduced)
    factoryProductionCarry = factoryProduced - factoryProducedWhole

    if (factoryProducedWhole > 0) {
      totalFactoryProduced.value += factoryProducedWhole
      totalProducedWhole += factoryProducedWhole
    }
  }

  const bankRate = bankCpsGeneration.value
  if (bankRate > 0) {
    const bankProduced = bankRate * deltaSeconds + bankProductionCarry
    const bankProducedWhole = Math.floor(bankProduced)
    bankProductionCarry = bankProduced - bankProducedWhole

    if (bankProducedWhole > 0) {
      totalProducedWhole += bankProducedWhole
    }
  }

  if (totalProducedWhole > 0) {
    addChicken(totalProducedWhole)
  }

  const elapsedSeconds = Math.max(1, (timestamp - sessionStartTimestamp) / 1000)
  averageCps.value = totalChickenEarned / elapsedSeconds

  const linearDamping = Math.exp(-linearFrictionPerSecond * deltaSeconds)
  const angularDamping = Math.exp(-angularFrictionPerSecond * deltaSeconds)

  if (!document.hidden) {
    if (areCollisionsEnabled.value) {
      stableFpsDurationMs = 0

      if (instantaneousFps < 26) {
        lowFpsDurationMs += frameMs
      } else {
        lowFpsDurationMs = Math.max(0, lowFpsDurationMs - frameMs)
      }

      if (lowFpsDurationMs >= 2000 && chickens.value.length >= 8) {
        areCollisionsEnabled.value = false
        isPerformanceMode.value = true
        performanceNoticeVisible.value = true
        window.setTimeout(() => {
          performanceNoticeVisible.value = false
        }, 10000)
        if (isUnlimitedCap.value) {
          isUnlimitedCap.value = false
          syncRenderedChickens()
        }
        stableFpsDurationMs = 0
      }
    } else {
      if (smoothedFps >= 40) {
        stableFpsDurationMs += frameMs
      } else {
        stableFpsDurationMs = Math.max(0, stableFpsDurationMs - frameMs * 2)
      }

      if (stableFpsDurationMs >= 30000) {
        isPerformanceMode.value = false
        areCollisionsEnabled.value = true
        lowFpsDurationMs = 0
        stableFpsDurationMs = 0
      }
    }
  }

  chickens.value.forEach((chicken) => {
    if (chicken.isDragging) {
      return
    }

    chicken.velocityX *= linearDamping
    chicken.velocityY *= linearDamping
    chicken.angularVelocity *= angularDamping

    if (Math.abs(chicken.velocityX) < 0.5) {
      chicken.velocityX = 0
    }
    if (Math.abs(chicken.velocityY) < 0.5) {
      chicken.velocityY = 0
    }
    if (Math.abs(chicken.angularVelocity) < 1.2) {
      chicken.angularVelocity = 0
    }

    chicken.x += chicken.velocityX * deltaSeconds
    chicken.y += chicken.velocityY * deltaSeconds
    chicken.angle = (chicken.angle + chicken.angularVelocity * deltaSeconds) % 360

    const maxX = Math.max(0, viewportWidth.value - chicken.size)
    const maxY = Math.max(0, viewportHeight.value - chicken.size)

    if (chicken.x <= 0) {
      chicken.x = 0
      chicken.velocityX = Math.abs(chicken.velocityX)
    } else if (chicken.x >= maxX) {
      chicken.x = maxX
      chicken.velocityX = -Math.abs(chicken.velocityX)
    }

    if (chicken.y <= 0) {
      chicken.y = 0
      chicken.velocityY = Math.abs(chicken.velocityY)
    } else if (chicken.y >= maxY) {
      chicken.y = maxY
      chicken.velocityY = -Math.abs(chicken.velocityY)
    }
  })

  if (areCollisionsEnabled.value) {
    resolveChickenCollisions()
  }

  animationFrameId = window.requestAnimationFrame(animateChickens)
}

onMounted(() => {
  window.forceBCAGPopup = forceRarePopup

  document.title = 'Barbeque Chicken Alert'
  updateViewport()
  sessionStartTimestamp = performance.now()
  totalChickenEarned = 0
  cookProductionCarry = 0
  bankProductionCarry = 0
  factoryProductionCarry = 0
  bankDecayDelayRemainingMs = bankDecayDelayMs.value
  shouldFlashRightMenu.value = shouldShowRightMenuAffordHint.value
  syncRenderedChickens()

  try {
    const serializedSave = window.localStorage.getItem(saveStorageKey)
    if (serializedSave) {
      const parsedSave = JSON.parse(serializedSave)
      if (parsedSave && typeof parsedSave === 'object') {
        pendingSavedState.value = parsedSave
        showSavePrompt.value = true
      }
    }
  } catch {
    pendingSavedState.value = null
  }

  if (showSavePrompt.value) {
    sessionSavingEnabled.value = false
    autosaveStatus.value = 'Autosave: Paused'
  }

  popupTimerId = window.setTimeout(spawnPopup, currentPopupIntervalMs.value)
  window.addEventListener('pointermove', handlePointerMove)
  window.addEventListener('pointerup', handlePointerUp)
  window.addEventListener('pointercancel', handlePointerUp)
  window.addEventListener('resize', updateViewport)

  if (window.visualViewport) {
    window.visualViewport.addEventListener('resize', updateViewport)
    window.visualViewport.addEventListener('scroll', updateViewport)
  }

  // Prevent double-tap zoom on iOS
  touchStartHandler = (e) => {
    if (e.target instanceof Element && e.target.closest(iOSInteractiveTouchSelector)) {
      touchStartHandler.lastTouchTime = Date.now()
      return
    }

    const now = Date.now()
    if (now - (touchStartHandler.lastTouchTime || 0) < 300 && e.touches.length === 1) {
      e.preventDefault()
    }
    touchStartHandler.lastTouchTime = now
  }

  // Allow pinch zoom-out, but block pinch zoom-in beyond the default scale
  gestureStartHandler = () => {}
  gestureChangeHandler = (e) => {
    if (e.scale > 1) {
      e.preventDefault()
    }
  }

  document.addEventListener('touchstart', touchStartHandler, { passive: false })
  document.addEventListener('gesturestart', gestureStartHandler, { passive: false })
  document.addEventListener('gesturechange', gestureChangeHandler, { passive: false })

  // Track document visibility to avoid blocking background music
  handleVisibilityChange = () => {
    isDocumentVisible.value = !document.hidden
  }
  document.addEventListener('visibilitychange', handleVisibilityChange)

  animationFrameId = window.requestAnimationFrame(animateChickens)
})

onUnmounted(() => {
  if (window.forceBCAGPopup === forceRarePopup) {
    delete window.forceBCAGPopup
  }

  if (popupTimerId !== null) {
    window.clearTimeout(popupTimerId)
  }
  if (animationFrameId !== null) {
    window.cancelAnimationFrame(animationFrameId)
  }

  window.removeEventListener('resize', updateViewport)
  window.removeEventListener('pointermove', handlePointerMove)
  window.removeEventListener('pointerup', handlePointerUp)
  window.removeEventListener('pointercancel', handlePointerUp)
  window.removeEventListener('visibilitychange', handleVisibilityChange)

  if (window.visualViewport) {
    window.visualViewport.removeEventListener('resize', updateViewport)
    window.visualViewport.removeEventListener('scroll', updateViewport)
  }

  if (touchStartHandler) {
    document.removeEventListener('touchstart', touchStartHandler)
  }
  if (gestureStartHandler) {
    document.removeEventListener('gesturestart', gestureStartHandler)
  }
  if (gestureChangeHandler) {
    document.removeEventListener('gesturechange', gestureChangeHandler)
  }

  popupAudio.pause()
  popupAudio.currentTime = 0
  chickenAudio.pause()
  chickenAudio.currentTime = 0
  rarePopupAudio.pause()
  rarePopupAudio.currentTime = 0
})

watch(
  [
    totalChickenCount,
    chickensPerPopup,
    upgradeLevel,
    hasAutoPopupClickUpgrade,
    isAutoClickerEnabled,
    isSoundEnabled,
    rebirthCount,
    hasChickenBreastUnlock,
    popupSpeedOtherUpgradeLevel,
    cookCount,
    chickenCap,
    areCollisionsEnabled,
    isPerformanceMode,
    manualChickenClicks,
    rarePopupShownCount,
    clickPowerUpgradeLevel,
    cookCpsUpgradeLevel,
    factoryCpsUpgradeLevel,
    bankDecayDelayUpgradeLevel,
    rareChanceUpgradeLevel,
    totalCookProduced,
    totalFactoryProduced,
    totalBankDeposited,
  ],
  () => {
    writeAutosave()
  },
)

watch(totalChickenCount, () => {
  syncRenderedChickens()
})
</script>

<template>
  <div class="black-screen" style="touch-action: none; user-select: none;">
    <div class="hud-left">
      <button
        :class="['menu-button', { 'flash-button': shouldFlashLeftMenu, 'menu-button--locked': !canOpenLeftMenu }]"
        type="button"
        aria-label="Open left menu"
        :aria-disabled="!canOpenLeftMenu"
        @click="handleLeftMenuButtonClick"
      >☰</button>
    </div>

    <div class="hud">
      <button :class="['menu-button', { 'flash-button-white': shouldFlashRightMenu }]" type="button" aria-label="Open menu" @click="toggleMenu">☰</button>
      <button class="menu-button help-button" type="button" aria-label="Open help" @click="openHelp">?</button>
      <div class="counter">Chicken: {{ chickenCount }}</div>
    </div>

    <div v-if="!isCompactHud" class="save-status">{{ autosaveStatus }}</div>

    <div v-if="isHelpOpen" class="save-overlay" @click.self="closeHelp">
      <div class="save-dialog help-dialog">
        <div class="save-title">Quick Help</div>
        <div class="help-list">
          <div class="save-text"><strong>How to Play:</strong> Click chickens to earn. Use chickens to buy Cooks and Factories that generate chickens passively. Spend time clicking popups for instant rewards.</div>
          <div class="save-text"><strong>Clicks:</strong> Wing = 1, Breast = 2. Earn more per click as you progress.</div>
          <div class="save-text"><strong>Popups:</strong> Spawn every ~3s. Click for instant chickens. Types: Normal (base), Rare ({{ (baseRarePopupChance * 100).toFixed(2) }}%, {{ rarePopupMultiplier }}x), Extremely Rare ({{ (extremelyRarePopupChance * 100).toFixed(4) }}%, {{ extremelyRarePopupMultiplier }}x).</div>
          <div class="save-text"><strong>Cooks:</strong> Cost {{ cookBaseCost }}-{{ getCookCost(5) }} each. Generate (1 + rebirth count) × efficiency chickens/s. Each cook adds +1% bank efficiency (max 90%).</div>
          <div class="save-text"><strong>Factory:</strong> Cost {{ factoryUnlockCost }}-{{ getFactoryCost(5) }} each. Generate 2 × rebirth multiplier × efficiency chickens/s.</div>
          <div class="save-text"><strong>Bank:</strong> Deposit chickens for passive income (CPS = stored × efficiency × 0.01). Decays 1%/min after delay. Unlocked as you progress.</div>
          <div class="save-text"><strong>Rebirth:</strong> Reset progress to gain +0.5× multiplier + choose permanent upgrades. Unlocks as you progress. Left menu available after first rebirth.</div>
          <div class="save-text"><strong>Mobile:</strong> First tap to reveal upgrade details, second tap to buy. Sound mutes when app is backgrounded.</div>
        </div>
        <div class="prompt-actions">
          <button type="button" class="save-button" @click="closeHelp">Close</button>
        </div>
      </div>
    </div>

    <div v-if="lockedLeftMenuNoticeOpen" class="save-overlay" @click.self="closeLockedLeftMenuNotice">
      <div class="save-dialog help-dialog">
        <div class="save-title">Left Menu Locked</div>
        <div class="save-text">Unlock it by rebirthing once or buying the Bank upgrade.</div>
        <div class="prompt-actions">
          <button type="button" class="save-button" @click="closeLockedLeftMenuNotice">Got it</button>
        </div>
      </div>
    </div>

    <div v-if="isMenuOpen" class="menu-panel">
      <div class="menu-title">Upgrades</div>
      <div class="menu-label">Current level: {{ upgradeLevel }}</div>
      <div class="menu-label">Rare shown: {{ rarePopupShownCount }} | Rare chance: {{ (currentRarePopupChance * 100).toFixed(2) }}%</div>

      <button
        type="button"
        class="upgrade-button"
        :disabled="!canAffordUpgrade"
        @click="upgradeChickenSpawn"
      >
        {{ nextUpgradeGain }} chicken per popup — Cost: {{ nextUpgradeCost }}
      </button>

      <button
        v-if="!hasAutoPopupClickUpgrade"
        type="button"
        class="upgrade-button"
        :disabled="!canAffordAutoPopupClickUpgrade"
        @click="upgradeAutoPopupClick"
      >
        Auto-click popup — Cost: {{ autoPopupClickUpgradeCost }}
      </button>

      <div class="menu-bottom-section">
        <button
          v-if="!hasChickenBreastUnlock"
          type="button"
          class="upgrade-button"
          :disabled="!canAffordChickenBreastUnlock"
          @click="unlockChickenBreast"
        >
          Unlock Chicken Breasts — Cost: {{ chickenBreastUnlockCost }}
        </button>

        <button
          v-if="!hasBankUnlock"
          type="button"
          class="upgrade-button"
          :disabled="!canAffordBankUnlock"
          @click="unlockBank"
        >
          Unlock Bank — Cost: {{ bankUnlockCost }}
        </button>

        <button
          v-if="!hasFactoryUnlock"
          type="button"
          class="upgrade-button"
          :disabled="!canAffordFactoryUnlock"
          @click="unlockFactory"
        >
          Unlock Factory — Cost: {{ factoryUnlockCost }}
        </button>

        <button
          v-if="hasFactoryUnlock"
          type="button"
          class="upgrade-button"
          :disabled="!canAffordFactory"
          @click="buyFactory"
        >
          Buy Factory ({{ factoryCount }}) — {{ factoryCpsGeneration.toFixed(1) }} CPS — Cost: {{ nextFactoryCost }}
        </button>

        <button
          type="button"
          class="upgrade-button"
          :disabled="!canAffordCook"
          @click="hireCook"
        >
          Hire Cook ({{ cookCount }}) — {{ cookOutputPerSecond.toFixed(1) }} CPS — Cost: {{ nextCookCost }}
        </button>

        <button
          type="button"
          class="upgrade-button"
          @click="toggleOtherUpgrades"
        >
          Other Upgrades
        </button>

        <label class="menu-label" for="chicken-cap-input">Rendered chicken cap</label>
        <input
          id="chicken-cap-input"
          v-model.number="chickenCap"
          class="menu-input"
          type="number"
          min="25"
          max="2000"
          step="5"
          @change="applyChickenCap"
        >

        <label class="menu-toggle">
          <input
            :checked="isUnlimitedCap"
            type="checkbox"
            @change="requestUnlimitedCapToggle"
          >
          Unlimited cap
        </label>

          <label class="menu-toggle">
            <input v-model="isSoundEnabled" type="checkbox" @change="syncSoundPreference">
            <span>Sound</span>
          </label>

          <div class="rebirth-panel">
            <div class="menu-title">Rebirth</div>
            <div class="menu-label">Multiplier: ×{{ rebirthMultiplier.toFixed(1) }} | Rebirths: {{ rebirthCount }}</div>
            <button
              type="button"
              class="upgrade-button rebirth-button"
              :disabled="!canAffordRebirth"
              @click="openRebirthMenu"
            >
              Rebirth at Level {{ nextRebirthLevelRequirement }} (+0.5× gain)
            </button>

            <div v-if="rebirthMenu.visible" class="rebirth-overlay">
              <div class="save-dialog">
                <div class="save-title">Rebirth Options</div>
                <div class="save-text">You will gain {{ Math.floor(totalChickenCount * 0.5 * 0.0001) }} RC (Rebirth Chickens).</div>
                <div class="save-text">Choose a permanent upgrade to unlock (costs RC):</div>
                <div class="rebirth-upgrade-list" style="display: flex; flex-direction: column; gap: 0.5em;">
                  <label v-for="opt in rebirthUpgradeOptions" :key="opt.key" class="rebirth-upgrade-option" style="display: flex; align-items: center; gap: 0.5em;">
                    <input type="radio" v-model="rebirthMenu.selected" :value="opt.key">
                    <span>
                      {{ opt.label }}
                      <span v-if="opt.key === 'mult'"> (costs {{ getBaseRebirthUpgradeCost('mult') }} RC)</span>
                      <span v-else-if="opt.key === 'startWithChickens'"> (costs {{ getEffectiveRebirthUpgradeCost('startWithChickens') }} RC)</span>
                      <span v-else-if="opt.key === 'earlyChickenUpgrade'"> (costs {{ getEffectiveRebirthUpgradeCost('earlyChickenUpgrade') }} RC)</span>
                      <span v-if="opt.key === 'startWithChickens' && rebirthUpgrades.startWithChickens"> (Unlocked)</span>
                      <span v-if="opt.key === 'earlyChickenUpgrade' && rebirthUpgrades.earlyChickenUpgrade"> (Unlocked)</span>
                    </span>
                  </label>
                </div>
                <div class="save-text" style="color: #e55; min-height: 1.5em;">{{ rebirthMenu.error }}</div>
                <div class="prompt-actions">
                  <button type="button" class="save-button" @click="confirmRebirthMenu">Confirm</button>
                  <button type="button" class="save-button" @click="cancelRebirthMenu">Cancel</button>
                </div>
                <div class="save-text">RC after rebirth: {{ projectedRebirthRcBalance }}</div>
              </div>
            </div>
          </div>

        <div class="menu-actions">
          <button type="button" class="menu-action-button" @click="reportBug">Report bug</button>
          <button type="button" class="menu-action-button" @click="exportSaveFile">Export save</button>
          <button type="button" class="menu-action-button" @click="triggerSaveImport">Import save</button>
          <button type="button" class="menu-action-button danger" @click="confirmAndDeleteSave">
            Delete save
          </button>
        </div>

        <button type="button" class="repo-link" @click="openRepository" aria-label="Open GitHub repository">
          <svg viewBox="0 0 24 24" class="repo-icon" aria-hidden="true">
            <path
              d="M12 2C6.48 2 2 6.59 2 12.25c0 4.52 2.87 8.35 6.84 9.7.5.09.66-.22.66-.49 0-.24-.01-.88-.01-1.73-2.78.62-3.37-1.38-3.37-1.38-.45-1.18-1.11-1.49-1.11-1.49-.9-.64.07-.63.07-.63 1 .07 1.52 1.05 1.52 1.05.88 1.55 2.31 1.1 2.87.84.09-.66.35-1.1.64-1.36-2.22-.26-4.56-1.14-4.56-5.08 0-1.12.39-2.04 1.03-2.76-.1-.26-.45-1.3.1-2.72 0 0 .84-.28 2.75 1.06A9.33 9.33 0 0 1 12 6.98c.85 0 1.7.12 2.5.35 1.9-1.34 2.74-1.06 2.74-1.06.56 1.42.21 2.46.1 2.72.64.72 1.03 1.64 1.03 2.76 0 3.95-2.34 4.81-4.58 5.07.36.32.68.93.68 1.88 0 1.36-.01 2.46-.01 2.79 0 .27.16.59.67.49A10.26 10.26 0 0 0 22 12.25C22 6.59 17.52 2 12 2Z"
            />
          </svg>
          Repository
        </button>
      </div>
    </div>

    <div v-if="isOtherUpgradesOpen" class="save-overlay" @click.self="closeOtherUpgrades">
      <div class="save-dialog help-dialog other-upgrades-dialog">
        <div class="save-title">Other Upgrades</div>
        <div class="menu-label">Unlocks appear when requirements are met.</div>
        <div class="other-upgrade-grid">
          <button
            v-if="!isPopupSpeedFullyUpgraded"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'popup-speed' }"
            :disabled="!canAffordPopupSpeedUpgrade"
            @click="handleUpgradeCardClick('popup-speed', upgradePopupSpeed)"
            @mouseenter="hoveredUpgradeId = 'popup-speed'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[>>]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'popup-speed'">
              <div class="upgrade-card-title">Faster Popups</div>
              <div class="upgrade-card-desc">{{ (currentPopupIntervalMs / 1000).toFixed(2) }}s &mdash; {{ nextPopupSpeedCost }}</div>
            </div>
          </button>

          <button
            v-if="isClickPowerUpgradeUnlocked && clickPowerUpgradeLevel < 10"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'click-power' }"
            :disabled="!canAffordClickPowerUpgrade"
            @click="handleUpgradeCardClick('click-power', upgradeClickPower)"
            @mouseenter="hoveredUpgradeId = 'click-power'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[CLK]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'click-power'">
              <div class="upgrade-card-title">Click Power</div>
              <div class="upgrade-card-desc">{{ nextClickPowerCost }}</div>
            </div>
          </button>

          <button
            v-if="isCookUpgradeUnlocked && cookCpsUpgradeLevel < 5"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'cook-cps' }"
            :disabled="!canAffordCookCpsUpgrade"
            @click="handleUpgradeCardClick('cook-cps', upgradeCookCps)"
            @mouseenter="hoveredUpgradeId = 'cook-cps'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[C]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'cook-cps'">
              <div class="upgrade-card-title">Cook Eff.</div>
              <div class="upgrade-card-desc">{{ nextCookCpsUpgradeCost }}</div>
            </div>
          </button>

          <button
            v-if="isFactoryUpgradeUnlocked && factoryCpsUpgradeLevel < 5"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'factory-cps' }"
            :disabled="!canAffordFactoryCpsUpgrade"
            @click="handleUpgradeCardClick('factory-cps', upgradeFactoryCps)"
            @mouseenter="hoveredUpgradeId = 'factory-cps'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[F]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'factory-cps'">
              <div class="upgrade-card-title">Factory Eff.</div>
              <div class="upgrade-card-desc">{{ nextFactoryCpsUpgradeCost }}</div>
            </div>
          </button>

          <button
            v-if="isBankDelayUpgradeUnlocked && bankDecayDelayUpgradeLevel < 5"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'bank-delay' }"
            :disabled="!canAffordBankDelayUpgrade"
            @click="handleUpgradeCardClick('bank-delay', upgradeBankDecayDelay)"
            @mouseenter="hoveredUpgradeId = 'bank-delay'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[$]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'bank-delay'">
              <div class="upgrade-card-title">Bank Delay</div>
              <div class="upgrade-card-desc">{{ nextBankDelayUpgradeCost }}</div>
            </div>
          </button>

          <button
            v-if="isRareChanceUpgradeUnlocked && rareChanceUpgradeLevel < 3"
            type="button"
            class="upgrade-card"
            :class="{ 'upgrade-card--hovered': hoveredUpgradeId === 'rare-chance' }"
            :disabled="!canAffordRareChanceUpgrade"
            @click="handleUpgradeCardClick('rare-chance', upgradeRareChance)"
            @mouseenter="hoveredUpgradeId = 'rare-chance'"
            @mouseleave="hoveredUpgradeId = null"
          >
            <div class="upgrade-card-icon">[*]</div>
            <div class="upgrade-card-details" v-if="hoveredUpgradeId === 'rare-chance'">
              <div class="upgrade-card-title">Rare Chance</div>
              <div class="upgrade-card-desc">{{ nextRareChanceUpgradeCost }}</div>
            </div>
          </button>
        </div>

        <div class="prompt-actions">
          <button type="button" class="save-button" @click="closeOtherUpgrades">Close</button>
        </div>
      </div>
    </div>

    <div v-if="isLeftMenuOpen && (rebirthCount > 0 || hasBankUnlock)" class="left-menu-panel">
      <div v-if="hasBankUnlock" class="bank-section">
        <div class="menu-title">Bank</div>
        <div class="menu-label">Stored: {{ Math.floor(bankChickenStored) }} | Bank CPS: {{ bankCpsGeneration.toFixed(2) }}</div>

        <div class="bank-input-group">
          <input
            id="deposit-amount"
            v-model.number="bankDepositAmount"
            class="menu-input"
            type="number"
            min="0"
            placeholder="Amount to deposit"
          >
          <button
            type="button"
            class="upgrade-button"
            @click="depositToBank(bankDepositAmount)"
          >
            Deposit
          </button>
        </div>

        <div class="bank-input-group">
          <input
            id="withdraw-amount"
            v-model.number="bankWithdrawAmount"
            class="menu-input"
            type="number"
            min="0"
            placeholder="Amount to withdraw"
          >
          <button
            type="button"
            class="upgrade-button"
            @click="withdrawFromBank(bankWithdrawAmount)"
          >
            Withdraw
          </button>
        </div>
      </div>

      <div v-if="rebirthCount > 0" class="hue-section">
        <label for="hue-slider" class="menu-label">Hue Shift: {{ chickenHueShift }}°</label>
        <input
          id="hue-slider"
          v-model.number="chickenHueShift"
          :disabled="rainbowCycleEnabled"
          class="hue-slider"
          type="range"
          min="0"
          max="360"
          step="1"
        />
        <label class="menu-toggle">
          <input v-model="rainbowCycleEnabled" type="checkbox">
          <span>Rainbow Cycle</span>
        </label>
      </div>
    </div>

    <div v-if="performanceNoticeVisible" class="performance-notice">
      Collision physics disabled for performance.
    </div>

    <div v-if="showSavePrompt" class="save-overlay">
      <div class="save-dialog">
        <div class="save-title">Saved progress found</div>
        <div class="save-text">Choose how to start this session:</div>
        <button type="button" class="save-button" @click="handleLoadSavedProgress">Load save</button>
        <button type="button" class="save-button" @click="handleKeepWithoutLoad">
          Keep save, do not load (disable saving this session)
        </button>
      </div>
    </div>

    <div v-if="activeConfirmPrompt" class="save-overlay">
      <div class="save-dialog">
        <div class="save-title">{{ activeConfirmPrompt.title }}</div>
        <div class="save-text">{{ activeConfirmPrompt.message }}</div>
        <input
          v-if="activeConfirmPrompt.requiresTypedConfirmation"
          v-model="deleteSaveConfirmationText"
          class="menu-input"
          type="text"
          autocomplete="off"
          autocapitalize="off"
          spellcheck="false"
          placeholder="yes im sure to delete"
        >
        <div class="prompt-actions">
          <button
            type="button"
            class="save-button"
            :disabled="activeConfirmPrompt.requiresTypedConfirmation && deleteSaveConfirmationText !== activeConfirmPrompt.requiredTypedValue"
            @click="confirmActivePrompt"
          >
            {{ activeConfirmPrompt.confirmLabel || 'Confirm' }}
          </button>
          <button type="button" class="save-button" @click="cancelActivePrompt">
            {{ activeConfirmPrompt.cancelLabel || 'Cancel' }}
          </button>
        </div>
      </div>
    </div>

    <div v-if="hasAutoPopupClickUpgrade" class="auto-clicker-toggle">
      <label class="toggle-label">
        <input
          v-model="isAutoClickerEnabled"
          type="checkbox"
        >
        <span>Auto-click</span>
      </label>
    </div>

    <input
      ref="saveImportInput"
      type="file"
      accept="application/json,.json"
      class="save-import-input"
      @change="handleSaveImportSelection"
    >

    <img
      v-for="chicken in chickens"
      :key="chicken.id"
      class="chicken"
      :src="chicken.src"
      alt="Barbecue chicken"
      :style="{
        left: `${chicken.x}px`,
        top: `${chicken.y}px`,
        width: `${chicken.size}px`,
        height: `${chicken.size}px`,
        transform: `rotate(${chicken.angle}deg)`,
        filter: `hue-rotate(${chickenHueShift}deg)`,
      }"
      @pointerdown="handleChickenPointerDown(chicken, $event)"
      @click="handleChickenClick(chicken, $event)"
      draggable="false"
    />

    <img
      v-if="isPopupVisible"
      :key="popupCycleKey"
      :class="['popup-image', { 'popup-image--rare': isRarePopupActive, 'popup-image--extreme': isExtremePopupActive }]"
      :src="popupImage"
      :alt="isExtremePopupActive ? 'BCA extreme popup' : (isRarePopupActive ? 'BCAG popup' : 'BCA popup')"
      @click="handlePopupClick"
    />

    <div v-if="isPopupVisible && hasPopupTierBonus" :class="['rare-popup-label', { 'rare-popup-label--extreme': isExtremePopupActive }]">
      {{ activePopupMultiplierLabel }}
    </div>

    <div
      v-for="floatingNum in floatingNumbers"
      :key="floatingNum.id"
      class="floating-number"
      :style="{
        left: `${floatingNum.x}px`,
        top: `${floatingNum.y}px`,
      }"
    >
      +{{ floatingNum.value }}
    </div>
  </div>
</template>

<style scoped>
.black-screen {
  position: relative;
  min-height: 100vh;
  width: 100%;
  overflow: hidden;
  background: #000;
}

.hud {
  position: fixed;
  top: 1rem;
  right: 1rem;
  z-index: 50;
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.hud-left {
  position: fixed;
  top: 1rem;
  left: 1rem;
  z-index: 50;
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.fps-hud {
  position: fixed;
  top: 1rem;
  left: 1rem;
  z-index: 50;
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.92);
  color: #f1f1f1;
  border-radius: 999px;
  padding: 0.35rem 0.75rem;
  font-size: 0.9rem;
}

.menu-button,
.counter {
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.92);
  color: #f1f1f1;
  border-radius: 999px;
  padding: 0.35rem 0.75rem;
  font-size: 0.9rem;
}

.menu-button {
  cursor: pointer;
}

.menu-button--locked {
  opacity: 0.7;
}

.help-button {
  min-width: 2rem;
  text-align: center;
  font-weight: 700;
}

.menu-panel {
  position: fixed;
  top: 3.8rem;
  right: 1rem;
  z-index: 90;
  width: min(320px, 88vw);
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.96);
  border-radius: 0.9rem;
  padding: 0.75rem;
  display: grid;
  gap: 0.55rem;
  max-height: 65vh;
  overflow-y: auto;
  pointer-events: auto;
  touch-action: manipulation;
  -webkit-overflow-scrolling: touch;
}

.menu-panel * {
  pointer-events: auto;
}

.left-menu-panel {
  position: fixed;
  top: 7rem;
  left: 1rem;
  z-index: 65;
  width: min(320px, 88vw);
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.96);
  border-radius: 0.9rem;
  padding: 0.75rem;
  display: grid;
  gap: 0.55rem;
  max-height: 65vh;
  overflow-y: auto;
  pointer-events: auto;
  touch-action: manipulation;
  -webkit-overflow-scrolling: touch;
}

.bank-input-group {
  display: flex;
  gap: 0.4rem;
}

.bank-input-group .menu-input {
  flex: 1;
}

.bank-input-group .upgrade-button {
  flex: 0 0 auto;
}

.menu-title {
  color: #f1f1f1;
  font-size: 0.85rem;
}

.menu-compact-actions {
  border: 1px solid #2f2f2f;
  background: rgba(17, 17, 17, 0.96);
  border-radius: 0.8rem;
  padding: 0.7rem;
  display: grid;
  gap: 0.45rem;
}

.menu-label {
  color: #cfcfcf;
  font-size: 0.78rem;
  display: block;
  width: 100%;
  line-height: 1.25;
}

.menu-input {
  border: 1px solid #4a4a4a;
  background: rgba(20, 20, 20, 0.95);
  color: #f7f7f7;
  border-radius: 0.6rem;
  padding: 0.45rem 0.55rem;
  font-size: 0.82rem;
  width: 100%;
  box-sizing: border-box;
}

.menu-toggle {
  display: flex;
  align-items: center;
  gap: 0.45rem;
  color: #d8d8d8;
  font-size: 0.8rem;
  width: 100%;
}

.menu-actions {
  display: grid;
  gap: 0.45rem;
}

.save-import-input {
  display: none;
}

.menu-action-button {
  border: 1px solid #474747;
  background: rgba(32, 32, 32, 0.95);
  color: #f4f4f4;
  border-radius: 0.6rem;
  padding: 0.48rem 0.58rem;
  font-size: 0.8rem;
  text-align: left;
  cursor: pointer;
}

.menu-action-button.danger {
  border-color: #6a2a2a;
  color: #ffc7c7;
}

.repo-link {
  margin-top: 0.25rem;
  border: 1px solid #3a3a3a;
  background: rgba(19, 19, 19, 0.96);
  color: #efefef;
  border-radius: 0.7rem;
  padding: 0.48rem 0.58rem;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.45rem;
  cursor: pointer;
  width: 100%;
}

.repo-icon {
  width: 1rem;
  height: 1rem;
  fill: currentColor;
}

.upgrade-button {
  border: 1px solid #4a4a4a;
  background: rgba(32, 32, 32, 0.95);
  color: #f7f7f7;
  border-radius: 0.7rem;
  padding: 0.5rem 0.65rem;
  font-size: 0.82rem;
  text-align: left;
  cursor: pointer;
}

.upgrade-button:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.upgrade-card {
  aspect-ratio: 1;
  border: 1px solid #4a4a4a;
  background: rgba(32, 32, 32, 0.95);
  color: #f7f7f7;
  border-radius: 0.6rem;
  padding: 0.4rem;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  transition: all 0.2s ease;
}

.upgrade-card:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.upgrade-card:not(:disabled):hover,
.upgrade-card:not(:disabled).upgrade-card--hovered {
  border-color: #6a6a6a;
  background: rgba(50, 50, 50, 0.98);
}

.upgrade-card-icon {
  font-size: 1.8rem;
  font-weight: 700;
  letter-spacing: -0.05em;
  transition: opacity 0.2s ease;
}

.upgrade-card--hovered .upgrade-card-icon {
  opacity: 0;
}

.upgrade-card-details {
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  padding: 0.35rem;
  background: rgba(20, 20, 20, 0.95);
  border-radius: 0.55rem;
  animation: fadeIn 0.15s ease;
}

.upgrade-card-title {
  font-size: 0.68rem;
  font-weight: 600;
  color: #e0e0e0;
  line-height: 1.1;
}

.upgrade-card-desc {
  font-size: 0.6rem;
  color: #a8a8a8;
  margin-top: 0.15rem;
  line-height: 1.15;
}

.rebirth-button {
  border-color: #5a4a3a;
  background: rgba(50, 40, 25, 0.95);
  color: #ffdbac;
}

.rebirth-panel {
  border: 1px solid #2f2418;
  background: rgba(17, 14, 10, 0.92);
  border-radius: 0.8rem;
  padding: 0.7rem;
  display: grid;
  gap: 0.45rem;
}

.hue-section {
  border: 1px solid #2f3f3f;
  background: rgba(15, 25, 25, 0.92);
  border-radius: 0.8rem;
  padding: 0.7rem;
  display: grid;
  gap: 0.45rem;
}

.bank-section {
  border: 1px solid #2c3a3c;
  background: rgba(12, 24, 28, 0.92);
  border-radius: 0.8rem;
  padding: 0.7rem;
  display: grid;
  gap: 0.45rem;
}

.hue-slider {
  width: 100%;
  height: 0.4rem;
  border-radius: 0.2rem;
  background: linear-gradient(to right, red, yellow, lime, cyan, blue, magenta, red);
  outline: none;
  cursor: pointer;
  -webkit-appearance: none;
  appearance: none;
}

.hue-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 1rem;
  height: 1rem;
  border-radius: 50%;
  background: #f0f0f0;
  cursor: pointer;
  border: 2px solid #1a1a1a;
  box-shadow: 0 0 0.3rem rgba(0, 0, 0, 0.8);
}

.hue-slider::-moz-range-thumb {
  width: 1rem;
  height: 1rem;
  border-radius: 50%;
  background: #f0f0f0;
  cursor: pointer;
  border: 2px solid #1a1a1a;
  box-shadow: 0 0 0.3rem rgba(0, 0, 0, 0.8);
}

.performance-notice {
  position: fixed;
  top: 3.75rem;
  right: 1rem;
  z-index: 50;
  border: 1px solid #634400;
  background: rgba(56, 36, 0, 0.9);
  color: #ffd88a;
  border-radius: 0.7rem;
  padding: 0.45rem 0.7rem;
  font-size: 0.78rem;
}

.save-status {
  position: fixed;
  top: 3.75rem;
  left: 1rem;
  z-index: 50;
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.92);
  color: #cfcfcf;
  border-radius: 0.6rem;
  padding: 0.35rem 0.6rem;
  font-size: 0.74rem;
}

.save-overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  background: rgba(0, 0, 0, 0.66);
  display: grid;
  place-items: center;
  padding: 1rem;
  pointer-events: auto;
}

.rebirth-overlay {
  position: relative;
  z-index: auto;
  background: transparent;
  display: block;
  padding: 0;
  margin-top: 0.4rem;
  pointer-events: none;
}

.rebirth-overlay .save-dialog {
  width: 100%;
  pointer-events: auto;
}

.save-dialog {
  width: min(430px, 95vw);
  border: 1px solid #2f2f2f;
  border-radius: 1rem;
  background: rgba(15, 15, 15, 0.98);
  padding: 1rem;
  display: grid;
  gap: 0.6rem;
  pointer-events: auto;
}

.help-dialog {
  width: min(560px, 95vw);
}

.help-list {
  display: grid;
  gap: 0.35rem;
}

.other-upgrades-dialog {
  gap: 0.8rem;
}

.other-upgrade-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 0.4rem;
}

@media (max-width: 768px) {
  .other-upgrade-grid {
    grid-template-columns: repeat(3, 1fr);
  }
}

@media (max-width: 480px) {
  .other-upgrade-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

.save-title {
  color: #f1f1f1;
  font-size: 0.95rem;
}

.save-text {
  color: #cfcfcf;
  font-size: 0.8rem;
}

.save-button {
  border: 1px solid #4a4a4a;
  background: rgba(32, 32, 32, 0.95);
  color: #f7f7f7;
  border-radius: 0.7rem;
  padding: 0.55rem 0.65rem;
  text-align: left;
  cursor: pointer;
  font-size: 0.82rem;
  pointer-events: auto;
}

.prompt-actions {
  display: flex;
  gap: 0.5rem;
}

.prompt-actions .save-button {
  flex: 1;
}

.chicken {
  position: fixed;
  z-index: 20;
  user-select: none;
  -webkit-user-drag: none;
  touch-action: none;
  cursor: grab;
}

.chicken:active {
  cursor: grabbing;
}

.auto-clicker-toggle {
  position: fixed;
  bottom: 1rem;
  right: 1rem;
  z-index: 50;
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.92);
  border-radius: 0.6rem;
  padding: 0.45rem 0.65rem;
}

.toggle-label {
  display: flex;
  align-items: center;
  gap: 0.45rem;
  color: #d8d8d8;
  font-size: 0.8rem;
  cursor: pointer;
  user-select: none;
}

.toggle-label input {
  cursor: pointer;
}

.popup-image {
  position: fixed;
  z-index: 40;
  right: clamp(1rem, 3vw, 2rem);
  bottom: clamp(1rem, 3vw, 2rem);
  width: clamp(220px, 23vw, 480px);
  max-width: 30vw;
  height: auto;
  cursor: pointer;
  animation: popup-in 220ms ease-out;
}

.popup-image--rare {
  filter: saturate(1.25) brightness(1.18) drop-shadow(0 0 1.1rem rgba(255, 219, 133, 0.95));
  animation: popup-in 220ms ease-out, rare-shimmer 1.1s ease-in-out infinite;
}

.popup-image--extreme {
  filter: saturate(1.55) brightness(1.35) drop-shadow(0 0 1.4rem rgba(255, 255, 255, 0.95));
  animation: popup-in 220ms ease-out, extreme-shimmer 0.9s ease-in-out infinite;
}

.rare-popup-label {
  position: fixed;
  z-index: 41;
  right: clamp(1rem, 3vw, 2rem);
  bottom: clamp(1rem, 3vw, 2rem);
  transform: translateY(-95%);
  border: 1px solid #846100;
  background: linear-gradient(135deg, rgba(106, 78, 0, 0.9), rgba(170, 129, 0, 0.9));
  color: #fff0c4;
  border-radius: 999px;
  padding: 0.2rem 0.6rem;
  font-size: 0.74rem;
  font-weight: 700;
  letter-spacing: 0.03em;
  pointer-events: none;
  text-shadow: 0 0 8px rgba(255, 231, 163, 0.7);
}

.rare-popup-label--extreme {
  border-color: #dfe7ff;
  background: linear-gradient(135deg, rgba(120, 126, 160, 0.95), rgba(230, 236, 255, 0.92));
  color: #0a101f;
  text-shadow: none;
}

@media (max-width: 767px) {
  .popup-image {
    left: 50%;
    right: auto;
    transform: translateX(-50%);
    width: clamp(180px, 72vw, 420px);
    max-width: 86vw;
  }

  .rare-popup-label {
    left: 50%;
    right: auto;
    transform: translate(-50%, -95%);
  }

  .hud {
    top: 0.75rem;
    right: 0.75rem;
  }

  .hud-left {
    top: 0.75rem;
    left: 0.75rem;
  }

  .fps-hud {
    top: 0.75rem;
    left: 0.75rem;
  }

  .menu-panel {
    top: 3.45rem;
    right: 0.75rem;
  }

  .left-menu-panel {
    top: 3.45rem;
    left: 0.75rem;
  }

  .performance-notice {
    top: 3.4rem;
    right: 0.75rem;
    max-width: 70vw;
  }

  .save-status {
    top: 3.4rem;
    left: 0.75rem;
  }

  .auto-clicker-toggle {
    bottom: 0.75rem;
    right: 0.75rem;
  }
}

@keyframes rare-shimmer {
  0%,
  100% {
    filter: saturate(1.15) brightness(1.1) drop-shadow(0 0 0.8rem rgba(255, 216, 138, 0.8));
  }

  50% {
    filter: saturate(1.45) brightness(1.32) drop-shadow(0 0 1.3rem rgba(255, 240, 172, 1));
  }
}

@keyframes popup-in {
  from {
    opacity: 0;
    scale: 0.95;
  }

  to {
    opacity: 1;
    scale: 1;
  }
}

@keyframes float-up {
  from {
    opacity: 1;
    transform: translateY(0) scale(1);
  }

  to {
    opacity: 0;
    transform: translateY(-60px) scale(1.2);
  }
}

@keyframes menu-white-flash {
  0%,
  100% {
    color: #f1f1f1;
    border-color: #2c2c2c;
    background: rgba(15, 15, 15, 0.92);
    transform: scale(1);
  }

  50% {
    color: #ffffff;
    border-color: #ffffff;
    background: rgba(46, 46, 46, 0.95);
    transform: scale(1.04);
  }
}

@keyframes extreme-shimmer {
  0%,
  100% {
    filter: saturate(1.4) brightness(1.2) drop-shadow(0 0 1rem rgba(220, 230, 255, 0.85));
  }

  50% {
    filter: saturate(1.75) brightness(1.45) drop-shadow(0 0 1.55rem rgba(255, 255, 255, 1));
  }
}

.floating-number {
  position: fixed;
  pointer-events: none;
  font-weight: 700;
  font-size: 1.2rem;
  color: #4ade80;
  text-shadow: 0 0 4px rgba(74, 222, 128, 0.8);
  animation: float-up 1s ease-out forwards;
  z-index: 30;
}

.flash-button,
.flash-button-white {
  animation: menu-white-flash 1s ease-in-out infinite;
}
</style>
