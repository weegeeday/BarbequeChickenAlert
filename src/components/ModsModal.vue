<script setup>
import { ref } from 'vue'
import modManager from '../services/modManager.js'

defineProps({
  isOpen: {
    type: Boolean,
    default: false,
  },
})

const emit = defineEmits(['close'])

const inputUrl = ref('')
const isLoading = ref(false)
const errorMessage = ref('')
const modPreview = ref(null)
const requiresReload = ref(false)
const zipFileInput = ref(null)

const installedMods = ref([...modManager.installedMods])

const refreshMods = () => {
  installedMods.value = [...modManager.loadSavedMods()]
}

const handleClose = () => {
  modPreview.value = null
  errorMessage.value = ''
  inputUrl.value = ''
  emit('close')
}

const triggerZipUpload = () => {
  if (zipFileInput.value) {
    zipFileInput.value.click()
  }
}

const handleZipFileUpload = async (e) => {
  const file = e.target.files?.[0]
  if (!file) return

  isLoading.value = true
  errorMessage.value = ''
  modPreview.value = null

  try {
    const preview = await modManager.parseModFromZipFile(file)
    modPreview.value = preview
  } catch (err) {
    console.error('[ModsModal] Error parsing zip mod:', err)
    errorMessage.value = err.message || 'Failed to parse .zip mod file.'
  } finally {
    isLoading.value = false
    if (zipFileInput.value) {
      zipFileInput.value.value = ''
    }
  }
}

const handleFetchMod = async () => {
  if (!inputUrl.value.trim()) {
    errorMessage.value = 'Please enter a valid URL or GitHub repository link.'
    return
  }

  isLoading.value = true
  errorMessage.value = ''
  modPreview.value = null

  try {
    const preview = await modManager.fetchModPreview(inputUrl.value)
    modPreview.value = preview
  } catch (err) {
    console.error('[ModsModal] Error fetching mod:', err)
    errorMessage.value = err.message || 'Failed to fetch mod. Please check the URL and try again.'
  } finally {
    isLoading.value = false
  }
}

const handleInstallMod = async () => {
  if (!modPreview.value) return

  try {
    await modManager.installMod(modPreview.value)
    refreshMods()
    modPreview.value = null
    inputUrl.value = ''
    requiresReload.value = true
  } catch (err) {
    console.error('[ModsModal] Install failed:', err)
    errorMessage.value = 'Failed to install mod: ' + err.message
  }
}

const handleToggleMod = (modId, currentEnabled) => {
  modManager.toggleMod(modId, !currentEnabled)
  refreshMods()
  requiresReload.value = true
}

const handleRemoveMod = (modId) => {
  if (confirm('Are you sure you want to remove this mod?')) {
    modManager.removeMod(modId)
    refreshMods()
    requiresReload.value = true
  }
}

const handleReloadGame = () => {
  window.location.reload()
}
</script>

