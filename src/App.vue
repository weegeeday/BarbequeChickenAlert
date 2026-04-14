<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import popupImage from './assets/BCA.svg'
import soundFile from './assets/sound.mp3'
import sound2File from './assets/sound2.mp3'

const chickenImageModules = import.meta.glob('./assets/chicken/**/*.{png,jpg,jpeg,webp,avif,gif}', {
  eager: true,
  import: 'default',
})

const linearFrictionPerSecond = 0.08
const angularFrictionPerSecond = 0.55
const saveStorageKey = 'barbequeChickenAlert.save.v1'
const repoUrl = 'https://github.com/weegeeday/BarbequeChickenAlert'
const bugReportUrl = `${repoUrl}/issues/new`

const isPopupVisible = ref(false)
const popupCycleKey = ref(0)
const viewportWidth = ref(window.innerWidth)
const viewportHeight = ref(window.innerHeight)
const viewportScale = ref(window.visualViewport?.scale ?? 1)
const chickens = ref([])
const areCollisionsEnabled = ref(true)
const isPerformanceMode = ref(false)
const isMenuOpen = ref(false)
const chickensPerPopup = ref(1)
const upgradeLevel = ref(1)
const hasAutoPopupClickUpgrade = ref(false)
const isAutoClickerEnabled = ref(true)
const isSoundEnabled = ref(true)
const rebirthCount = ref(0)
const hasChickenBreastUnlock = ref(false)
const popupSpeedUpgradeLevel = ref(0)
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

let popupTimerId = null
let animationFrameId = null
let previousTimestamp = 0
let chickenIdSeed = 0
let activeDragId = null
let lowFpsDurationMs = 0
let stableFpsDurationMs = 0
let smoothedFps = 60
let cookProductionCarry = 0
let totalChickenEarned = 0
let sessionStartTimestamp = performance.now()

const popupAudio = new Audio(soundFile)
const chickenAudio = new Audio(sound2File)
const chickenImageEntries = Object.entries(chickenImageModules)
const wingChickenImages = chickenImageEntries
  .filter(([path]) => path.toLowerCase().includes('/wings/'))
  .map(([, src]) => src)
const chickenBreastImages = chickenImageEntries
  .filter(([path]) => path.toLowerCase().includes('/full/'))
  .map(([, src]) => src)
const chickenImages = [...wingChickenImages, ...chickenBreastImages]

popupAudio.preload = 'auto'
chickenAudio.preload = 'auto'

