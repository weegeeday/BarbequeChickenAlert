import JSZip from 'jszip'
import modApi from './modApi.js'

const MODS_STORAGE_KEY = 'barbequeChickenAlert.mods.v1'

class ModManager {
  constructor() {
    this.installedMods = []
    this.loadedMods = new Set()
    this.init()
  }

  init() {
    this.installedMods = this.loadSavedMods()
  }

  loadSavedMods() {
    try {
      const raw = localStorage.getItem(MODS_STORAGE_KEY)
      if (raw) {
        return JSON.parse(raw)
      }
    } catch (e) {
      console.error('[ModManager] Failed to read installed mods:', e)
    }
    return []
  }

  saveMods(mods) {
    try {
      this.installedMods = mods
      localStorage.setItem(MODS_STORAGE_KEY, JSON.stringify(mods))
    } catch (e) {
      console.error('[ModManager] Failed to save mods:', e)
    }
  }

  /**
   * Parse user input URL (GitHub repository, tree URL, raw URL, or raw folder)
   * into a standardized root URL for fetching mod files.
   */
  resolveModBaseUrl(inputUrl) {
    let url = inputUrl.trim()
    if (!url) throw new Error('URL cannot be empty')

    // Standardize URL schema
    if (!url.startsWith('http://') && !url.startsWith('https://')) {
      url = 'https://' + url
    }

    // Handle GitHub repository URLs
    // Example: https://github.com/username/repository
    // Example: https://github.com/username/repository/tree/main
    // Example: https://github.com/username/repository/blob/main/mod.json
    const githubRegex = /^https?:\/\/github\.com\/([^/]+)\/([^/]+)(?:\/(?:tree|blob)\/([^/]+)(?:\/(.*))?)?$/i
    const match = url.match(githubRegex)

    if (match) {
      const user = match[1]
      const repo = match[2].replace(/\.git$/, '')
      let branch = match[3] || 'main'
      let subpath = match[4] || ''

      // If user linked directly to mod.json, strip filename from subpath
      if (subpath.endsWith('mod.json') || subpath.endsWith('manifest.json')) {
        subpath = subpath.substring(0, subpath.lastIndexOf('/'))
      }

      let rawBase = `https://raw.githubusercontent.com/${user}/${repo}/${branch}/`
      if (subpath) {
        rawBase += subpath.endsWith('/') ? subpath : subpath + '/'
      }
      return { baseUrl: rawBase, originalUrl: inputUrl, isGithub: true }
    }

    // Handle raw githubusercontent URLs directly
    // Example: https://raw.githubusercontent.com/user/repo/main/
    if (url.includes('raw.githubusercontent.com')) {
      if (url.endsWith('mod.json') || url.endsWith('manifest.json')) {
        url = url.substring(0, url.lastIndexOf('/') + 1)
      } else if (!url.endsWith('/')) {
        url += '/'
      }
      return { baseUrl: url, originalUrl: inputUrl, isGithub: true }
    }

    // Direct HTTP(S) folder or file URL
    if (url.endsWith('mod.json') || url.endsWith('manifest.json')) {
      url = url.substring(0, url.lastIndexOf('/') + 1)
    } else if (!url.endsWith('/')) {
      url += '/'
    }

    return { baseUrl: url, originalUrl: inputUrl, isGithub: false }
  }