<template>
  <div v-if="isOpen" class="mods-modal-overlay" @click.self="handleClose">
    <div class="mods-modal-card">
      <div class="mods-modal-header">
        <h2 class="mods-modal-title">Mod Manager</h2>
        <button type="button" class="close-btn" @click="handleClose">✕</button>
      </div>

      <!-- Reload Notice Banner -->
      <div v-if="requiresReload" class="reload-banner">
        <span>Changes made! Reload the game to apply mod updates.</span>
        <button type="button" class="reload-btn" @click="handleReloadGame">Reload Game Now</button>
      </div>

      <!-- Add Mod URL / Zip Section -->
      <div class="url-input-section">
        <label class="url-label">Import Mod from GitHub, URL, or .ZIP Archive:</label>
        <div class="url-input-row">
          <input
            v-model="inputUrl"
            type="text"
            placeholder="e.g. https://github.com/user/repo or mod.zip link"
            class="url-input"
            :disabled="isLoading"
            @keyup.enter="handleFetchMod"
          />
          <button
            type="button"
            class="fetch-btn"
            :disabled="isLoading || !inputUrl.trim()"
            @click="handleFetchMod"
          >
            {{ isLoading ? 'Fetching...' : 'Fetch Mod' }}
          </button>
          <button
            type="button"
            class="upload-zip-btn"
            :disabled="isLoading"
            @click="triggerZipUpload"
          >
            Upload .ZIP
          </button>
          <input
            ref="zipFileInput"
            type="file"
            accept=".zip"
            style="display: none;"
            @change="handleZipFileUpload"
          />
        </div>
        <div v-if="errorMessage" class="error-msg">{{ errorMessage }}</div>
        <div class="url-hint">
          Supports GitHub repository links, direct <code>.zip</code> URLs, or uploading local <code>.zip</code> mod folders.
        </div>
      </div>

      <!-- Mod Preview / Install Dialog -->
      <div v-if="modPreview" class="mod-preview-card">
        <div class="preview-header">
          <img :src="modPreview.image" alt="Mod Icon" class="mod-icon" />
          <div class="preview-details">
            <h3 class="preview-title">{{ modPreview.title }} <span class="preview-version">v{{ modPreview.version }}</span></h3>
            <div class="preview-author">By {{ modPreview.author }}</div>
            <div class="tier-badges">
              <span v-for="tier in modPreview.tiers" :key="tier" class="tier-badge">{{ tier }}</span>
            </div>
          </div>
        </div>

        <p class="preview-desc">{{ modPreview.description }}</p>

        <!-- External Libraries Warning Banner -->
        <div v-if="modPreview.externalLibraries && modPreview.externalLibraries.length > 0" class="warning-banner">
          <div class="warning-title">Warning: Custom External Libraries Detected</div>
          <ul class="warning-list">
            <li v-for="(lib, idx) in modPreview.externalLibraries" :key="idx">{{ lib }}</li>
          </ul>
          <div class="warning-sub">This mod references external JavaScript libraries/scripts. You can still install it if you trust the source.</div>
        </div>

        <div class="preview-actions">
          <button type="button" class="cancel-btn" @click="modPreview = null">Cancel</button>
          <button type="button" class="install-btn" @click="handleInstallMod">Install & Add Mod</button>
        </div>
      </div>

      <!-- Installed Mods List -->
      <div class="installed-mods-section">
        <h3 class="section-title">Installed Mods ({{ installedMods.length }})</h3>

        <div v-if="installedMods.length === 0" class="no-mods-msg">
          No mods installed yet. Import a mod URL above to get started!
        </div>

        <div v-else class="mods-list">
          <div
            v-for="mod in installedMods"
            :key="mod.id"
            class="mod-card"
            :class="{ disabled: !mod.enabled }"
          >
            <img :src="mod.image" alt="Icon" class="mod-list-icon" />
            <div class="mod-info">
              <div class="mod-header-row">
                <span class="mod-name">{{ mod.title }}</span>
                <span class="mod-version">v{{ mod.version }}</span>
                <div class="tier-badges">
                  <span v-for="tier in mod.tiers" :key="tier" class="tier-badge micro">{{ tier }}</span>
                </div>
              </div>
              <div class="mod-author">By {{ mod.author }}</div>
              <div class="mod-desc">{{ mod.description }}</div>
            </div>

            <div class="mod-actions">
              <button
                type="button"
                class="toggle-btn"
                :class="{ active: mod.enabled }"
                @click="handleToggleMod(mod.id, mod.enabled)"
              >
                {{ mod.enabled ? 'Enabled' : 'Disabled' }}
              </button>
              <button
                type="button"
                class="remove-btn"
                title="Uninstall Mod"
                @click="handleRemoveMod(mod.id)"
              >
                ✕
              </button>
            </div>
          </div>
        </div>
      </div>

      <!-- Developer Docs Footer -->
      <div class="dev-docs-footer">
        Looking to create a mod? Read the <a href="https://github.com/weegeeday/BarbequeChickenAlert/blob/main/modding/MOD_DEVELOPMENT_GUIDE.md" target="_blank" rel="noopener noreferrer" class="dev-docs-link">Mod Development Guide</a>.
      </div>
    </div>
  </div>
