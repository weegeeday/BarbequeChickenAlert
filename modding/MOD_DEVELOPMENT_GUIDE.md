# 🧩 Barbeque Chicken Alert - Modding Guide

Welcome to the modding system for **Barbeque Chicken Alert**! Users can easily import mods via GitHub repository links, direct raw folder URLs, or by uploading `.zip` archives.

---

## 🚀 1. Mod Structure & Manifest (`mod.json`)

Every mod must contain a `mod.json` (or `manifest.json`) file at its root folder:

```json
{
  "id": "red-square-mod",
  "title": "Red Square Mod",
  "version": "1.0.0",
  "author": "Weegeeday",
  "description": "Replaces all chicken graphics with a red square.",
  "image": "https://img.magnific.com/premium-photo/red-square-background-simple-design-backdrop-banners-posters-various-design-works_7954-54551.jpg",
  "js": ["mod.js"],
  "css": [],
  "html": [],
  "externalLibraries": []
}
```

### Manifest Fields:

- `id`: Unique string slug (alphanumeric, hyphens).
- `title`: Display name of your mod.
- `version`: Version string (e.g. `1.0.0`).
- `author`: Author or creator name.
- `description`: Short description of what the mod does.
- `image`: URL or relative path to a thumbnail image/icon.
- `js`: String or Array of JS files to execute.
- `css`: String or Array of CSS files to inject.
- `html`: String or Array of HTML templates to inject into `#mod-container`.
- `externalLibraries`: Array of external CDN script URLs or library names if your mod relies on third-party libraries. (Triggers an informational note on installation).

---

## 🛠️ 2. JavaScript Modding API (`window.BCA_API`)

Your JS code runs with full access to `BCA_API` (also aliased as `window.BCAMods`).

### Game State Getters & Actions:

```js
// Get live state snapshot
const state = BCA_API.getGameState()
console.log(state.totalChickenCount, state.cps, state.rebirthCount)

// Add chickens to total
BCA_API.addChickens(1000)

// Set exact chicken count
BCA_API.setChickens(50000)
```

### Event Hooks:

```js
// On initial load
BCA_API.onInit((state) => { ... })

// On physics/game frame tick
BCA_API.onTick(({ deltaMs, deltaSeconds, timestamp }) => { ... })

// On chicken/item spawn
BCA_API.onChickenSpawn((chicken) => { ... })

// On popup clicked
BCA_API.onPopupClick(({ tier, totalAdded }) => { ... })

// On rebirth
BCA_API.onRebirth(({ rebirthCount }) => { ... })
```

### Custom Upgrades API:

```js
BCA_API.addUpgrade({
  id: 'red-upgrade',
  name: 'Red',
  description: 'Generates 1 chicken per second.',
  cost: 10,
  costMultiplier: 1.5,
  icon: '[R]',
  onBuy: (level) => {
    BCA_API.addChickens(1)
  },
})
```

### Custom HUD Buttons:

```js
BCA_API.addHUDButton({
  id: 'party-btn',
  label: 'Party Time',
  icon: '[P]',
  onClick: () => {
    document.body.classList.toggle('party-mode')
  },
})
```

### Custom Graphics / Skins API:

```js
// Override active chicken sprites
BCA_API.overrideChickenImages([
  'https://img.magnific.com/premium-photo/red-square-background-simple-design-backdrop-banners-posters-various-design-works_7954-54551.jpg',
])

// Register custom skin package
BCA_API.registerSkin({
  id: 'red-square-skin',
  name: 'Red Square Skin',
  chickenImages: [
    'https://img.magnific.com/premium-photo/red-square-background-simple-design-backdrop-banners-posters-various-design-works_7954-54551.jpg',
  ],
})
BCA_API.setSkin('red-square-skin')
```

---

## 📥 3. How Users Install Mods

Users can install mods using **any of the following methods**:

1. **GitHub Repository Link**:
   - Paste a repository URL (e.g. `https://github.com/username/my-bca-mod`).
2. **Direct `.zip` Link**:
   - Paste a direct URL ending in `.zip` or a GitHub release archive link.
3. **Local `.zip` File Upload**:
   - Click **Upload .ZIP** in the Mod Manager and select any `.zip` archive containing a `mod.json` file.

After fetching or uploading, click **➕ Install & Add Mod** and then **Reload Game Now** to activate your mod!

---

## ⚡ 4. Testing Mods Locally

You can test mods locally by zipping your mod folder (`mod.json`, `mod.js`, `mod.css`, `mod.html`, images) and clicking **Upload .ZIP** in the Mod Manager UI.
Sample files are provided in [`modding/examples/red-square-mod/`](file:///c:/Users/simon/BarbequeChickenAlert/modding/examples/red-square-mod/).