  /**
   * Parse a mod directly from an ArrayBuffer of a .zip file.
   */
  async parseModFromZipBuffer(arrayBuffer, sourceName = 'Uploaded Zip Archive') {
    const zip = await JSZip.loadAsync(arrayBuffer)

    let manifestEntryKey = null
    let shortestDepth = Infinity

    zip.forEach((relativePath, entry) => {
      if (!entry.dir && (relativePath.endsWith('mod.json') || relativePath.endsWith('manifest.json'))) {
        const depth = relativePath.split('/').length
        if (depth < shortestDepth) {
          shortestDepth = depth
          manifestEntryKey = relativePath
        }
      }
    })

    if (!manifestEntryKey) {
      throw new Error('Could not find mod.json or manifest.json in the .zip archive')
    }

    const manifestText = await zip.file(manifestEntryKey).async('text')
    const manifest = JSON.parse(manifestText)

    if (!manifest.id || (!manifest.title && !manifest.name)) {
      throw new Error('mod.json inside .zip must contain an "id" and a "title" (or "name")')
    }

    const zipSubpath = manifestEntryKey.includes('/')
      ? manifestEntryKey.substring(0, manifestEntryKey.lastIndexOf('/') + 1)
      : ''

    const id = String(manifest.id).trim().toLowerCase().replace(/[^a-z0-9_-]/g, '')
    const title = manifest.title || manifest.name
    const version = manifest.version || '1.0.0'
    const author = manifest.author || 'Unknown'
    const description = manifest.description || 'No description provided.'

    const jsFiles = Array.isArray(manifest.js) ? manifest.js : (manifest.js ? [manifest.js] : [])
    const cssFiles = Array.isArray(manifest.css) ? manifest.css : (manifest.css ? [manifest.css] : [])
    const htmlFiles = Array.isArray(manifest.html) ? manifest.html : (manifest.html ? [manifest.html] : [])

    const fetchedJsContents = []
    const externalLibraries = []

    if (Array.isArray(manifest.externalLibraries)) {
      externalLibraries.push(...manifest.externalLibraries)
    }
    if (Array.isArray(manifest.dependencies)) {
      externalLibraries.push(...manifest.dependencies)
    }

    for (const jsRel of jsFiles) {
      const fullZipPath = zipSubpath + jsRel.replace(/^\//, '')
      const fileEntry = zip.file(fullZipPath)
      if (fileEntry) {
        const code = await fileEntry.async('text')
        fetchedJsContents.push({ url: jsRel, code })

        const extMatches = code.match(/https?:\/\/[^\s'"`()<>]+/g)
        if (extMatches) {
          extMatches.forEach(matchUrl => {
            const lower = matchUrl.toLowerCase()
            const isAsset = lower.match(/\.(png|jpg|jpeg|gif|svg|webp|avif|mp3|wav|ogg|css|html|json)($|\?|#)/)
            const isScriptLib = lower.match(/\.(m?js)($|\?|#)/) || lower.includes('cdn.') || lower.includes('unpkg.com') || lower.includes('cdnjs.') || lower.includes('jsdelivr.net')
            if (!isAsset && isScriptLib && !externalLibraries.includes(matchUrl)) {
              externalLibraries.push(matchUrl)
            }
          })
        }
      } else {
        console.warn(`[ModManager] JS file ${jsRel} not found in zip archive`)
      }
    }

    const fetchedCssContents = []
    for (const cssRel of cssFiles) {
      const fullZipPath = zipSubpath + cssRel.replace(/^\//, '')
      const fileEntry = zip.file(fullZipPath)
      if (fileEntry) {
        const cssCode = await fileEntry.async('text')
        fetchedCssContents.push({ url: cssRel, code: cssCode })
      }
    }

    const fetchedHtmlContents = []
    for (const htmlRel of htmlFiles) {
      const fullZipPath = zipSubpath + htmlRel.replace(/^\//, '')
      const fileEntry = zip.file(fullZipPath)
      if (fileEntry) {
        const htmlCode = await fileEntry.async('text')
        fetchedHtmlContents.push({ url: htmlRel, code: htmlCode })
      }
    }

    let image = manifest.image || manifest.icon
    if (image && !image.startsWith('http://') && !image.startsWith('https://') && !image.startsWith('data:')) {
      const imgZipPath = zipSubpath + image.replace(/^\//, '')
      const imgEntry = zip.file(imgZipPath)
      if (imgEntry) {
        const base64 = await imgEntry.async('base64')
        const ext = image.split('.').pop().toLowerCase()
        const mime = ext === 'svg' ? 'image/svg+xml' : (ext === 'jpg' || ext === 'jpeg' ? 'image/jpeg' : 'image/png')
        image = `data:${mime};base64,${base64}`
      }
    }
    if (!image) {
      image = 'https://raw.githubusercontent.com/weegeeday/BarbequeChickenAlert/main/src/assets/BCA.svg'
    }

    const tiers = ['ZIP']
    if (jsFiles.length > 0) tiers.push('JS')
    if (cssFiles.length > 0) tiers.push('CSS')
    if (htmlFiles.length > 0) tiers.push('HTML')
    if (manifest.skin || manifest.type === 'skin') tiers.push('Skin')

    return {
      id,
      title,
      version,
      author,
      description,
      image,
      baseUrl: 'zip://' + id,
      sourceUrl: sourceName,
      tiers,
      jsFiles: [],
      cssFiles: [],
      htmlFiles: [],
      jsContents: fetchedJsContents,
      cssContents: fetchedCssContents,
      htmlContents: fetchedHtmlContents,
      externalLibraries: Array.from(new Set(externalLibraries)),
      manifest,
      installedAt: Date.now(),
      enabled: true,
      isZip: true,
    }
  }

  /**
   * Parse a mod from an uploaded File object (.zip).
   */
  async parseModFromZipFile(file) {
    if (!file || !file.name.toLowerCase().endsWith('.zip')) {
      throw new Error('Please select a valid .zip file')
    }
    const buffer = await file.arrayBuffer()
    return this.parseModFromZipBuffer(buffer, file.name)
  }

  /**
   * Fetch mod manifest (`mod.json` or `manifest.json`) from URL or Zip URL.
   */
  async fetchModPreview(inputUrl) {
    const trimmed = inputUrl.trim()
    if (trimmed.toLowerCase().endsWith('.zip') || trimmed.includes('/archive/refs/') || trimmed.includes('/releases/download/')) {
      try {
        const res = await fetch(trimmed)
        if (res.ok) {
          const buffer = await res.arrayBuffer()
          return this.parseModFromZipBuffer(buffer, trimmed)
        }
      } catch {
        // Fall back to standard URL handling
      }
    }

    const { baseUrl, originalUrl } = this.resolveModBaseUrl(inputUrl)

    let manifestRes = null
    let manifestUrl = baseUrl + 'mod.json'

    try {
      manifestRes = await fetch(manifestUrl)
      if (!manifestRes.ok) {
        manifestUrl = baseUrl + 'manifest.json'
        manifestRes = await fetch(manifestUrl)
      }
    } catch {
      if (originalUrl.includes('github.com') && !originalUrl.includes('/tree/')) {
        const masterBase = baseUrl.replace('/main/', '/master/')
        manifestUrl = masterBase + 'mod.json'
        try {
          manifestRes = await fetch(manifestUrl)
          if (!manifestRes.ok) {
            manifestUrl = masterBase + 'manifest.json'
            manifestRes = await fetch(manifestUrl)
          }
        } catch {
          // ignore
        }
      }
    }

    if (!manifestRes || !manifestRes.ok) {
      throw new Error(`Could not find mod.json or manifest.json at ${baseUrl}`)
    }

    const manifest = await manifestRes.json()

    if (!manifest.id || !manifest.title && !manifest.name) {
      throw new Error('mod.json must contain an "id" and a "title" (or "name")')
    }

    const id = String(manifest.id).trim().toLowerCase().replace(/[^a-z0-9_-]/g, '')
    const title = manifest.title || manifest.name
    const version = manifest.version || '1.0.0'
    const author = manifest.author || 'Unknown'
    const description = manifest.description || 'No description provided.'

    const resolveUrl = (path) => {
      if (!path) return null
      if (path.startsWith('http://') || path.startsWith('https://') || path.startsWith('data:')) {
        return path
      }
      return baseUrl + path.replace(/^\//, '')
    }

    let image = resolveUrl(manifest.image || manifest.icon)
    if (!image) {
      image = 'https://raw.githubusercontent.com/weegeeday/BarbequeChickenAlert/main/src/assets/BCA.svg'
    }

    const jsFiles = Array.isArray(manifest.js) ? manifest.js : (manifest.js ? [manifest.js] : [])
    const cssFiles = Array.isArray(manifest.css) ? manifest.css : (manifest.css ? [manifest.css] : [])
    const htmlFiles = Array.isArray(manifest.html) ? manifest.html : (manifest.html ? [manifest.html] : [])

    const resolvedJs = jsFiles.map(resolveUrl)
    const resolvedCss = cssFiles.map(resolveUrl)
    const resolvedHtml = htmlFiles.map(resolveUrl)

    const tiers = []
    if (resolvedJs.length > 0) tiers.push('JS')
    if (resolvedCss.length > 0) tiers.push('CSS')
    if (resolvedHtml.length > 0) tiers.push('HTML')
    if (manifest.skin || manifest.type === 'skin') tiers.push('Skin')
    if (tiers.length === 0) tiers.push('General')

    const externalLibraries = []
    if (Array.isArray(manifest.externalLibraries)) {
      externalLibraries.push(...manifest.externalLibraries)
    }
    if (Array.isArray(manifest.dependencies)) {
      externalLibraries.push(...manifest.dependencies)
    }

    let fetchedJsContents = []
    for (const jsUrl of resolvedJs) {
      try {
        const res = await fetch(jsUrl)
        if (res.ok) {
          const code = await res.text()
          fetchedJsContents.push({ url: jsUrl, code })

          const extMatches = code.match(/https?:\/\/[^\s'"`()<>]+/g)
          if (extMatches) {
            extMatches.forEach(matchUrl => {
              const lower = matchUrl.toLowerCase()
              const isAsset = lower.match(/\.(png|jpg|jpeg|gif|svg|webp|avif|mp3|wav|ogg|css|html|json)($|\?|#)/)
              const isScriptLib = lower.match(/\.(m?js)($|\?|#)/) || lower.includes('cdn.') || lower.includes('unpkg.com') || lower.includes('cdnjs.') || lower.includes('jsdelivr.net')
              if (!matchUrl.startsWith(baseUrl) && !isAsset && isScriptLib && !externalLibraries.includes(matchUrl)) {
                externalLibraries.push(matchUrl)
              }
            })
          }
        }
      } catch (e) {
        console.warn(`[ModManager] Could not pre-fetch JS file for analysis: ${jsUrl}`, e)
      }
    }

    return {
      id,
      title,
      version,
      author,
      description,
      image,
      baseUrl,
      sourceUrl: originalUrl,
      tiers,
      jsFiles: resolvedJs,
      cssFiles: resolvedCss,
      htmlFiles: resolvedHtml,
      jsContents: fetchedJsContents,
      externalLibraries: Array.from(new Set(externalLibraries)),
      manifest,
      installedAt: Date.now(),
      enabled: true,
    }
  }

  /**
   * Install a mod from preview metadata
   */
  async installMod(modPreview) {
    const existingIndex = this.installedMods.findIndex(m => m.id === modPreview.id)
    if (existingIndex >= 0) {
      this.installedMods[existingIndex] = modPreview
    } else {
      this.installedMods.push(modPreview)
    }
    this.saveMods(this.installedMods)
    return modPreview
  }

  /**
   * Enable or disable an installed mod
   */
  toggleMod(modId, enabled) {
    const mod = this.installedMods.find(m => m.id === modId)
    if (mod) {
      mod.enabled = enabled
      this.saveMods(this.installedMods)
    }
  }

  /**
   * Remove an installed mod
   */
  removeMod(modId) {
    this.installedMods = this.installedMods.filter(m => m.id !== modId)
    this.saveMods(this.installedMods)
  }

  /**
   * Apply all active enabled mods to the DOM / JS runtime.
   */
  async applyActiveMods() {
    const activeMods = this.installedMods.filter(m => m.enabled)

    for (const mod of activeMods) {
      if (this.loadedMods.has(mod.id)) continue
      this.loadedMods.add(mod.id)

      try {
        // 1. Inject CSS (from extracted contents or file URLs)
        if (mod.cssContents && mod.cssContents.length > 0) {
          for (const cssItem of mod.cssContents) {
            const style = document.createElement('style')
            style.textContent = cssItem.code
            style.dataset.modId = mod.id
            document.head.appendChild(style)
          }
        } else if (mod.cssFiles && mod.cssFiles.length > 0) {
          for (const cssUrl of mod.cssFiles) {
            const link = document.createElement('link')
            link.rel = 'stylesheet'
            link.href = cssUrl
            link.dataset.modId = mod.id
            document.head.appendChild(link)
          }
        }

        // 2. Inject HTML (from extracted contents or file URLs)
        if (mod.htmlContents && mod.htmlContents.length > 0) {
          for (const htmlItem of mod.htmlContents) {
            modApi.injectHTML(htmlItem.code, '#mod-container')
          }
        } else if (mod.htmlFiles && mod.htmlFiles.length > 0) {
          for (const htmlUrl of mod.htmlFiles) {
            try {
              const res = await fetch(htmlUrl)
              if (res.ok) {
                const htmlText = await res.text()
                modApi.injectHTML(htmlText, '#mod-container')
              }
            } catch (err) {
              console.error(`[ModManager] Error loading HTML for ${mod.id}:`, err)
            }
          }
        }

        // 3. Execute JS files
        if (mod.jsContents && mod.jsContents.length > 0) {
          for (const item of mod.jsContents) {
            try {
              const runScript = new Function('BCA_API', 'BCAMods', 'mod', item.code)
              runScript(modApi, modApi, mod)
            } catch (err) {
              console.error(`[ModManager] Error executing JS code for ${mod.id}:`, err)
            }
          }
        } else if (mod.jsFiles && mod.jsFiles.length > 0) {
          for (const jsUrl of mod.jsFiles) {
            try {
              const res = await fetch(jsUrl)
              if (res.ok) {
                const code = await res.text()
                const runScript = new Function('BCA_API', 'BCAMods', 'mod', code)
                runScript(modApi, modApi, mod)
              }
            } catch (err) {
              console.error(`[ModManager] Error executing JS file ${jsUrl} for ${mod.id}:`, err)
            }
          }
        }

        console.log(`[ModManager] Successfully loaded mod: ${mod.title} (v${mod.version})`)
      } catch (err) {
        console.error(`[ModManager] Failed to activate mod ${mod.id}:`, err)
      }
    }
  }
}

const modManager = new ModManager()
export default modManager
