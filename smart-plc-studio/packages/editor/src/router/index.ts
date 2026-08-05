import {
  createRouter,
  createWebHashHistory,
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
  {
    path: "/surgical-sim",
    name: "SurgicalSim",
    component: () => import("../views/SurgicalSim.vue"),
  },
  {
    path: "/topology",
    name: "Topology",
    component: () => import("../views/TopologyEditor.vue"),
  },
];

const router = createRouter({
  history: createWebHashHistory(),
  routes,
});

export default router;
