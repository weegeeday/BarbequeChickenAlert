import { createRouter, createWebHistory } from 'vue-router'
import GameView from '../views/GameView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: GameView,
    },
    {
      path: '/home',
      redirect: '/',
    },
    {
      path: '/menu',
      name: 'menu',
      component: () => import('../views/MenuListView.vue'),
    },
    {
      path: '/corndog',
      name: 'corndog',
      component: () => import('../views/Corndog.vue'),
    },
    {
      path: '/moai',
      name: 'moai',
      component: () => import('../views/Moai.vue'),
    },
    {
      path: '/custom',
      name: 'custom',
      component: () => import('../views/Custom.vue'),
    }
  ],
})

export default router