</template>

<style scoped>
.mods-modal-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
  background: rgba(0, 0, 0, 0.75);
  backdrop-filter: blur(6px);
  display: grid;
  place-items: center;
  padding: 1rem;
}

.mods-modal-card {
  width: min(720px, 95vw);
  max-height: 90vh;
  overflow-y: auto;
  background: #16181d;
  border: 1px solid #2e3340;
  border-radius: 1rem;
  color: #e2e8f0;
  padding: 1.5rem;
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  gap: 1.25rem;
}

.mods-modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid #2a2f3d;
  padding-bottom: 0.75rem;
}

.mods-modal-title {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 700;
  color: #ffffff;
}

.close-btn {
  background: transparent;
  border: none;
  color: #94a3b8;
  font-size: 1.4rem;
  cursor: pointer;
  padding: 0.2rem 0.5rem;
}
.close-btn:hover {
  color: #ffffff;
}

.reload-banner {
  background: rgba(234, 179, 8, 0.15);
  border: 1px solid rgba(234, 179, 8, 0.4);
  padding: 0.75rem 1rem;
  border-radius: 0.5rem;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 1rem;
  color: #fef08a;
  font-size: 0.9rem;
}

.reload-btn {
  background: #eab308;
  color: #000;
  border: none;
  font-weight: 700;
  padding: 0.4rem 0.8rem;
  border-radius: 0.4rem;
  cursor: pointer;
}
.reload-btn:hover {
  background: #facc15;
}

.url-input-section {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.url-label {
  font-size: 0.9rem;
  font-weight: 600;
  color: #cbd5e1;
}

.url-input-row {
  display: flex;
  gap: 0.5rem;
}

.url-input {
  flex: 1;
  background: #0f1115;
  border: 1px solid #334155;
  border-radius: 0.5rem;
  padding: 0.6rem 0.8rem;
  color: #ffffff;
  font-size: 0.9rem;
}
.url-input:focus {
  outline: none;
  border-color: #3b82f6;
}

.fetch-btn {
  background: #2563eb;
  color: #ffffff;
  border: none;
  border-radius: 0.5rem;
  padding: 0.6rem 1rem;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
}
.fetch-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
.fetch-btn:hover:not(:disabled) {
  background: #3b82f6;
}

.upload-zip-btn {
  background: #059669;
  color: #ffffff;
  border: none;
  border-radius: 0.5rem;
  padding: 0.6rem 1rem;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
}
.upload-zip-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
.upload-zip-btn:hover:not(:disabled) {
  background: #10b981;
}

.url-hint {
  font-size: 0.8rem;
  color: #64748b;
}
.url-hint code {
  background: #0f1115;
  padding: 0.1rem 0.3rem;
  border-radius: 0.2rem;
  color: #93c5fd;
}

.error-msg {
  color: #f87171;
  font-size: 0.85rem;
  margin-top: 0.2rem;
}

.mod-preview-card {
  background: #1e222d;
  border: 1px solid #3b82f6;
  border-radius: 0.75rem;
  padding: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.8rem;
}

.preview-header {
  display: flex;
  gap: 0.8rem;
  align-items: center;
}

.mod-icon, .mod-list-icon {
  width: 48px;
  height: 48px;
  border-radius: 0.5rem;
  object-fit: cover;
  background: #0f1115;
}

.preview-details {
  display: flex;
  flex-direction: column;
  gap: 0.2rem;
}

.preview-title {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 700;
  color: #ffffff;
}

.preview-version {
  font-size: 0.85rem;
  color: #94a3b8;
  font-weight: normal;
}

.preview-author {
  font-size: 0.85rem;
  color: #cbd5e1;
}

.preview-desc {
  margin: 0;
  font-size: 0.9rem;
  color: #94a3b8;
  line-height: 1.4;
}

.tier-badges {
  display: flex;
  gap: 0.3rem;
  flex-wrap: wrap;
}

.tier-badge {
  background: #334155;
  color: #38bdf8;
  font-size: 0.75rem;
  font-weight: 600;
  padding: 0.15rem 0.4rem;
  border-radius: 0.25rem;
}
.tier-badge.micro {
  font-size: 0.7rem;
  padding: 0.1rem 0.3rem;
}

.warning-banner {
  background: rgba(239, 68, 68, 0.12);
  border: 1px solid rgba(239, 68, 68, 0.3);
  padding: 0.6rem 0.8rem;
  border-radius: 0.5rem;
  color: #fca5a5;
  font-size: 0.85rem;
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
}

.warning-title {
  font-weight: 700;
}

.warning-list {
  margin: 0;
  padding-left: 1.2rem;
  font-size: 0.8rem;
  word-break: break-all;
}

.warning-sub {
  font-size: 0.75rem;
  color: #f87171;
}

.preview-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.6rem;
  margin-top: 0.4rem;
}

