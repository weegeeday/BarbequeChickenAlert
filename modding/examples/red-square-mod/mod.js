/* global BCA_API */
/**
 * Red Square Mod
 * Overrides all spawned chicken graphics with a Red Square image
 * and adds a simple upgrade named "Red" that generates 1 chicken per second.
 */

if (typeof BCA_API !== 'undefined') {
  const RED_SQUARE_URL = 'https://img.magnific.com/premium-photo/red-square-background-simple-design-backdrop-banners-posters-various-design-works_7954-54551.jpg'

  // 1. Override active chicken graphics for spawned items
  BCA_API.overrideChickenImages([RED_SQUARE_URL])

  // 2. Register skin
  BCA_API.registerSkin({
    id: 'red-square-skin',
    name: 'Red Square Skin',
    chickenImages: [RED_SQUARE_URL]
  })
  BCA_API.setSkin('red-square-skin')

  // 3. Add a simple upgrade named "Red"
  BCA_API.addUpgrade({
    id: 'red-upgrade',
    name: 'Red',
    description: 'Generates 1 chicken per second.',
    cost: 10,
    costMultiplier: 1.5,
    icon: '[R]',
    onBuy: () => {
      // Immediate +1 chicken reward on purchase
      BCA_API.addChickens(1)
    }
  })

  // 4. Hook into game tick loop to generate 1 chicken/sec per upgrade level
  let accumulatedSeconds = 0

  BCA_API.onTick(({ deltaSeconds }) => {
    const upgrade = BCA_API.customUpgrades.get('red-upgrade')
    if (upgrade && upgrade.level > 0) {
      accumulatedSeconds += deltaSeconds
      if (accumulatedSeconds >= 1) {
        const secondsToProcess = Math.floor(accumulatedSeconds)
        accumulatedSeconds -= secondsToProcess
        BCA_API.addChickens(secondsToProcess * upgrade.level)
      }
    }
  })
}
