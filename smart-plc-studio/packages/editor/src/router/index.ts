import {
  createRouter,
  createWebHistory,
  type RouteRecordRaw,
} from "vue-router";

const routes: RouteRecordRaw[] = [
  {
    path: "/",
    name: "IDE",
    component: () => import("../views/IDE.vue"),
  },
  {
    path: "/welcome",
    name: "Welcome",
    component: () => import("../views/Welcome.vue"),
  },
  {
    path: "/simulator",
    name: "Simulator",
    component: () => import("../views/Simulator.vue"),
  },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

export default router;