.cancel-btn {
  background: #334155;
  color: #cbd5e1;
  border: none;
  padding: 0.5rem 1rem;
  border-radius: 0.4rem;
  cursor: pointer;
}

.install-btn {
  background: #16a34a;
  color: #ffffff;
  border: none;
  font-weight: 700;
  padding: 0.5rem 1rem;
  border-radius: 0.4rem;
  cursor: pointer;
}
.install-btn:hover {
  background: #22c55e;
}

.installed-mods-section {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.section-title {
  margin: 0;
  font-size: 1.05rem;
  font-weight: 700;
  color: #cbd5e1;
}

.no-mods-msg {
  text-align: center;
  color: #64748b;
  font-size: 0.9rem;
  padding: 1.5rem;
  background: #0f1115;
  border-radius: 0.5rem;
  border: 1px dashed #2e3340;
}

.mods-list {
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
}

.mod-card {
  display: flex;
  align-items: center;
  gap: 0.8rem;
  background: #1a1d24;
  border: 1px solid #2e3340;
  border-radius: 0.6rem;
  padding: 0.75rem 0.9rem;
  transition: opacity 150ms ease;
}

.mod-card.disabled {
  opacity: 0.5;
}

.mod-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 0.2rem;
}

.mod-header-row {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.mod-name {
  font-weight: 700;
  color: #ffffff;
  font-size: 0.95rem;
}

.mod-version {
  font-size: 0.75rem;
  color: #94a3b8;
}

.mod-author {
  font-size: 0.8rem;
  color: #64748b;
}

.mod-desc {
  font-size: 0.82rem;
  color: #94a3b8;
}

.mod-actions {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.toggle-btn {
  background: #334155;
  color: #94a3b8;
  border: none;
  padding: 0.4rem 0.75rem;
  border-radius: 0.4rem;
  font-size: 0.8rem;
  font-weight: 600;
  cursor: pointer;
}

.toggle-btn.active {
  background: #15803d;
  color: #ffffff;
}

.remove-btn {
  background: transparent;
  border: 1px solid rgba(239, 68, 68, 0.4);
  color: #ef4444;
  font-weight: bold;
  font-size: 1rem;
  line-height: 1;
  cursor: pointer;
  padding: 0.35rem 0.6rem;
  border-radius: 0.4rem;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background-color 150ms ease, color 150ms ease;
}
.remove-btn:hover {
  background: #dc2626;
  color: #ffffff;
  border-color: #dc2626;
}

.dev-docs-footer {
  text-align: center;
  font-size: 0.78rem;
  color: #64748b;
  margin-top: 0.5rem;
  padding-top: 0.75rem;
  border-top: 1px solid #2e3340;
}

.dev-docs-link {
  color: #60a5fa;
  text-decoration: underline;
  font-weight: 500;
}
.dev-docs-link:hover {
  color: #93c5fd;
}
</style>