const syncSoundPreference = () => {
  const muted = !isSoundEnabled.value
  popupAudio.muted = muted
  chickenAudio.muted = muted
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
const cookBaseCost = 125
const getPopupSpeedUpgradeCost = (level) => {
  return Math.max(cookBaseCost, getUpgradeCost(level))
}
const getCookCost = (level) => {
  return Math.max(cookBaseCost, getUpgradeCost(level))
}
const rebirthMultiplier = computed(() => 1 + 0.5 * rebirthCount.value)
const canAffordChickenBreastUnlock = computed(() => {
  return !hasChickenBreastUnlock.value && chickenCount.value >= chickenBreastUnlockCost
})
const nextPopupSpeedCost = computed(() => getPopupSpeedUpgradeCost(upgradeLevel.value))
const canAffordPopupSpeedUpgrade = computed(() => chickenCount.value >= nextPopupSpeedCost.value)
const nextCookCost = computed(() => getCookCost(upgradeLevel.value))
const canAffordCook = computed(() => chickenCount.value >= nextCookCost.value)
const cookOutputPerSecond = computed(() => cookCount.value * (1 + rebirthCount.value))
const currentPopupIntervalMs = computed(() => {
  return Math.max(1000, 5000 - popupSpeedUpgradeLevel.value * 50)
})
const isCompactHud = computed(() => viewportWidth.value < 520)
const isPopupSpeedFullyUpgraded = computed(() => currentPopupIntervalMs.value <= 1000)
const getRebirthLevelRequirement = () => {
  return 45 + rebirthCount.value * 10
}
const nextRebirthLevelRequirement = computed(() => getRebirthLevelRequirement())
const canAffordRebirth = computed(() => upgradeLevel.value >= nextRebirthLevelRequirement.value)
const canAffordAutoPopupClickUpgrade = computed(() => {
  return !hasAutoPopupClickUpgrade.value && chickenCount.value >= autoPopupClickUpgradeCost
})
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

const normalizePositiveInteger = (value, fallback = 1, minimum = 1, maximum = 1000000) => {
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
  if (!isSoundEnabled.value) {
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
  if (!isSoundEnabled.value) {
    return
  }

  try {
    chickenAudio.currentTime = 0
    await chickenAudio.play()
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

const spawnPopup = () => {
  isPopupVisible.value = true
  popupCycleKey.value += 1
  void playPopupAudio()

  if (hasAutoPopupClickUpgrade.value && isAutoClickerEnabled.value) {
    window.setTimeout(() => {
      handlePopupClick()
    }, 0)
  }
}

const createChicken = () => {
  const size = getChickenSize()
  const maxX = Math.max(0, viewportWidth.value - size)
  const maxY = Math.max(0, viewportHeight.value - size)

  const validImages = hasChickenBreastUnlock.value
    ? chickenImages
    : (wingChickenImages.length > 0 ? wingChickenImages : chickenImages)

  return {
    id: ++chickenIdSeed,
    src: validImages[Math.floor(Math.random() * validImages.length)],
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

  const requested = chickensPerPopup.value

  if (requested > 0) {
    addChicken(requested)
  }

  isPopupVisible.value = false

  if (popupTimerId !== null) {
    window.clearTimeout(popupTimerId)
  }

  popupTimerId = window.setTimeout(spawnPopup, currentPopupIntervalMs.value)
}

const toggleMenu = () => {
  isMenuOpen.value = !isMenuOpen.value
}

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

const rebirth = () => {
  if (!canAffordRebirth.value) {
    return
  }

  rebirthCount.value += 1
  totalChickenCount.value = 0
  chickensPerPopup.value = 1
  upgradeLevel.value = 1
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
  popupSpeedUpgradeLevel.value += 1
  syncRenderedChickens()
}

const hireCook = () => {
  if (!canAffordCook.value) {
    return
  }

  totalChickenCount.value -= nextCookCost.value
  cookCount.value += 1
  syncRenderedChickens()
}

const applyChickenCap = () => {
  chickenCap.value = normalizeCapValue(chickenCap.value)
  enforceChickenCap()
  syncRenderedChickens()
}

const toggleUnlimitedCap = () => {
  const desiredValue = isUnlimitedCap.value

  activeConfirmPrompt.value = {
    title: 'Unlimited cap',
    message: 'Are you sure you want to toggle unlimited cap? This can cause lag or crash your browser.',
    confirmLabel: 'Yes, toggle it',
    cancelLabel: 'Cancel',
    onConfirm: () => {
      if (!desiredValue) {
        applyChickenCap()
      }
    },
    onCancel: () => {
      isUnlimitedCap.value = !desiredValue
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
    hasChickenBreastUnlock: hasChickenBreastUnlock.value,
    popupSpeedUpgradeLevel: popupSpeedUpgradeLevel.value,
    cookCount: cookCount.value,
    chickenCap: chickenCap.value,
    areCollisionsEnabled: areCollisionsEnabled.value,
    isPerformanceMode: isPerformanceMode.value,
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

const removeSavedProgress = () => {
  try {
    window.localStorage.removeItem(saveStorageKey)
  } catch (error) {
    void error
  }
}

const openDeleteSavePrompt = () => {
  activeConfirmPrompt.value = {
    title: 'Delete save',
    message: 'Delete saved progress from this device?',
    confirmLabel: 'Delete save',
    cancelLabel: 'Cancel',
    onConfirm: () => {
      removeSavedProgress()
      autosaveStatus.value = sessionSavingEnabled.value ? 'Autosave: On' : 'Autosave: Off (session)'
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
  hasChickenBreastUnlock.value = Boolean(savedState.hasChickenBreastUnlock)
  popupSpeedUpgradeLevel.value = normalizePositiveInteger(savedState.popupSpeedUpgradeLevel, 0, 0)
  cookCount.value = normalizePositiveInteger(savedState.cookCount, 0, 0)
  chickenCap.value = normalizeCapValue(savedState.chickenCap)
  areCollisionsEnabled.value = savedState.areCollisionsEnabled !== false
  isPerformanceMode.value = Boolean(savedState.isPerformanceMode)

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

const handleDeleteSavedProgress = () => {
  removeSavedProgress()
  sessionSavingEnabled.value = true
  autosaveStatus.value = 'Autosave: On'
  showSavePrompt.value = false
  pendingSavedState.value = null
}

const handleKeepWithoutLoad = () => {
  sessionSavingEnabled.value = false
  autosaveStatus.value = 'Autosave: Off (session)'
  showSavePrompt.value = false
  pendingSavedState.value = null
}

const closeConfirmPrompt = () => {
  activeConfirmPrompt.value = null
}

const confirmActivePrompt = () => {
  const prompt = activeConfirmPrompt.value
  if (!prompt) {
    return
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

const handleChickenClick = async (chicken) => {
  if (chicken.movedWhileDragging) {
    chicken.movedWhileDragging = false
    return
  }

  await playChickenAudio()
}

const shakeChickens = () => {
  chickens.value.forEach((chicken) => {
    if (chicken.isDragging) {
      return
    }

    chicken.velocityX = randomBetween(-420, 420)
    chicken.velocityY = randomBetween(-420, 420)
    chicken.angularVelocity = randomBetween(-600, 600)
  })
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

  const cookRate = cookOutputPerSecond.value
  if (cookRate > 0) {
    const produced = cookRate * deltaSeconds + cookProductionCarry
    const producedWhole = Math.floor(produced)
    cookProductionCarry = produced - producedWhole

    if (producedWhole > 0) {
      addChicken(producedWhole)
    }
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
  document.title = 'Barbeque Chicken Alert'
  updateViewport()
  sessionStartTimestamp = performance.now()
  totalChickenEarned = 0
  cookProductionCarry = 0
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

  animationFrameId = window.requestAnimationFrame(animateChickens)
})

onUnmounted(() => {
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

  if (window.visualViewport) {
    window.visualViewport.removeEventListener('resize', updateViewport)
    window.visualViewport.removeEventListener('scroll', updateViewport)
  }

  popupAudio.pause()
  popupAudio.currentTime = 0
  chickenAudio.pause()
  chickenAudio.currentTime = 0
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
    popupSpeedUpgradeLevel,
    cookCount,
    chickenCap,
    areCollisionsEnabled,
    isPerformanceMode,
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
  <div class="black-screen">
    <div v-if="!isCompactHud" class="fps-hud">FPS: {{ fpsCounter }} | Avg CPS: {{ averageCps.toFixed(1) }}</div>

    <div class="hud">
      <button class="menu-button" type="button" aria-label="Open menu" @click="toggleMenu">☰</button>
      <button class="shake-button" type="button" @click="shakeChickens">Shake</button>
      <div class="counter">Chicken: {{ chickenCount }}</div>
    </div>

    <div v-if="!isCompactHud" class="save-status">{{ autosaveStatus }}</div>

    <div v-if="isMenuOpen" class="menu-panel">
      <div class="menu-title">Upgrades</div>
      <div class="menu-label">Current level: {{ upgradeLevel }}</div>

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
          type="button"
          class="upgrade-button"
          :disabled="!canAffordCook"
          @click="hireCook"
        >
          Hire Cook ({{ cookCount }}) — {{ cookOutputPerSecond.toFixed(1) }} CPS — Cost: {{ nextCookCost }}
        </button>

        <button
          v-if="!isPopupSpeedFullyUpgraded"
          type="button"
          class="upgrade-button"
          :disabled="!canAffordPopupSpeedUpgrade"
          @click="upgradePopupSpeed"
        >
          Faster Popups ({{ (currentPopupIntervalMs / 1000).toFixed(2) }}s) — Cost: {{ nextPopupSpeedCost }}
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
            v-model="isUnlimitedCap"
            type="checkbox"
            :disabled="isPerformanceMode"
            @change="toggleUnlimitedCap"
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
              @click="rebirth"
            >
              Rebirth at Level {{ nextRebirthLevelRequirement }} (+0.5× gain)
            </button>
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

    <div v-if="isPerformanceMode" class="performance-notice">
      Collision physics disabled for performance.
    </div>

    <div v-if="showSavePrompt" class="save-overlay">
      <div class="save-dialog">
        <div class="save-title">Saved progress found</div>
        <div class="save-text">Choose how to start this session:</div>
        <button type="button" class="save-button" @click="handleLoadSavedProgress">Load save</button>
        <button type="button" class="save-button" @click="handleDeleteSavedProgress">Delete save</button>
        <button type="button" class="save-button" @click="handleKeepWithoutLoad">
          Keep save, do not load (disable saving this session)
        </button>
      </div>
    </div>

    <div v-if="activeConfirmPrompt" class="save-overlay">
      <div class="save-dialog">
        <div class="save-title">{{ activeConfirmPrompt.title }}</div>
        <div class="save-text">{{ activeConfirmPrompt.message }}</div>
        <div class="prompt-actions">
          <button type="button" class="save-button" @click="confirmActivePrompt">
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
      }"
      @pointerdown="handleChickenPointerDown(chicken, $event)"
      @click="handleChickenClick(chicken)"
      draggable="false"
    />

    <img
      v-if="isPopupVisible"
      :key="popupCycleKey"
      class="popup-image"
      :src="popupImage"
      alt="BCA popup"
      @click="handlePopupClick"
    />
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
.shake-button,
.counter {
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.92);
  color: #f1f1f1;
  border-radius: 999px;
  padding: 0.35rem 0.75rem;
  font-size: 0.9rem;
}

.menu-button,
.shake-button {
  cursor: pointer;
}

.menu-panel {
  position: fixed;
  top: 3.8rem;
  right: 1rem;
  z-index: 55;
  width: min(320px, 88vw);
  border: 1px solid #2c2c2c;
  background: rgba(15, 15, 15, 0.96);
  border-radius: 0.9rem;
  padding: 0.75rem;
  display: grid;
  gap: 0.55rem;
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
}

.menu-input {
  border: 1px solid #4a4a4a;
  background: rgba(20, 20, 20, 0.95);
  color: #f7f7f7;
  border-radius: 0.6rem;
  padding: 0.45rem 0.55rem;
  font-size: 0.82rem;
}

.menu-toggle {
  display: flex;
  align-items: center;
  gap: 0.45rem;
  color: #d8d8d8;
  font-size: 0.8rem;
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
  z-index: 100;
  background: rgba(0, 0, 0, 0.66);
  display: grid;
  place-items: center;
  padding: 1rem;
}

.save-dialog {
  width: min(430px, 95vw);
  border: 1px solid #2f2f2f;
  border-radius: 1rem;
  background: rgba(15, 15, 15, 0.98);
  padding: 1rem;
  display: grid;
  gap: 0.6rem;
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

@media (max-width: 767px) {
  .popup-image {
    left: 50%;
    right: auto;
    transform: translateX(-50%);
    width: clamp(180px, 72vw, 420px);
    max-width: 86vw;
  }

  .hud {
    top: 0.75rem;
    right: 0.75rem;
  }

  .fps-hud {
    top: 0.75rem;
    left: 0.75rem;
  }

  .menu-panel {
    top: 3.45rem;
    right: 0.75rem;
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
</style>
