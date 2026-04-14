<script setup>
const urls = [
  ['/', 'Home'],
  ['/menu', 'Menu'],
  ['/corndog', 'Corndogcat edition'],
]
</script>

<template>
  <div class="menu-view">
    <div class="menu-card">
      <h1 class="menu-title">Navigation</h1>
      <p class="menu-subtitle">Choose a destination:</p>
      <ul class="url-list">
        <li v-for="[path, label] in urls" :key="path" class="url-item">
          <a :href="path" class="url-link">
            <span class="url-label">{{ label }}</span>
            <span class="url-path">{{ path }}</span>
          </a>
        </li>
      </ul>
    </div>
  </div>
</template>

<style scoped>
.menu-view {
  display: grid;
  place-items: center;
  width: 100%;
  min-height: 100vh;
  background: #000;
  color: #f1f1f1;
  font-family: Inter, Segoe UI, Roboto, Arial, sans-serif;
  padding: 2rem;
}

.menu-card {
  width: min(680px, 100%);
  border: 1px solid #2c2c2c;
  border-radius: 1rem;
  background: rgba(15, 15, 15, 0.95);
  padding: 1rem;
}

.menu-title {
  margin: 0;
  font-size: 1.3rem;
  font-weight: 700;
}

.menu-subtitle {
  margin: 0.35rem 0 1rem;
  color: #bdbdbd;
  font-size: 0.95rem;
}

.url-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  gap: 0.6rem;
}

.url-link {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.75rem;
  border: 1px solid #3a3a3a;
  border-radius: 0.75rem;
  padding: 0.7rem 0.85rem;
  color: #f1f1f1;
  text-decoration: none;
  background: rgba(30, 30, 30, 0.85);
  transition: border-color 140ms ease, background-color 140ms ease;
}

.url-link:hover {
  border-color: #6e6e6e;
  background: rgba(42, 42, 42, 0.9);
}

.url-link:focus-visible {
  outline: 2px solid #7aa2ff;
  outline-offset: 2px;
}

.url-label {
  font-size: 0.95rem;
  font-weight: 600;
}

.url-path {
  color: #bdbdbd;
  font-size: 0.88rem;
  font-family: Consolas, 'Courier New', monospace;
}

@media (max-width: 520px) {
  .url-link {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
