const LINEAR_SPEED_CAP = 320
const ANGULAR_SPEED_CAP = 540
const BOUNCE_FACTOR = 0.95
const RADIUS_SCALE = 0.45
const RESTING_NORMAL_VELOCITY = -8
const MIN_NORMAL_MAGNITUDE = 0.0001

const capSpeed = (value, limit) => {
  if (value > limit) {
    return limit
  }

  if (value < -limit) {
    return -limit
  }

  return value
}

const createFallbackNormal = (firstId, secondId) => {
  const mixed = (firstId * 73856093) ^ (secondId * 19349663)
  const normalized = ((mixed % 6283) + 6283) % 6283
  const angle = normalized / 1000

  return {
    x: Math.cos(angle),
    y: Math.sin(angle),
  }
}

const createDeltaBucket = (id) => ({
  id,
  deltaX: 0,
  deltaY: 0,
  deltaVelocityX: 0,
  deltaVelocityY: 0,
  deltaAngularVelocity: 0,
})

const resolveCollisionDeltas = (snapshot) => {
  const chickens = snapshot.map((chicken) => ({ ...chicken }))
  const originalById = new Map(snapshot.map((chicken) => [chicken.id, chicken]))
  const deltasById = new Map()

  const ensureDelta = (id) => {
    if (!deltasById.has(id)) {
      deltasById.set(id, createDeltaBucket(id))
    }

    return deltasById.get(id)
  }

  for (let index = 0; index < chickens.length; index += 1) {
    const chickenA = chickens[index]

    if (chickenA.isDragging) {
      continue
    }

    for (let offset = index + 1; offset < chickens.length; offset += 1) {
      const chickenB = chickens[offset]

      if (chickenB.isDragging) {
        continue
      }

      const firstCenterX = chickenA.x + chickenA.size / 2
      const firstCenterY = chickenA.y + chickenA.size / 2
      const secondCenterX = chickenB.x + chickenB.size / 2
      const secondCenterY = chickenB.y + chickenB.size / 2

      let deltaX = secondCenterX - firstCenterX
      let deltaY = secondCenterY - firstCenterY
      let distanceSquared = deltaX * deltaX + deltaY * deltaY

      if (distanceSquared < MIN_NORMAL_MAGNITUDE) {
        const fallbackNormal = createFallbackNormal(chickenA.id, chickenB.id)
        deltaX = fallbackNormal.x
        deltaY = fallbackNormal.y
        distanceSquared = deltaX * deltaX + deltaY * deltaY
      }

      const firstRadius = chickenA.size * RADIUS_SCALE
      const secondRadius = chickenB.size * RADIUS_SCALE
      const minimumDistance = firstRadius + secondRadius

      if (distanceSquared >= minimumDistance * minimumDistance) {
        continue
      }

      const distance = Math.sqrt(distanceSquared)
      const normalX = deltaX / distance
      const normalY = deltaY / distance
      const overlap = minimumDistance - distance
      const correction = overlap / 2

      chickenA.x -= normalX * correction
      chickenA.y -= normalY * correction
      chickenB.x += normalX * correction
      chickenB.y += normalY * correction

      const relativeVelocityX = chickenB.velocityX - chickenA.velocityX
      const relativeVelocityY = chickenB.velocityY - chickenA.velocityY
      const velocityAlongNormal = relativeVelocityX * normalX + relativeVelocityY * normalY

      if (velocityAlongNormal >= RESTING_NORMAL_VELOCITY) {
        continue
      }

      const impulse = (-(1 + BOUNCE_FACTOR) * velocityAlongNormal) / 2
      const impulseX = impulse * normalX
      const impulseY = impulse * normalY

      chickenA.velocityX = capSpeed(chickenA.velocityX - impulseX, LINEAR_SPEED_CAP)
      chickenA.velocityY = capSpeed(chickenA.velocityY - impulseY, LINEAR_SPEED_CAP)
      chickenB.velocityX = capSpeed(chickenB.velocityX + impulseX, LINEAR_SPEED_CAP)
      chickenB.velocityY = capSpeed(chickenB.velocityY + impulseY, LINEAR_SPEED_CAP)
      chickenA.angularVelocity = capSpeed(chickenA.angularVelocity - impulse * 0.08, ANGULAR_SPEED_CAP)
      chickenB.angularVelocity = capSpeed(chickenB.angularVelocity + impulse * 0.08, ANGULAR_SPEED_CAP)
    }
  }

  chickens.forEach((chicken) => {
    const original = originalById.get(chicken.id)

    if (!original) {
      return
    }

    const deltaX = chicken.x - original.x
    const deltaY = chicken.y - original.y
    const deltaVelocityX = chicken.velocityX - original.velocityX
    const deltaVelocityY = chicken.velocityY - original.velocityY
    const deltaAngularVelocity = chicken.angularVelocity - original.angularVelocity

    if (deltaX === 0 && deltaY === 0 && deltaVelocityX === 0 && deltaVelocityY === 0 && deltaAngularVelocity === 0) {
      return
    }

    const delta = ensureDelta(chicken.id)
    delta.deltaX += deltaX
    delta.deltaY += deltaY
    delta.deltaVelocityX += deltaVelocityX
    delta.deltaVelocityY += deltaVelocityY
    delta.deltaAngularVelocity += deltaAngularVelocity
  })

  return Array.from(deltasById.values())
}

self.onmessage = (event) => {
  const { requestId, chickens } = event.data || {}

  if (!Array.isArray(chickens)) {
    self.postMessage({ requestId, deltas: [] })
    return
  }

  const deltas = resolveCollisionDeltas(chickens)
  self.postMessage({ requestId, deltas })
}
